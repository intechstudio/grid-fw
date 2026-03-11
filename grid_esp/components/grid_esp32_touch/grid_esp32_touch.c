/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_touch.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "rom/ets_sys.h"

DRAM_ATTR struct grid_esp32_touch_model grid_esp32_touch_state;

static struct grid_esp32_touch_model* touch_ptr = NULL;

static void IRAM_ATTR grid_esp32_touch_int_handler(void* arg) {
  struct grid_esp32_touch_model* touch = (struct grid_esp32_touch_model*)arg;
  ets_printf("OCTV touch INT\r\n");
  if (touch->process_touch) {
    touch->process_touch();
  }
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

  // RESET: drive high before gpio_reset_pin to avoid floating the pin
  gpio_set_direction(touch->reset_gpio, GPIO_MODE_OUTPUT);
  gpio_set_level(touch->reset_gpio, 1);
  gpio_reset_pin(touch->reset_gpio);
  gpio_set_direction(touch->reset_gpio, GPIO_MODE_OUTPUT);
  gpio_set_level(touch->reset_gpio, 1);

  vTaskDelay(pdMS_TO_TICKS(150));

  // INT: input with pull-up, falling edge interrupt (active low)
  gpio_reset_pin(touch->int_gpio);
  gpio_set_direction(touch->int_gpio, GPIO_MODE_INPUT);
  gpio_set_pull_mode(touch->int_gpio, GPIO_PULLUP_ONLY);
  gpio_set_intr_type(touch->int_gpio, GPIO_INTR_NEGEDGE);
  gpio_install_isr_service(0);
  gpio_isr_handler_add(touch->int_gpio, grid_esp32_touch_int_handler, touch);
  gpio_intr_enable(touch->int_gpio);

  // I2C master
  gpio_reset_pin(touch->scl_gpio);
  gpio_reset_pin(touch->sda_gpio);
  i2c_config_t conf = {
      .mode             = I2C_MODE_MASTER,
      .sda_io_num       = touch->sda_gpio,
      .scl_io_num       = touch->scl_gpio,
      .sda_pullup_en    = GPIO_PULLUP_ENABLE,
      .scl_pullup_en    = GPIO_PULLUP_ENABLE,
      .master.clk_speed = touch->i2c_freq_hz,
  };
  i2c_param_config(touch->i2c_port, &conf);
  i2c_driver_install(touch->i2c_port, I2C_MODE_MASTER, 0, 0, 0);
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
