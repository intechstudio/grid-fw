/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_touch.h"

#include <errno.h>
#include <string.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/ets_sys.h"

#include "grid_math.h"
#include "grid_platform.h"

#include "mxt144u_cfg.h"

#define MXT_I2C_ADDR 0x4A

#define MXT_CFG_MAGIC "OBP_RAW V4\r\n"
#define MXT_CFG_ENCRYPTION "ENCRYPTION 0\r\nMAX_ENCRYPTION_BLOCKS 0\r\n"
#define MXT_CFG_NO_DEVICES "NO_DEVICES 1\r\n"
#define MXT_CFG_DEVICE_0 "DEVICE_0\r\n"

#define MXT_OBJECT_START 0x07
#define MXT_OBJECT_SIZE 6
#define MXT_INFO_CHECKSUM_SIZE 3
#define MXT_MAX_BLOCK_WRITE 256

#define MXT_DEBUG_DIAGNOSTIC_T37 37
#define MXT_GEN_MESSAGE_T5 5
#define MXT_GEN_COMMAND_T6 6
#define MXT_GEN_POWER_T7 7
#define MXT_SPT_MESSAGECOUNT_T44 44
#define MXT_GEN_DYNAMICCONFIGURATIONCONTAINER_T71 71
#define MXT_TOUCH_MULTITOUCHSCREEN_T100 100

#define MXT_RPTID_NOMSG 0xff

#define MXT_COMMAND_RESET 0
#define MXT_COMMAND_BACKUPNV 1
#define MXT_COMMAND_CALIBRATE 2
#define MXT_COMMAND_REPORTALL 3
#define MXT_COMMAND_DIAGNOSTIC 5

#define MXT_T6_STATUS_RESET BIT(7)
#define MXT_T6_STATUS_OFL BIT(6)
#define MXT_T6_STATUS_SIGERR BIT(5)
#define MXT_T6_STATUS_CAL BIT(4)
#define MXT_T6_STATUS_CFGERR BIT(3)
#define MXT_T6_STATUS_COMSERR BIT(2)

#define MXT_RESET_VALUE 0x01
#define MXT_BACKUP_VALUE 0x55

#define MXT_T100_DETECT BIT(7)
#define MXT_T100_TYPE_MASK 0x70
#define MXT_T100_EVENT_MASK 0x0f

#define MXT_DIAGNOSTIC_PAGEUP 0x01
#define MXT_DIAGNOSTIC_DELTAS 0x10
#define MXT_DIAGNOSTIC_REFS 0x11

enum t100_type {
  MXT_T100_TYPE_FINGER = 1,
  MXT_T100_TYPE_PASSIVE_STYLUS = 2,
  MXT_T100_TYPE_HOVERING_FINGER = 4,
  MXT_T100_TYPE_GLOVE = 5,
  MXT_T100_TYPE_LARGE_TOUCH = 6,
};

DRAM_ATTR struct grid_esp32_touch_model grid_esp32_touch_state;

static void IRAM_ATTR mxt_chg_isr(void* user) {

  struct grid_esp32_touch_model* touch = (struct grid_esp32_touch_model*)user;

  xTaskNotifyFromISR(touch->task, 0, eNoAction, NULL);
}

static size_t mxt_obj_size(const struct mxt_object* obj) { return obj->size_minus_one + 1; }

static size_t mxt_obj_instances(const struct mxt_object* obj) { return obj->inst_minus_one + 1; }

static esp_err_t mxt_read_reg(i2c_master_dev_handle_t dev, uint16_t addr, uint16_t size, uint8_t* dest) {

  uint8_t tx[2] = {addr & 0xff, addr >> 8};
  return i2c_master_transmit_receive(dev, tx, 2, dest, size, -1);
}

static esp_err_t mxt_write_reg(i2c_master_dev_handle_t dev, uint16_t addr, uint16_t size, uint8_t* src) {

  uint8_t* buf = malloc(size + 2);
  buf[0] = addr & 0xff;
  buf[1] = (addr >> 8) & 0xff;
  memcpy(&buf[2], src, size);

  esp_err_t err = i2c_master_transmit(dev, buf, size + 2, -1);

  free(buf);

  return err;
}

static struct mxt_object* mxt_get_object(struct mxt_data* data, uint8_t type) {

