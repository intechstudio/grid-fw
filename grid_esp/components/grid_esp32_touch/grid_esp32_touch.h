/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "driver/i2c.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*grid_process_touch_t)(void);

struct grid_esp32_touch_model {
  i2c_port_t i2c_port;
  gpio_num_t scl_gpio;
  gpio_num_t sda_gpio;
  gpio_num_t reset_gpio;
  gpio_num_t int_gpio;
  uint32_t i2c_freq_hz;
  grid_process_touch_t process_touch;
};

extern DRAM_ATTR struct grid_esp32_touch_model grid_esp32_touch_state;

void grid_esp32_touch_init(struct grid_esp32_touch_model* touch, i2c_port_t i2c_port, gpio_num_t scl_gpio, gpio_num_t sda_gpio, gpio_num_t reset_gpio, gpio_num_t int_gpio, uint32_t i2c_freq_hz,
                           grid_process_touch_t process_touch);

void grid_esp32_touch_scan(struct grid_esp32_touch_model* touch);

#ifdef __cplusplus
}
#endif
