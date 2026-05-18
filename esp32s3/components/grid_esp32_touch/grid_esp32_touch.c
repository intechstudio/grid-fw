/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_touch.h"

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/ets_sys.h"

#define MXT_I2C_ADDR 0x4A

#define MXT_ID_BLOCK_SIZE 7
#define MXT_OBJ_ENTRY_SIZE 6

#define MXT_OBJ_T5 5
#define MXT_OBJ_T44 44
#define MXT_OBJ_T100 100

DRAM_ATTR struct grid_esp32_touch_model grid_esp32_touch_state;

static struct grid_esp32_touch_model* s_touch_ptr = NULL;

static void IRAM_ATTR mxt_chg_isr(void* arg) { s_touch_ptr->pending = true; }

static esp_err_t mxt_write_addr(i2c_port_t port, uint16_t addr) {
  uint8_t buf[2] = {(uint8_t)(addr & 0xFF), (uint8_t)(addr >> 8)};
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (MXT_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write(cmd, buf, 2, true);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(100));
  i2c_cmd_link_delete(cmd);
  return ret;
}

static esp_err_t mxt_read_block(i2c_port_t port, uint16_t addr, uint8_t* dest, size_t len) {
  esp_err_t ret = mxt_write_addr(port, addr);
  if (ret != ESP_OK)
    return ret;

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (MXT_I2C_ADDR << 1) | I2C_MASTER_READ, true);
  if (len > 1) {
    i2c_master_read(cmd, dest, len - 1, I2C_MASTER_ACK);
  }
  i2c_master_read_byte(cmd, dest + len - 1, I2C_MASTER_NACK);
  i2c_master_stop(cmd);
  ret = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(100));
  i2c_cmd_link_delete(cmd);
  return ret;
}

void grid_esp32_touch_init(struct grid_esp32_touch_model* touch, i2c_port_t i2c_port, gpio_num_t scl_gpio, gpio_num_t sda_gpio, gpio_num_t reset_gpio, gpio_num_t int_gpio, uint32_t i2c_freq_hz,
                           grid_process_touch_t process_touch) {

  touch->i2c_port = i2c_port;
  touch->scl_gpio = scl_gpio;
  touch->sda_gpio = sda_gpio;
  touch->reset_gpio = reset_gpio;
  touch->int_gpio = int_gpio;
  touch->i2c_freq_hz = i2c_freq_hz;
  touch->process_touch = process_touch;
  touch->t5_addr = 0;
  touch->t5_size = 0;
  touch->t44_addr = 0;
  touch->t100_first_report_id = 0;

  // I2C master
  i2c_config_t conf = {0};
  conf.mode = I2C_MODE_MASTER;
  conf.sda_io_num = sda_gpio;
  conf.scl_io_num = scl_gpio;
  conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
  conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
  conf.master.clk_speed = i2c_freq_hz;
  i2c_param_config(i2c_port, &conf);
  esp_err_t ret = i2c_driver_install(i2c_port, I2C_MODE_MASTER, 0, 0, 0);
  if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
    ets_printf("MXT i2c_driver_install failed: %d\r\n", ret);
    return;
  }

  // RST as output, INT/CHG as input with pull-up
  gpio_config_t io_conf = {0};
  io_conf.pin_bit_mask = (1ULL << reset_gpio);
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.intr_type = GPIO_INTR_DISABLE;
  gpio_config(&io_conf);

  io_conf.pin_bit_mask = (1ULL << int_gpio);
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  gpio_config(&io_conf);

  // Reset sequence — hold RST low 100ms, then wait 300ms for chip to initialise
  gpio_set_level(reset_gpio, 0);
  vTaskDelay(pdMS_TO_TICKS(100));
  gpio_set_level(reset_gpio, 1);
  vTaskDelay(pdMS_TO_TICKS(300));

  // Check CHG state as a diagnostic (not a hard gate)
  ets_printf("MXT CHG after reset: %s\r\n", gpio_get_level(int_gpio) == 0 ? "LOW (ready)" : "HIGH");

  // Scan I2C bus to confirm device is visible
  grid_esp32_touch_scan(touch);

  // Read ID block at address 0
  uint8_t id[MXT_ID_BLOCK_SIZE];
  ret = mxt_read_block(i2c_port, 0, id, MXT_ID_BLOCK_SIZE);
  if (ret != ESP_OK) {
    ets_printf("MXT ID block read failed: %d\r\n", ret);
    return;
  }
  ets_printf("MXT family=0x%02X variant=0x%02X fw=%d.%d.%02X matrix=%dx%d objects=%d\r\n", id[0], id[1], id[2] >> 4, id[2] & 0x0F, id[3], id[4], id[5], id[6]);

  // Parse object table to find T5, T44, T100
  uint8_t num_objects = id[6];
  uint16_t table_ptr = MXT_ID_BLOCK_SIZE;
  uint8_t report_id = 1;

  for (uint8_t i = 0; i < num_objects; i++) {
    uint8_t entry[MXT_OBJ_ENTRY_SIZE];
    ret = mxt_read_block(i2c_port, table_ptr, entry, MXT_OBJ_ENTRY_SIZE);
    if (ret != ESP_OK) {
      ets_printf("MXT object table read failed at entry %d\r\n", i);
      return;
    }
    table_ptr += MXT_OBJ_ENTRY_SIZE;

    uint8_t obj_type = entry[0];
    uint16_t obj_addr = entry[1] | ((uint16_t)entry[2] << 8);
    uint8_t num_inst = entry[4]; // stored as instances-1
    uint8_t num_rids = entry[5];

    if (obj_type == MXT_OBJ_T5) {
      touch->t5_addr = obj_addr;
      touch->t5_size = entry[3]; // read size-1 bytes, skipping trailing checksum
    } else if (obj_type == MXT_OBJ_T44) {
      touch->t44_addr = obj_addr;
    } else if (obj_type == MXT_OBJ_T100) {
      touch->t100_first_report_id = report_id;
    }

    if (num_rids > 0) {
      report_id += (num_inst + 1) * num_rids;
    }
  }

  if (touch->t5_addr && touch->t44_addr && touch->t100_first_report_id) {
    ets_printf("MXT init OK: T5@0x%04X(sz=%d) T44@0x%04X T100 first_rid=%d\r\n", touch->t5_addr, touch->t5_size, touch->t44_addr, touch->t100_first_report_id);

  } else {
    ets_printf("MXT init FAILED: T5=0x%04X T44=0x%04X T100_rid=%d\r\n", touch->t5_addr, touch->t44_addr, touch->t100_first_report_id);
    return;
  }

  // Interrupt on falling edge of CHG — sets pending flag, serviced in main loop
  s_touch_ptr = touch;
  esp_err_t isr_ret;
  isr_ret = gpio_install_isr_service(0);
  ets_printf("MXT gpio_install_isr_service: %d\r\n", isr_ret);
  isr_ret = gpio_set_intr_type(int_gpio, GPIO_INTR_NEGEDGE);
  ets_printf("MXT gpio_set_intr_type: %d\r\n", isr_ret);
  isr_ret = gpio_isr_handler_add(int_gpio, mxt_chg_isr, NULL);
  ets_printf("MXT gpio_isr_handler_add: %d\r\n", isr_ret);
  isr_ret = gpio_intr_enable(int_gpio);
  ets_printf("MXT gpio_intr_enable: %d\r\n", isr_ret);
  touch->pending = true;
}

