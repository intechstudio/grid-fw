/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_module_xy.h"

#include <stdint.h>

#include "grid_sys.h"
#include "grid_ui.h"

#include "grid_esp32_touch.h"

#include "rom/ets_sys.h"

#define XY_I2C_PORT I2C_NUM_0
#define XY_I2C_SCL_GPIO 40
#define XY_I2C_SDA_GPIO 41
#define XY_I2C_FREQ_HZ 100000

#define XY_SENSOR_RESET_GPIO 39
#define XY_SENSOR_INT_GPIO 42

void grid_esp32_module_xy_poll_touch(void) {
  TOUCHINFO ti = {};
  int rc = grid_esp32_touch_get_samples(&grid_esp32_touch_state, &ti);
  ets_printf("touch rc=%d count=%d\r\n", rc, ti.count);
  for (int i = 0; i < ti.count; i++) {
    ets_printf("  [%d] x=%d y=%d area=%d\r\n", i, ti.x[i], ti.y[i], ti.area[i]);
  }
}

void grid_esp32_module_xy_init(struct grid_sys_model* sys, struct grid_ui_model* ui) {

  grid_esp32_touch_init(&grid_esp32_touch_state, XY_I2C_PORT, XY_I2C_SCL_GPIO, XY_I2C_SDA_GPIO, XY_SENSOR_RESET_GPIO, XY_SENSOR_INT_GPIO, XY_I2C_FREQ_HZ, NULL);

  grid_ui_bulk_start_with_state(ui, grid_ui_bulk_conf_read, 0, 0, NULL);
  grid_ui_bulk_flush(ui);
}
