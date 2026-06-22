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

#include "grid_platform.h"

#define MXT_I2C_ADDR 0x4A

#define MXT_OBJECT_START 0x07
#define MXT_OBJECT_SIZE 6
#define MXT_INFO_CHECKSUM_SIZE 3

#define MXT_GEN_MESSAGE_T5 5
#define MXT_SPT_MESSAGECOUNT_T44 44
#define MXT_TOUCH_MULTITOUCHSCREEN_T100 100

#define MXT_RPTID_NOMSG 0xff

#define MXT_T100_DETECT BIT(7)
#define MXT_T100_TYPE_MASK 0x70

enum t100_type {
  MXT_T100_TYPE_FINGER = 1,
  MXT_T100_TYPE_PASSIVE_STYLUS = 2,
  MXT_T100_TYPE_HOVERING_FINGER = 4,
  MXT_T100_TYPE_GLOVE = 5,
  MXT_T100_TYPE_LARGE_TOUCH = 6,
};

struct mxt_info {
  uint8_t family_id;
  uint8_t variant_id;
  uint8_t version;
  uint8_t build;
  uint8_t matrix_xsize;
  uint8_t matrix_ysize;
  uint8_t object_num;
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

static void grid_esp32_touch_init_gpio(struct grid_esp32_touch_model* touch, gpio_num_t reset, gpio_num_t change) {

  touch->rst = reset;

  gpio_config_t rst_cfg = {
      .pin_bit_mask = 1 << touch->rst,
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&rst_cfg));

  touch->chg = change;

  gpio_config_t chg_cfg = {
      .pin_bit_mask = 1 << touch->chg,
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
  vTaskDelay(pdMS_TO_TICKS(10));

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

  ESP_ERROR_CHECK(i2c_master_bus_add_device(touch->bus_hndl, &dev_cfg, &touch->dev_hndl));
}

static void grid_esp32_touch_parse_info_block(struct grid_esp32_touch_model* touch, uint8_t* iblk) {

  struct mxt_info* info = (struct mxt_info*)iblk;

  // Parse object table
  uint8_t reportid = 1;
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

      touch->T5_start_addr = object->start_addr;
      ets_printf("touch->T5_start_addr %d\n", touch->T5_start_addr);

      if (info->family_id == 0x80 && info->version < 0x20) {
        touch->T5_msg_size = mxt_obj_size(object);
      } else {
        touch->T5_msg_size = mxt_obj_size(object) - 1;
      }
      ets_printf("touch->T5_msg_size %d\n", touch->T5_msg_size);

    } break;
    case MXT_SPT_MESSAGECOUNT_T44: {

      touch->T44_start_addr = object->start_addr;
      ets_printf("touch->T44_start_addr %d\n", touch->T44_start_addr);

    } break;
    case MXT_TOUCH_MULTITOUCHSCREEN_T100: {

      touch->T100_rid_min = min_id;
      touch->T100_rid_max = max_id;

      ets_printf("touch->T100_rid_min %d\n", touch->T100_rid_min);
      ets_printf("touch->T100_rid_max %d\n", touch->T100_rid_max);

      // The first two report IDs are reserved
      touch->num_touchids = object->num_report_ids - 2;

      ets_printf("touch->num_touchids %d\n", touch->num_touchids);

    } break;
    }
  }

  // Store maximum reportid
  touch->max_reportid = reportid;
  ets_printf("touch->max_reportid %d\n", touch->max_reportid);
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