void grid_esp32_touch_scan(struct grid_esp32_touch_model* touch) {

  ets_printf("I2C scan (SCL=GPIO%d SDA=GPIO%d):\r\n", touch->scl_gpio, touch->sda_gpio);
  int found = 0;
  for (uint8_t addr = 0x08; addr < 0x78; addr++) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(touch->i2c_port, cmd, pdMS_TO_TICKS(10));
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_OK) {
      ets_printf("  found device at 0x%02X\r\n", addr);
      found++;
    }
  }
  if (found == 0) {
    ets_printf("  no devices found\r\n");
  }
}

int grid_esp32_touch_get_samples(struct grid_esp32_touch_model* touch, TOUCHINFO* pTI) {
  if (!pTI)
    return 0;
  pTI->count = 0;

  if (!touch->t44_addr || !touch->t5_addr || !touch->t100_first_report_id) {
    return 0;
  }

  if (!touch->pending) {
    return 0;
  }
  touch->pending = false;

  uint8_t msg_count = 0;
  esp_err_t ret = mxt_read_block(touch->i2c_port, touch->t44_addr, &msg_count, 1);
  if (ret != ESP_OK || msg_count == 0) {
    return 0;
  }

  uint8_t msg[10];
  uint8_t finger_start = touch->t100_first_report_id + 2;

  for (uint8_t i = 0; i < msg_count; i++) {
    ret = mxt_read_block(touch->i2c_port, touch->t5_addr, msg, touch->t5_size);
    if (ret != ESP_OK)
      break;

    uint8_t report_id = msg[0];

    if (report_id >= finger_start && report_id < finger_start + 5) {
      uint8_t finger_idx = report_id - finger_start;
      uint8_t event = msg[1] & 0x0F;

      if (finger_idx + 1 > pTI->count) {
        pTI->count = finger_idx + 1;
      }
      pTI->x[finger_idx] = msg[2] | ((uint16_t)msg[3] << 8);
      pTI->y[finger_idx] = msg[4] | ((uint16_t)msg[5] << 8);

      if (event == 4 || event == 1) { // touch down or move
        pTI->area[finger_idx] = 50;
      } else if (event == 5) { // touch up
        pTI->area[finger_idx] = 0;
      }
    }
  }

  return (pTI->count > 0);
}
