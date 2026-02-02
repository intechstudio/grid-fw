/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Type of led strip encoder configuration
 */
#define GRID_ESP32_PINS_HWCFG_SHIFT 10
#define GRID_ESP32_PINS_HWCFG_CLOCK 11
#define GRID_ESP32_PINS_HWCFG_DATA 12

#define GRID_ESP32_PINS_RP_SWDIO 13
#define GRID_ESP32_PINS_RP_SWCLK 14
#define GRID_ESP32_PINS_RP_CLOCK 15

#define GRID_ESP32_PINS_RP_MOSI 8
#define GRID_ESP32_PINS_RP_MISO 6
#define GRID_ESP32_PINS_RP_SCLK 9
#define GRID_ESP32_PINS_RP_CS 7
#define GRID_ESP32_PINS_RP_INTERRUPT -1

#define GRID_ESP32_PINS_MUX_0_A 37
#define GRID_ESP32_PINS_MUX_0_B 36
#define GRID_ESP32_PINS_MUX_0_C 35

#define GRID_ESP32_PINS_MUX_0_Z 18

#define GRID_ESP32_PINS_MUX_1_A 5
#define GRID_ESP32_PINS_MUX_1_B 4
#define GRID_ESP32_PINS_MUX_1_C 3

#define GRID_ESP32_PINS_MUX_1_Z 2

#define GRID_ESP32_PINS_MAPMODE 38

#ifdef __cplusplus
}
#endif
