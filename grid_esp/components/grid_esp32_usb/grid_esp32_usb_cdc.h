/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize CDC subsystem (called from grid_esp32_usb_init)
void grid_esp32_usb_cdc_init(void);

// Platform API
int32_t grid_platform_usb_serial_ready(void);
int32_t grid_platform_usb_serial_write(char* buffer, uint32_t length);

#ifdef __cplusplus
}
#endif