  for (int i = 0; i < data->info.object_num; ++i) {

    struct mxt_object* object = &data->object_table[i];

    if (object->type == type) {
      return object;
    }
  }

  ets_printf("mxt_get_object: invalid type T%hhu\n", type);
  return NULL;
}

static esp_err_t mxt_t6_command(struct mxt_data* data, uint8_t cmd_off, uint8_t val, bool wait) {

  uint16_t reg = data->T6_address + cmd_off;

  esp_err_t err = mxt_write_reg(data->dev_hndl, reg, 1, &val);
  if (err != ESP_OK) {
    return err;
  }

  if (!wait) {
    return 0;
  }

  uint8_t cmd_reg;
  int timeout_counter = 0;

  do {

    vTaskDelay(pdMS_TO_TICKS(20));
    err = mxt_read_reg(data->dev_hndl, reg, 1, &cmd_reg);
    if (err != ESP_OK) {
      return err;
    }

  } while (cmd_reg && timeout_counter++ <= 100);

  if (timeout_counter > 100) {
    return ESP_FAIL;
  }

  return ESP_OK;
}

static esp_err_t mxt_update_crc(struct mxt_data* data, uint8_t cmd, uint8_t val) {

  data->config_crc = 0;
  data->crc_completion = 0;

  esp_err_t err = mxt_t6_command(data, cmd, val, true);
  if (err != ESP_OK) {
    return err;
  }

  while (!data->crc_completion) {
    vTaskDelay(1);
  }

  return ESP_OK;
}

static void mxt_calc_crc24(uint32_t* crc, uint8_t firstbyte, uint8_t secondbyte) {

  static const unsigned int crcpoly = 0x80001B;
  uint32_t result;
  uint32_t data_word;

  data_word = (secondbyte << 8) | firstbyte;
  result = ((*crc << 1) ^ data_word);

  if (result & 0x1000000) {
    result ^= crcpoly;
  }

  *crc = result;
}

static uint32_t mxt_calculate_crc(uint8_t* base, off_t start_off, off_t end_off) {

  uint32_t crc = 0;
  uint8_t* ptr = base + start_off;
  uint8_t* last_val = base + end_off - 1;

  if (end_off < start_off) {
    return -EINVAL;
  }

  while (ptr < last_val) {
    mxt_calc_crc24(&crc, *ptr, *(ptr + 1));
    ptr += 2;
  }

  if (ptr == last_val) {
    mxt_calc_crc24(&crc, *ptr, 0);
  }

  crc &= 0x00FFFFFF;

  return crc;
}

struct firmware {
  size_t size;
  uint8_t* data;
};

struct mxt_cfg {
  uint8_t* raw;
  size_t raw_size;
  size_t raw_pos;
  struct mxt_info info;
  size_t start_ofs;
  uint8_t* mem;
  uint16_t mem_size;
};

