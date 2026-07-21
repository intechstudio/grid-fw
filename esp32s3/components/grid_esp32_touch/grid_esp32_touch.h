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

#include "grid_utask.h"

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

struct mxt_info {
  uint8_t family_id;
  uint8_t variant_id;
  uint8_t version;
  uint8_t build;
  uint8_t matrix_xsize;
  uint8_t matrix_ysize;
  uint8_t object_num;
};

#define MXT_XLINES 12
#define MXT_YLINES 12
#define MXT_XYNODES (MXT_XLINES * MXT_YLINES)
#define MXT_DIAGNOSTIC_SIZE 128

struct t37_debug {
  uint8_t mode;
  uint8_t page;
  uint8_t data[MXT_DIAGNOSTIC_SIZE];
};

// Minimum number of pages to accommodate reference and delta debug modes
#define MXT_DBG_PAGES_MIN (((MXT_XYNODES * 2) / MXT_DIAGNOSTIC_SIZE) + 1)

struct mxt_dbg {
  uint16_t T37_address;
  uint16_t diag_cmd_address;
  struct t37_debug t37_buf[MXT_DBG_PAGES_MIN];
  unsigned int t37_pages;
  unsigned int t37_nodes;
};

struct mxt_data {

  i2c_master_dev_handle_t dev_hndl;

  uint8_t* raw_info_block;

  struct mxt_info info;
  uint32_t info_crc;

  struct mxt_object* object_table;

  uint16_t mem_size;

  uint32_t config_crc;
  uint8_t crc_completion;

  uint8_t reset_completion;

  uint8_t max_reportid;

  uint16_t T5_address;
  uint8_t T5_msg_size;
  uint8_t T6_reportid;
  uint16_t T6_address;
  uint16_t T7_address;
  uint16_t T44_address;
  uint16_t T71_address;
  uint8_t T100_rid_min;
  uint8_t T100_rid_max;

  uint8_t T6_status;

  struct mxt_dbg dbg;
};

struct grid_esp32_touch_model {

  gpio_num_t rst;
  gpio_num_t chg;

  i2c_master_bus_config_t bus_conf;
  i2c_master_bus_handle_t bus_hndl;

  grid_process_touch_t process_touch;

  TaskHandle_t task;

  struct mxt_data data;

  uint8_t* msg_buf;
};

extern struct grid_esp32_touch_model grid_esp32_touch_state;

bool grid_esp32_touch_init(struct grid_esp32_touch_model* touch, i2c_port_t i2c_port, gpio_num_t scl_gpio, gpio_num_t sda_gpio, gpio_num_t reset_gpio, gpio_num_t int_gpio, uint32_t i2c_freq_hz,
                           grid_process_touch_t process_touch, TaskHandle_t task);

void grid_esp32_touch_update(struct grid_esp32_touch_model* touch);

void grid_esp32_touch_process_msgs(struct grid_esp32_touch_model* touch);

void grid_esp32_utask_touch_t37(struct grid_utask_timer* timer);

#ifdef __cplusplus
}
#endif
