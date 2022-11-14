/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>
#include "driver/rmt_encoder.h"

#include "driver/gpio.h"
#include "esp_check.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Type of led strip encoder configuration
 */
#define GRID_ESP32_PINS_HWCFG_SHIFT 10
#define GRID_ESP32_PINS_HWCFG_CLOCK 11
#define GRID_ESP32_PINS_HWCFG_DATA  12



#ifdef __cplusplus
}
#endif
