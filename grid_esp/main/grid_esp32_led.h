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
#include "esp_log.h"


#include "../../grid_common/grid_led.h"

#include "driver/gpio.h"

#include "driver/rmt_tx.h"
#include "led_strip_encoder.h"


#ifdef __cplusplus
extern "C" {
#endif

void grid_esp32_led_task(void *arg);


#ifdef __cplusplus
}
#endif
