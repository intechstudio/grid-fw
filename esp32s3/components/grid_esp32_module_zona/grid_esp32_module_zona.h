/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "grid_sys.h"
#include "grid_ui.h"

#include "freertos/FreeRTOS.h"

void grid_esp32_module_zona_init(struct grid_sys_model* sys, struct grid_ui_model* ui, TaskHandle_t touch_task);

void grid_esp32_module_zona_update_task(void* arg);

#ifdef __cplusplus
}
#endif
