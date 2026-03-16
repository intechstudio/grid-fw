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

static void IRAM_ATTR grid_esp32_touch_int_handler(void* arg) {
  struct grid_esp32_touch_model* touch = (struct grid_esp32_touch_model*)arg;
  touch->int_pending = true;
  ets_printf("INT\r\n");
}

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

  // Apply pull-ups to all sensor pins before reset — no external resistors on this board.
  // Must happen before the reset pulse so SDA/SCL are not floating during sensor boot.
  gpio_reset_pin(touch->scl_gpio);
  gpio_pullup_en(touch->scl_gpio);
  gpio_reset_pin(touch->sda_gpio);
  gpio_pullup_en(touch->sda_gpio);
  gpio_reset_pin(touch->int_gpio);
  gpio_pullup_en(touch->int_gpio);

  // RESET: active-low reset pulse
  gpio_reset_pin(touch->reset_gpio);
  gpio_set_direction(touch->reset_gpio, GPIO_MODE_OUTPUT);
  gpio_set_level(touch->reset_gpio, 0);  // assert reset
  vTaskDelay(pdMS_TO_TICKS(10));
  gpio_set_level(touch->reset_gpio, 1);  // deassert reset
  vTaskDelay(pdMS_TO_TICKS(250));        // wait for sensor boot

  // INT: falling edge interrupt (pull-up already set above)
  gpio_set_direction(touch->int_gpio, GPIO_MODE_INPUT);
  gpio_set_intr_type(touch->int_gpio, GPIO_INTR_NEGEDGE);
  gpio_install_isr_service(0);
  gpio_isr_handler_add(touch->int_gpio, grid_esp32_touch_int_handler, touch);
  gpio_intr_enable(touch->int_gpio);

  // I2C + MXT144 init — i2c_param_config will reconfigure SCL/SDA with pull-ups enabled
  int rc = bbc.init(touch->sda_gpio, touch->scl_gpio, touch->i2c_freq_hz);
  if (rc == CT_SUCCESS) {
    ets_printf("MXT144 init OK\r\n");
    touch->int_pending = true; // drain any messages queued during init
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

void grid_esp32_touch_diagnostic(struct grid_esp32_touch_model* touch) {
  (void)touch;
  bbc.dumpDiagnostic();
}
