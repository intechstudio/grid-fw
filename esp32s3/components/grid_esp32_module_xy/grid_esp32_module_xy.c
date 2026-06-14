/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_module_xy.h"

#include <stdint.h>

#include "grid_sys.h"
#include "grid_ui.h"
#include "grid_ui_touch.h"

#include "grid_esp32_touch.h"

#define XY_I2C_PORT I2C_NUM_0
#define XY_I2C_SCL_GPIO 40
#define XY_I2C_SDA_GPIO 41
#define XY_I2C_FREQ_HZ 100000

#define XY_SENSOR_RESET_GPIO 39
#define XY_SENSOR_INT_GPIO 42

void grid_esp32_module_xy_process_touch(struct touchinfo_t* info) {

  struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, info->id);
  if (!ele) {
    return;
  }

  grid_ui_touch_store_input(grid_ui_touch_get_state(ele), *info);
}

void grid_esp32_module_xy_init(struct grid_sys_model* sys, struct grid_ui_model* ui, TaskHandle_t touch_task) {

  grid_esp32_touch_init(&grid_esp32_touch_state, XY_I2C_PORT, XY_I2C_SCL_GPIO, XY_I2C_SDA_GPIO, XY_SENSOR_RESET_GPIO, XY_SENSOR_INT_GPIO, XY_I2C_FREQ_HZ, grid_esp32_module_xy_process_touch,
                        touch_task);

  for (int i = 0; i < ui->element_list_length; ++i) {
    struct grid_ui_element* ele = &ui->element_list[i];
    if (ele->type == GRID_PARAMETER_ELEMENT_TOUCH) {
      grid_ui_touch_state_init(grid_ui_touch_get_state(ele), 10);
    }
  }

  grid_ui_bulk_start_with_state(ui, grid_ui_bulk_conf_read, 0, 0, NULL);
  grid_ui_bulk_flush(ui);
}
