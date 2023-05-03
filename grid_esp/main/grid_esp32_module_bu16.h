/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_check.h"


#include "../../grid_common/grid_ain.h"
#include "../../grid_common/grid_ui.h"

#include "driver/gpio.h"
#include "grid_esp32_pins.h"

#include "../../grid_common/grid_module.h"

#ifdef __cplusplus
extern "C" {
#endif

void grid_esp32_module_bu16_task(void *arg);



#ifdef __cplusplus
}
#endif
