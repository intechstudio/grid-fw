/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "grid_cal.h"
#include "grid_config.h"
#include "grid_sys.h"
#include "grid_ui.h"

#include "grid_esp32_adc.h"
#include "grid_esp32_lcd.h"

void grid_esp32_module_tek1_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_esp32_adc_model* adc, struct grid_config_model* conf, struct grid_cal_model* cal,
                                 struct grid_esp32_lcd_model* lcds);

#ifdef __cplusplus
}
#endif
