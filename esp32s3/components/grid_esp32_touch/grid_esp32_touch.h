/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"

#include "grid_ui_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*grid_process_touch_t)(struct touchinfo_t*);

struct mxt_object {
  uint8_t type;
  uint16_t start_addr;
  uint8_t size_minus_one;
  uint8_t inst_minus_one;
  uint8_t num_report_ids;
} __packed;

struct grid_esp32_touch_model {

  gpio_num_t rst;
  gpio_num_t chg;

  i2c_master_bus_config_t bus_conf;
  i2c_master_bus_handle_t bus_hndl;
  i2c_master_dev_handle_t dev_hndl;

  grid_process_touch_t process_touch;

  TaskHandle_t task;

  uint8_t max_reportid;
  uint16_t T5_start_addr;
  uint8_t T5_msg_size;
  uint16_t T44_start_addr;
  uint8_t T100_rid_min;
  uint8_t T100_rid_max;
  uint8_t num_touchids;

  uint8_t* msg_buf;
};

extern struct grid_esp32_touch_model grid_esp32_touch_state;

bool grid_esp32_touch_init(struct grid_esp32_touch_model* touch, i2c_port_t i2c_port, gpio_num_t scl_gpio, gpio_num_t sda_gpio, gpio_num_t reset_gpio, gpio_num_t int_gpio, uint32_t i2c_freq_hz,
                           grid_process_touch_t process_touch, TaskHandle_t task);

void grid_esp32_touch_process_msgs(struct grid_esp32_touch_model* touch);

#ifdef __cplusplus
}
#endif
