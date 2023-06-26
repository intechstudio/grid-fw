/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>


#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_check.h"


#include "../../grid_common/grid_ain.h"
#include "../../grid_common/grid_ui.h"

#include "driver/gpio.h"
#include "grid_esp32_pins.h"
#include "grid_esp32_adc.h"

#include "../../grid_common/grid_module.h"


#include "rom/ets_sys.h" // For ets_printf

#include "esp_rom_sys.h"


#include "grid_esp32_adc.h"
#include "grid_esp32_encoder.h"

#include "../../grid_common/grid_led.h"

#ifdef __cplusplus
extern "C" {
#endif

void grid_esp32_module_pbf4_task(void *arg);



#ifdef __cplusplus
}
#endif
