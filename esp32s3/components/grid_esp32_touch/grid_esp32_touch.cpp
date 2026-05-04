/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_touch.h"
#include "bb_captouch.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "rom/ets_sys.h"

DRAM_ATTR struct grid_esp32_touch_model grid_esp32_touch_state;

static struct grid_esp32_touch_model* touch_ptr = NULL;
static BBCapTouch bbc;

void grid_esp32_touch_init(struct grid_esp32_touch_model* touch, i2c_port_t i2c_port, gpio_num_t scl_gpio, gpio_num_t sda_gpio, gpio_num_t reset_gpio, gpio_num_t int_gpio, uint32_t i2c_freq_hz,
                           grid_process_touch_t process_touch) {

  touch_ptr = touch;
  touch->i2c_port = i2c_port;
  touch->scl_gpio = scl_gpio;
  touch->sda_gpio = sda_gpio;
  touch->reset_gpio = reset_gpio;
  touch->int_gpio = int_gpio;
  touch->i2c_freq_hz = i2c_freq_hz;
  touch->process_touch = process_touch;

  int rc = bbc.init(touch->sda_gpio, touch->scl_gpio, touch->reset_gpio, touch->int_gpio, touch->i2c_freq_hz);
  if (rc == CT_SUCCESS) {
    ets_printf("MXT144 init OK\r\n");
  } else {
    ets_printf("MXT144 init FAILED\r\n");
  }
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
  (void)touch;
  return bbc.getSamples(pTI);
}
