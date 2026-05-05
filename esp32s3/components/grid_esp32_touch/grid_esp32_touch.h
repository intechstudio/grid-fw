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
#include "driver/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __TOUCHINFO_STRUCT__
#define __TOUCHINFO_STRUCT__
typedef struct _fttouchinfo {
  int count;          // number of pressed keys
  uint32_t key_state; // T15 key bitmap (1 bit per key, up to 32 keys)
  uint16_t x[5], y[5];
  uint8_t pressure[5], area[5];
} TOUCHINFO;
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

  uint16_t t5_addr;
  uint8_t t5_size;
  uint16_t t44_addr;
  uint8_t t100_first_report_id;

  volatile bool pending;
};

extern struct grid_esp32_touch_model grid_esp32_touch_state;

void grid_esp32_touch_init(struct grid_esp32_touch_model* touch, i2c_port_t i2c_port, gpio_num_t scl_gpio, gpio_num_t sda_gpio, gpio_num_t reset_gpio, gpio_num_t int_gpio, uint32_t i2c_freq_hz,
                           grid_process_touch_t process_touch);

void grid_esp32_touch_scan(struct grid_esp32_touch_model* touch);

int grid_esp32_touch_get_samples(struct grid_esp32_touch_model* touch, TOUCHINFO* pTI);

#ifdef __cplusplus
}
#endif
