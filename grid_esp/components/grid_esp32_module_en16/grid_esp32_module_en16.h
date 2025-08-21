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

#include "grid_esp32_encoder.h"

void grid_esp32_module_en16_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_esp32_encoder_model* enc);

#ifdef __cplusplus
}
#endif