static int mxt_prepare_cfg_mem(struct mxt_data* data, struct mxt_cfg* cfg) {

  int ret;

  unsigned int type, inst, size;
  int offset;
  while (cfg->raw_pos < cfg->raw_size) {

    // Read type, instance, length
    ret = sscanf((char*)&cfg->raw[cfg->raw_pos], "%x %x %x%n", &type, &inst, &size, &offset);

    if (ret == 0) {
      // EOF
      break;
    } else if (ret != 3) {
      ets_printf("mxt_prepare_cfg_mem: bad format\n");
      return -EINVAL;
    }
    cfg->raw_pos += offset;

    struct mxt_object* object = mxt_get_object(data, type);
    if (!object) {

      // Skip object
      uint8_t val;
      for (int i = 0; i < size; ++i) {
        ret = sscanf((char*)&cfg->raw[cfg->raw_pos], "%hhx %n", &val, &offset);
        if (ret != 1) {
          ets_printf("mxt_prepare_cfg_mem: bad format in T%d at %d\n", type, i);
          return -EINVAL;
        }
        cfg->raw_pos += offset;
      }
      continue;
    }

    if (size > mxt_obj_size(object)) {

      // Either we are in fallback mode due to a wrong config,
      // or this is a configuration from a later fw version,
      // or this is a corrupt or hand-edited configuration.
      ets_printf("mxt_prepare_cfg_mem: discarding %u byte(s) in T%d\n", size - mxt_obj_size(object), type);

    } else if (mxt_obj_size(object) > size) {

      // An upgraded firmware may add new bytes to the end of objects.
      // It is generally forward compatible to zero these bytes, and
      // previous behaviour will be retained. However, this does invalidate
      // the CRC and will force fallback mode until the config is updated.
      ets_printf("mxt_prepare_cfg_mem: zeroing %u byte(s) in T%d\n", mxt_obj_size(object) - size, type);
    }

    if (inst >= mxt_obj_instances(object)) {
      ets_printf("mxt_prepare_cfg_mem: too many instances\n");
      return -EINVAL;
    }

    uint16_t reg = object->start_addr + mxt_obj_size(object) * inst;

    uint8_t val;
    for (int i = 0; i < size; ++i) {

      ret = sscanf((char*)&cfg->raw[cfg->raw_pos], "%hhx %n", &val, &offset);
      if (ret != 1) {
        ets_printf("mxt_prepare_cfg_mem: bad format in T%d at %d\n", type, i);
        return -EINVAL;
      }
      cfg->raw_pos += offset;

      if (i >= mxt_obj_size(object)) {
        continue;
      }

      unsigned int byte_off = reg + i - cfg->start_ofs;

      if (byte_off >= cfg->mem_size) {
        ets_printf("mxt_prepare_cfg_mem: bad object (reg %d) in T%d at %d\n", reg, object->type, byte_off);
        return -EINVAL;
      }

      *(cfg->mem + byte_off) = val;
    }
  }

  return 0;
}

static esp_err_t mxt_upload_cfg_mem(struct mxt_data* data, struct mxt_cfg* cfg) {

  for (size_t off = 0; off < cfg->mem_size; off += MXT_MAX_BLOCK_WRITE) {

    uint16_t size = MIN(cfg->mem_size - off, MXT_MAX_BLOCK_WRITE);

    esp_err_t err = mxt_write_reg(data->dev_hndl, cfg->start_ofs + off, size, cfg->mem + off);
    if (err != ESP_OK) {
      return err;
    }
  }

  return ESP_OK;
}

static esp_err_t mxt_soft_reset(struct mxt_data* data) {

  data->reset_completion = 0;

  esp_err_t err = mxt_t6_command(data, MXT_COMMAND_RESET, MXT_RESET_VALUE, false);
  if (err != ESP_OK) {
    return err;
  }

  vTaskDelay(pdMS_TO_TICKS(100));

  while (!data->reset_completion) {
    vTaskDelay(1);
  }

  return ESP_OK;
}

