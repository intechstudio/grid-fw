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

void grid_esp32_module_xy_init(struct grid_sys_model* sys, struct grid_ui_model* ui);
void grid_esp32_module_xy_handle_touch(void);

#ifdef __cplusplus
}
#endif
