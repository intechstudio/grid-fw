/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>

#include "esp_check.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdlib.h>
#include <string.h>

#include "grid_led.h"
#include "grid_utask.h"

#include "driver/gpio.h"

#include "driver/rmt_tx.h"
#include "led_strip_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif

void grid_esp32_led_start(uint8_t led_gpio);
void grid_esp32_utask_led(struct grid_utask_timer* timer);

#ifdef __cplusplus
}
#endif