static esp_err_t mxt_update_cfg(struct mxt_data* data, struct firmware* fw) {

  esp_err_t ret = ESP_OK;

  mxt_update_crc(data, MXT_COMMAND_REPORTALL, 1);

  // Initialize config update context
  struct mxt_cfg cfg = (struct mxt_cfg){
      .raw = NULL,
      .raw_size = fw->size,
  };

  // Allocate zero terminated copy of the file
  cfg.raw = malloc(cfg.raw_size + 1);
  if (!cfg.raw) {
    goto mxt_update_cfg_error;
  }
  memcpy(cfg.raw, fw->data, cfg.raw_size);
  cfg.raw[cfg.raw_size] = '\0';

  // Parse magic bytes
  if (strncmp((char*)&cfg.raw[cfg.raw_pos], MXT_CFG_MAGIC, strlen(MXT_CFG_MAGIC))) {
    goto mxt_update_cfg_error;
  }
  cfg.raw_pos += strlen(MXT_CFG_MAGIC);

  // Parse encryption configuration
  if (strncmp((char*)&cfg.raw[cfg.raw_pos], MXT_CFG_ENCRYPTION, strlen(MXT_CFG_ENCRYPTION))) {
    goto mxt_update_cfg_error;
  }
  cfg.raw_pos += strlen(MXT_CFG_ENCRYPTION);

  // Parse number of devices, expecting only one
  if (strncmp((char*)&cfg.raw[cfg.raw_pos], MXT_CFG_NO_DEVICES, strlen(MXT_CFG_NO_DEVICES))) {
    goto mxt_update_cfg_error;
  }
  cfg.raw_pos += strlen(MXT_CFG_NO_DEVICES);

  // Parse information block
  int offset;
  for (size_t i = 0; i < sizeof(struct mxt_info); ++i) {

    if (sscanf((char*)&cfg.raw[cfg.raw_pos], "%hhx %n", (uint8_t*)&cfg.info + i, &offset) != 1) {
      goto mxt_update_cfg_error;
    }
    cfg.raw_pos += offset;
  }

  if (cfg.info.family_id != data->info.family_id) {
    ets_printf("mxt_update_cfg: family ID mismatch\n");
    goto mxt_update_cfg_error;
  }

  if (cfg.info.variant_id != data->info.variant_id) {
    ets_printf("mxt_update_cfg: variant ID mismatch\n");
    goto mxt_update_cfg_error;
  }

  // Parse info CRC
  uint32_t info_crc;
  if (sscanf((char*)&cfg.raw[cfg.raw_pos], "%x\r\n%n", (unsigned int*)&info_crc, &offset) != 1) {
    goto mxt_update_cfg_error;
  }
  cfg.raw_pos += offset;

  // Parse config CRC
  uint32_t config_crc;
  if (sscanf((char*)&cfg.raw[cfg.raw_pos], "%x\r\n%n", (unsigned int*)&config_crc, &offset) != 1) {
    goto mxt_update_cfg_error;
  }
  cfg.raw_pos += offset;

  // Check for CRC mismatches
  if (info_crc == data->info_crc) {

    if (config_crc == 0 || data->config_crc == 0) {

      ets_printf("mxt_update_cfg: config CRC zero, attempting to apply config\n");

    } else if (config_crc == data->config_crc) {

      ets_printf("mxt_update_cfg: config CRC %06X OK\n", data->config_crc);
      goto mxt_update_cfg_cleanup;

    } else {

      ets_printf("mxt_update_cfg: config CRC device %06X != file %06X\n", data->config_crc, config_crc);
    }

  } else {

    ets_printf("mxt_update_cfg: info CRC device %06X != file %06X\n", data->info_crc, info_crc);
  }

  // The start address of objects on the device is right after the info block
  uint16_t obj_table_size = sizeof(struct mxt_object) * data->info.object_num;
  uint16_t info_size = MXT_OBJECT_START + obj_table_size + MXT_INFO_CHECKSUM_SIZE;
  cfg.start_ofs = info_size;

  if (data->mem_size <= cfg.start_ofs) {
    ets_printf("mxt_update_cfg: mem_size too small, %hu < %hu\n", data->mem_size, cfg.start_ofs);
    goto mxt_update_cfg_error;
  }

  // Allocate memory to store configuration
  cfg.mem_size = data->mem_size - cfg.start_ofs;
  cfg.mem = malloc(cfg.mem_size);
  if (!cfg.mem) {
    ets_printf("mxt_update_cfg: failed to allocate cfg.mem\n");
    goto mxt_update_cfg_error;
  }
  memset(cfg.mem, 0, cfg.mem_size);

  // Parse the index of the first device, expect it to be zero
  if (strncmp((char*)&cfg.raw[cfg.raw_pos], MXT_CFG_DEVICE_0, strlen(MXT_CFG_DEVICE_0))) {
    goto mxt_update_cfg_error;
  }
  cfg.raw_pos += strlen(MXT_CFG_DEVICE_0);

  int error = mxt_prepare_cfg_mem(data, &cfg);
  if (error) {
    goto mxt_update_cfg_error;
  }

  // Calculate CRC of the received configs (not the raw config file)
  uint16_t crc_start = 0;
  if (data->T71_address) {
    crc_start = data->T71_address;
  } else if (data->T7_address) {
    crc_start = data->T7_address;
  } else {
    ets_printf("mxt_update_cfg: could not find CRC start\n");
  }

  if (crc_start > cfg.start_ofs) {

    uint32_t calc_crc = mxt_calculate_crc(cfg.mem, crc_start - cfg.start_ofs, cfg.mem_size);

    if (config_crc > 0 && config_crc != calc_crc) {
      ets_printf("mxt_update_cfg: config CRC calculated %06X != file %06X\n", calc_crc, config_crc);
    }
  }

  esp_err_t err = mxt_upload_cfg_mem(data, &cfg);
  if (err != ESP_OK) {
    ets_printf("mxt_update_cfg: config upload error\n");
    goto mxt_update_cfg_error;
  }

  err = mxt_update_crc(data, MXT_COMMAND_BACKUPNV, MXT_BACKUP_VALUE);
  if (err != ESP_OK) {
    ets_printf("mxt_update_cfg: backup error (%d)\n", err);
  }

  err = mxt_soft_reset(data);
  if (err != ESP_OK) {
    ets_printf("mxt_update_cfg: soft reset error (%d)\n", err);
  }

  // Note that there are more steps that are required for an update to take
  // effect on the fly (with a soft reset). Currently, we rely on a hard reset.
  goto mxt_update_cfg_cleanup;

mxt_update_cfg_error:
  ret = ESP_FAIL;
mxt_update_cfg_cleanup:
  if (cfg.raw) {
    free(cfg.raw);
  }
  if (cfg.mem) {
    free(cfg.mem);
  }
  return ret;
}

