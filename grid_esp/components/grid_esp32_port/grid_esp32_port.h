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

uint8_t grid_platform_send_grid_message(uint8_t direction, char* buffer, uint16_t length);
void grid_platform_sync1_pulse_send(void);
void grid_esp32_port_task(void* arg);

#ifdef __cplusplus
}
#endif
