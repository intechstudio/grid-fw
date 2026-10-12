/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef GRID_ESP32_TOUCH_H
#define GRID_ESP32_TOUCH_H

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"

#include "grid_maxtouch.h"
#include "grid_ui_touch.h"
#include "grid_utask.h"

#ifdef __cplusplus
extern "C" {
#endif

struct grid_esp32_touch_model {

  gpio_num_t rst;
  gpio_num_t chg;

  i2c_master_bus_config_t bus_conf;
  i2c_master_bus_handle_t bus_hndl;

  TaskHandle_t task;

  struct mxt_data data;

  uint8_t* msg_buf;
};

extern struct grid_esp32_touch_model grid_esp32_touch_state;

bool grid_esp32_touch_init(struct grid_esp32_touch_model* touch, i2c_port_t i2c_port, gpio_num_t scl_gpio, gpio_num_t sda_gpio, gpio_num_t reset_gpio, gpio_num_t int_gpio, uint32_t i2c_freq_hz,
                           TaskHandle_t task);

void grid_esp32_touch_update(struct grid_esp32_touch_model* touch);

void grid_esp32_touch_process_msgs(struct mxt_data* data, uint8_t* msg_buf);

void grid_esp32_utask_touch_t37(struct grid_utask_timer* timer);

#ifdef __cplusplus
}
#endif

#endif /* GRID_ESP32_TOUCH_H */