static void grid_esp32_touch_init_gpio(struct grid_esp32_touch_model* touch, gpio_num_t reset, gpio_num_t change) {

  touch->rst = reset;

  gpio_config_t rst_cfg = {
      .pin_bit_mask = 1ULL << touch->rst,
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&rst_cfg));

  touch->chg = change;

  gpio_config_t chg_cfg = {
      .pin_bit_mask = 1ULL << touch->chg,
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&chg_cfg));
}

static bool grid_esp32_touch_reset(struct grid_esp32_touch_model* touch) {

  // Hold RST low for at least 90 ns
  gpio_set_level(touch->rst, 0);
  vTaskDelay(pdMS_TO_TICKS(20));

  // Send RST high for more than 39 ms (typical hardware reset to CHG low time)
  gpio_set_level(touch->rst, 1);
  vTaskDelay(pdMS_TO_TICKS(100));

  // CHG should be low at this point
  return gpio_get_level(touch->chg) == 0;
}

static void grid_esp32_touch_init_i2c(struct grid_esp32_touch_model* touch, i2c_port_t port, gpio_num_t sda, gpio_num_t scl, uint32_t scl_freq) {

  touch->bus_conf = (i2c_master_bus_config_t){
      .i2c_port = port,
      .sda_io_num = sda,
      .scl_io_num = scl,
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .glitch_ignore_cnt = 7,
      .flags.enable_internal_pullup = true,
  };

  ESP_ERROR_CHECK(i2c_new_master_bus(&touch->bus_conf, &touch->bus_hndl));

  i2c_device_config_t dev_cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = 0x4A,
      .scl_speed_hz = scl_freq,
  };

  ESP_ERROR_CHECK(i2c_master_bus_add_device(touch->bus_hndl, &dev_cfg, &touch->data.dev_hndl));
}

static void grid_esp32_touch_parse_info_block(struct grid_esp32_touch_model* touch, uint8_t* iblk) {

  struct mxt_info* info = (struct mxt_info*)iblk;

  struct mxt_data* data = &touch->data;

  // Parse object table
  uint8_t reportid = 1;
  touch->data.mem_size = 0;
  uint8_t min_id = 0;
  uint8_t max_id = 0;
  for (uint8_t i = 0; i < info->object_num; ++i) {

    struct mxt_object* object = (struct mxt_object*)&iblk[MXT_OBJECT_START + i * MXT_OBJECT_SIZE];

    if (object->num_report_ids) {

      min_id = reportid;
      reportid += object->num_report_ids * mxt_obj_instances(object);
      max_id = reportid - 1;

    } else {

      min_id = max_id = 0;
    }

    switch (object->type) {
    case MXT_GEN_MESSAGE_T5: {

      data->T5_address = object->start_addr;

      if (info->family_id == 0x80 && info->version < 0x20) {
        data->T5_msg_size = mxt_obj_size(object);
      } else {
        data->T5_msg_size = mxt_obj_size(object) - 1;
      }

    } break;
    case MXT_GEN_COMMAND_T6: {

      touch->data.T6_address = object->start_addr;
      touch->data.T6_reportid = min_id;

    } break;
    case MXT_GEN_POWER_T7: {

      touch->data.T7_address = object->start_addr;

    } break;
    case MXT_SPT_MESSAGECOUNT_T44: {

      data->T44_address = object->start_addr;

    } break;
    case MXT_GEN_DYNAMICCONFIGURATIONCONTAINER_T71: {

      data->T71_address = object->start_addr;

    } break;
    case MXT_TOUCH_MULTITOUCHSCREEN_T100: {

      data->T100_rid_min = min_id;
      data->T100_rid_max = max_id;

    } break;
    }

    // Update the maximum memory size based on the address following the object
    uint16_t addr_after = object->start_addr + mxt_obj_size(object) * mxt_obj_instances(object);
    if (addr_after >= touch->data.mem_size) {
      touch->data.mem_size = addr_after;
    }
  }

  // Store maximum reportid
  data->max_reportid = reportid;
}

