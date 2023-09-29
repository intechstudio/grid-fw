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

#include "../../grid_common/include/grid_ain.h"
#include "../../grid_common/include/grid_module.h"

#include "rom/ets_sys.h" // For ets_printf

#include "grid_esp32_adc.h"


#ifdef __cplusplus
extern "C" {
#endif


void grid_esp32_module_po16_task(void *arg);



#ifdef __cplusplus
}
#endif