bool grid_esp32_touch_read_info_block(struct grid_esp32_touch_model* touch) {

  bool ret = false;

  uint8_t* iblk = NULL;

  esp_err_t err;

  // Read ID information block
  struct mxt_info info = {0};
  err = mxt_read_reg(touch->dev_hndl, 0, sizeof(struct mxt_info), (uint8_t*)&info);
  if (err != ESP_OK) {
    ets_printf("grid_esp32_touch_read_info_block: failed to read first 7 bytes\n");
    goto grid_esp32_touch_read_info_block_cleanup;
  }

  // Allocate information block
  size_t info_size = MXT_OBJECT_START + sizeof(struct mxt_object) * info.object_num + MXT_INFO_CHECKSUM_SIZE;
  iblk = malloc(info_size);
  if (!iblk) {
    ets_printf("grid_esp32_touch_read_info_block: failed allocate info block\n");
    goto grid_esp32_touch_read_info_block_cleanup;
  }

  // Read information block
  err = mxt_read_reg(touch->dev_hndl, 0, info_size, iblk);
  if (err != ESP_OK) {
    ets_printf("grid_esp32_touch_read_info_block: failed to read info block\n");
    goto grid_esp32_touch_read_info_block_cleanup;
  }

  // Extract checksum
  uint8_t* crc_ptr = &iblk[info_size - MXT_INFO_CHECKSUM_SIZE];
  uint32_t info_crc = crc_ptr[0] | crc_ptr[1] << 8 | crc_ptr[2] << 16;
  uint32_t calc_crc = mxt_calculate_crc(iblk, 0, info_size - MXT_INFO_CHECKSUM_SIZE);
  if (info_crc == 0 || info_crc != calc_crc) {
    ets_printf("grid_esp32_touch_read_info_block: crc mismatch\n");
    goto grid_esp32_touch_read_info_block_cleanup;
  }

  ets_printf("family %02X variant %02X firmware v%u.%u.%02X objects %u\n", info.family_id, info.variant_id, info.version >> 4, info.version & 0xf, info.build, info.object_num);

  // Parse information block
  grid_esp32_touch_parse_info_block(touch, iblk);

  ret = true;

grid_esp32_touch_read_info_block_cleanup:
  if (iblk) {
    free(iblk);
  }
  return ret;
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

  // Allocate message buffer
  touch->msg_buf = grid_platform_allocate_volatile(touch->T5_msg_size * touch->max_reportid);
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

static void grid_esp32_touch_proc_t100(struct grid_esp32_touch_model* touch, uint8_t* msg) {

  int id = msg[0] - touch->T100_rid_min - 2;

  // Ignore SCRSTATUS event
  if (id < 0) {
    return;
  }

  uint8_t status = msg[1];
  uint16_t x = msg[2] + (msg[3] << 8);
  uint16_t y = msg[4] + (msg[5] << 8);
  uint8_t distance = 0;

  uint8_t type = 0;
  if (status & MXT_T100_DETECT) {

    type = (status & MXT_T100_TYPE_MASK) >> 4;

    switch (type) {
    case MXT_T100_TYPE_FINGER:
    case MXT_T100_TYPE_PASSIVE_STYLUS:
    case MXT_T100_TYPE_HOVERING_FINGER:
    case MXT_T100_TYPE_GLOVE: {

      struct touchinfo_t info = (struct touchinfo_t){
          .id = id,
          .type = type,
          .x = x,
          .y = y,
      };
      touch->process_touch(&info);

    } break;
    case MXT_T100_TYPE_LARGE_TOUCH:
      break;
    default:
      return;
    }
  }
}

static int grid_esp32_touch_proc_message(struct grid_esp32_touch_model* touch, uint8_t* msg) {

  uint8_t report_id = msg[0];

  if (report_id == MXT_RPTID_NOMSG) {
    return 0;
  }

  if (report_id >= touch->T100_rid_min && report_id <= touch->T100_rid_max) {
    grid_esp32_touch_proc_t100(touch, msg);
  }

  return 1;
}

void grid_esp32_touch_process_msgs(struct grid_esp32_touch_model* touch) {

  esp_err_t err;

  uint32_t start = grid_platform_rtc_get_micros();
  // Read the number of messages from T44
  uint8_t count;
  err = mxt_read_reg(touch->dev_hndl, touch->T44_start_addr, 1, &count);
  if (err != ESP_OK) {
    return;
  }

  if (!count) {
    return;
  }

  // Read messages from T5
  err = mxt_read_reg(touch->dev_hndl, touch->T5_start_addr, touch->T5_msg_size * count, touch->msg_buf);
  if (err != ESP_OK) {
    return;
  }

  // Process messages
  for (uint8_t i = 0; i < count; ++i) {
    grid_esp32_touch_proc_message(touch, touch->msg_buf + i * touch->T5_msg_size);
  }

  return;
}