bool grid_esp32_touch_read_info_block(struct grid_esp32_touch_model* touch) {

  esp_err_t err;

  // Read ID information block
  struct mxt_info* info = &touch->data.info;
  err = mxt_read_reg(touch->data.dev_hndl, 0, sizeof(struct mxt_info), (uint8_t*)info);
  if (err != ESP_OK) {
    ets_printf("grid_esp32_touch_read_info_block: failed to read first 7 bytes\n");
    return false;
  }

  // Allocate information block
  size_t info_size = MXT_OBJECT_START + sizeof(struct mxt_object) * info->object_num + MXT_INFO_CHECKSUM_SIZE;
  uint8_t* iblk = touch->data.raw_info_block = malloc(info_size);
  if (!iblk) {
    ets_printf("grid_esp32_touch_read_info_block: failed allocate info block\n");
    return false;
  }

  // Read information block
  err = mxt_read_reg(touch->data.dev_hndl, 0, info_size, iblk);
  if (err != ESP_OK) {
    ets_printf("grid_esp32_touch_read_info_block: failed to read info block\n");
    return false;
  }

  // Extract checksum
  uint8_t* crc_ptr = &iblk[info_size - MXT_INFO_CHECKSUM_SIZE];
  uint32_t info_crc = crc_ptr[0] | crc_ptr[1] << 8 | crc_ptr[2] << 16;
  uint32_t calc_crc = mxt_calculate_crc(iblk, 0, info_size - MXT_INFO_CHECKSUM_SIZE);
  if (info_crc == 0 || info_crc != calc_crc) {
    ets_printf("grid_esp32_touch_read_info_block: crc mismatch\n");
    return false;
  }
  touch->data.info_crc = info_crc;

  ets_printf("family %02X variant %02X firmware v%u.%u.%02X objects %u\n", info->family_id, info->variant_id, info->version >> 4, info->version & 0xf, info->build, info->object_num);

  // Parse information block
  grid_esp32_touch_parse_info_block(touch, iblk);

  touch->data.object_table = (struct mxt_object*)&touch->data.raw_info_block[MXT_OBJECT_START];

  return true;
}

static void mxt_debug_init(struct mxt_data* data) {

  struct mxt_info* info = &data->info;
  struct mxt_dbg* dbg = &data->dbg;
  struct mxt_object* object;

  object = mxt_get_object(data, MXT_GEN_COMMAND_T6);
  if (!object) {
    goto mxt_debug_init_error;
  }

  dbg->diag_cmd_address = object->start_addr + MXT_COMMAND_DIAGNOSTIC;

  object = mxt_get_object(data, MXT_DEBUG_DIAGNOSTIC_T37);
  if (!object) {
    goto mxt_debug_init_error;
  }

  if (mxt_obj_size(object) != sizeof(struct t37_debug)) {
    ets_printf("bad t37 size\n");
    goto mxt_debug_init_error;
  }

  dbg->T37_address = object->start_addr;

  dbg->t37_nodes = MXT_XYNODES;

  dbg->t37_pages = (dbg->t37_nodes * sizeof(uint16_t)) / sizeof(dbg->t37_buf->data) + 1;

  if (dbg->t37_pages != MXT_DBG_PAGES_MIN) {
    ets_printf("wrong number of allocated debug pages\n");
    goto mxt_debug_init_error;
  }

  return;

mxt_debug_init_error:
  ets_printf("mxt_debug_init: error initializing t37\n");
}

