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

// Initialize NCM networking - call after USB init
void grid_platform_ncm_init(void);

// Service lwIP timers - call this periodically from main loop
void grid_platform_ncm_service(void);

#ifdef __cplusplus
}
#endif
