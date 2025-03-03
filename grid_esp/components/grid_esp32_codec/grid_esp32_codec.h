/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>

#include "esp_check.h"

#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"


#include "rom/ets_sys.h" // For ets_printf

#include "grid_esp32_pins.h"

#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

extern struct grid_esp32_codec_model grid_esp32_codec_state;

struct grid_esp32_codec_model {

  uint8_t foo;
};

void grid_esp32_codec_init(void);
void grid_esp32_codec_deinit(void);
void grid_esp32_codec_write(void);

void grid_esp32_codec_enable(void);
void grid_esp32_codec_disable(void);

#ifdef __cplusplus
}
#endif