bool grid_esp32_touch_init(struct grid_esp32_touch_model* touch, i2c_port_t i2c_port, gpio_num_t scl_gpio, gpio_num_t sda_gpio, gpio_num_t reset, gpio_num_t change, uint32_t i2c_freq_hz,
                           grid_process_touch_t process_touch, TaskHandle_t task) {

  touch->process_touch = process_touch;
  touch->task = task;

  // Initialize I2C configuration
  grid_esp32_touch_init_i2c(touch, i2c_port, sda_gpio, scl_gpio, i2c_freq_hz);

  // Initialize RST and INT/CHG GPIOs
  grid_esp32_touch_init_gpio(touch, reset, change);

  // Perform reset sequence
  if (!grid_esp32_touch_reset(touch)) {

    // Ignore return status, as success cannot be reliably detected due to some
    // bootloader versions not asserting the CHG line after bootloading
    ets_printf("grid_esp32_touch_init: CHG was high after reset\n");
  }

  // Read information block
  if (!grid_esp32_touch_read_info_block(touch)) {
    return false;
  }

  // Initialize debug diagnostics
  mxt_debug_init(&grid_esp32_touch_state.data);

  // Allocate message buffer
  touch->msg_buf = grid_platform_allocate_volatile(touch->data.T5_msg_size * touch->data.max_reportid);
  assert(touch->msg_buf);

  // Set up per-pin interrupt for CHG
  ESP_ERROR_CHECK(gpio_install_isr_service(0));
  ESP_ERROR_CHECK(gpio_set_intr_type(change, GPIO_INTR_NEGEDGE));
  ESP_ERROR_CHECK(gpio_isr_handler_add(change, mxt_chg_isr, touch));
  ESP_ERROR_CHECK(gpio_intr_enable(change));

  // Start communications with the device
  mxt_chg_isr(touch);

  return true;
}

void grid_esp32_touch_update(struct grid_esp32_touch_model* touch) {

  struct firmware fw = (struct firmware){
      .size = mxt144u_cfg_raw_len,
      .data = mxt144u_cfg_raw,
  };

  mxt_update_cfg(&touch->data, &fw);
}

static void grid_esp32_touch_proc_t6(struct grid_esp32_touch_model* touch, uint8_t* msg) {

  struct mxt_data* data = &touch->data;

  uint8_t status = msg[1];
  uint32_t crc = msg[2] | (msg[3] << 8) | (msg[4] << 16);

  if (crc != data->config_crc) {
    data->config_crc = crc;
  }

  data->crc_completion = 1;

  if (status & MXT_T6_STATUS_RESET) {
    data->reset_completion = 1;
  }

  if (status != data->T6_status) {

    ets_printf("T6 status 0x%02X%s%s%s%s%s%s%s\n", status, status == 0 ? " OK" : "", status & MXT_T6_STATUS_RESET ? " RESET" : "", status & MXT_T6_STATUS_OFL ? " OFL" : "",
               status & MXT_T6_STATUS_SIGERR ? " SIGERR" : "", status & MXT_T6_STATUS_CAL ? " CAL" : "", status & MXT_T6_STATUS_CFGERR ? " CFGERR" : "",
               status & MXT_T6_STATUS_COMSERR ? " COMSERR" : "");
  }

  data->T6_status = status;
}

static void grid_esp32_touch_proc_t100(struct grid_esp32_touch_model* touch, uint8_t* msg) {

  int id = msg[0] - touch->data.T100_rid_min - 2;

  // Ignore SCRSTATUS event
  if (id < 0) {
    return;
  }

  uint8_t status = msg[1];
  uint16_t x = msg[2] + (msg[3] << 8);
  uint16_t y = msg[4] + (msg[5] << 8);
  uint8_t distance = 0;

  uint8_t type = (status & MXT_T100_TYPE_MASK) >> 4;

  switch (type) {
  case MXT_T100_TYPE_FINGER:
  case MXT_T100_TYPE_PASSIVE_STYLUS:
  case MXT_T100_TYPE_HOVERING_FINGER:
  case MXT_T100_TYPE_GLOVE: {

    struct touchinfo_t info = (struct touchinfo_t){
        .id = id,
        .event = status & MXT_T100_EVENT_MASK,
        .x = x,
        .y = y,
    };
    ets_printf("i %d e %d x %4d y %4d\n", info.id, info.event, info.x, info.y);
    touch->process_touch(&info);

  } break;
  case MXT_T100_TYPE_LARGE_TOUCH:
    break;
  default:
    return;
  }
}

static int grid_esp32_touch_proc_message(struct grid_esp32_touch_model* touch, uint8_t* msg) {

  uint8_t report_id = msg[0];

  if (report_id == MXT_RPTID_NOMSG) {
    return 0;
  }

  struct mxt_data* data = &touch->data;

  if (report_id == data->T6_reportid) {
    grid_esp32_touch_proc_t6(touch, msg);
  } else if (report_id >= data->T100_rid_min && report_id <= data->T100_rid_max) {
    grid_esp32_touch_proc_t100(touch, msg);
  }

  return 1;
}

void grid_esp32_touch_process_msgs(struct grid_esp32_touch_model* touch) {

  esp_err_t err;

  struct mxt_data* data = &touch->data;

  // Read the number of messages from T44
  uint8_t count;
  err = mxt_read_reg(touch->data.dev_hndl, data->T44_address, 1, &count);
  if (err != ESP_OK) {
    return;
  }

  if (!count) {
    return;
  }

  // Read messages from T5
  err = mxt_read_reg(touch->data.dev_hndl, data->T5_address, data->T5_msg_size * count, touch->msg_buf);
  if (err != ESP_OK) {
    return;
  }

  // Process messages
  for (uint8_t i = 0; i < count; ++i) {
    grid_esp32_touch_proc_message(touch, touch->msg_buf + i * data->T5_msg_size);
  }

  return;
}

static esp_err_t mxt_read_diagnostic_debug(struct mxt_data* data, uint8_t mode) {

  esp_err_t err;

  struct mxt_dbg* dbg = &data->dbg;
  int retries = 0;
  uint8_t cmd = mode;
  uint8_t cmd_poll;

  for (int page = 0; page < MXT_DBG_PAGES_MIN; ++page) {

    struct t37_debug* p = &dbg->t37_buf[0] + page;

    err = mxt_write_reg(data->dev_hndl, dbg->diag_cmd_address, 1, &cmd);
    if (err != ESP_OK) {
      return err;
    }

    int timeout_counter = 0;

    do {

      vTaskDelay(pdMS_TO_TICKS(20));
      err = mxt_read_reg(data->dev_hndl, dbg->diag_cmd_address, 1, &cmd_poll);
      if (err != ESP_OK) {
        return err;
      }

    } while (cmd_poll && timeout_counter++ <= 100);

    if (timeout_counter > 100) {
      return ESP_FAIL;
    }

    err = mxt_read_reg(data->dev_hndl, dbg->T37_address, sizeof(struct t37_debug), (uint8_t*)p);
    if (err != ESP_OK) {
      return err;
    }

    if (p->mode != mode || p->page != page) {
      ets_printf("T37 page mismatch\n");
      return ESP_FAIL;
    }

    // For remaining pages, write PAGEUP rather than mode
    cmd = MXT_DIAGNOSTIC_PAGEUP;
  }

  return ESP_OK;
}

static void mxt_display_debug_pages(struct mxt_dbg* dbg) {

  assert(MXT_XLINES == MXT_YLINES);

  int values = 0;

  for (int page = 0; page < MXT_DBG_PAGES_MIN; ++page) {

    struct t37_debug* p = &dbg->t37_buf[0] + page;

    for (int j = 0; j < MXT_DIAGNOSTIC_SIZE; j += sizeof(uint16_t)) {

      int16_t val = p->data[j + 1] << 8 | p->data[j];
      ets_printf("%s%5d ", (val > 0) - (val < 0) ? "+" : "-", abs(val));
      ets_delay_us(50);

      if (++values >= dbg->t37_nodes) {
        break;
      }

      if (values % MXT_XLINES == 0) {
        ets_printf("\n");
      }
    }
  }

  ets_printf("\n");
}

void grid_esp32_utask_touch_t37(struct grid_utask_timer* timer) {

  if (!grid_utask_timer_elapsed(timer)) {
    return;
  }

  esp_err_t err = mxt_read_diagnostic_debug(&grid_esp32_touch_state.data, MXT_DIAGNOSTIC_REFS);
  if (err != ESP_OK) {
    ets_printf("mxt_read_diagnostic_debug returned %d\n", err);
  }

  mxt_display_debug_pages(&grid_esp32_touch_state.data.dbg);
  ets_printf("\n");
}
