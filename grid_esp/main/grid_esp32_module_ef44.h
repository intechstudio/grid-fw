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

#include "../../grid_common/grid_module.h"


#include "rom/ets_sys.h" // For ets_printf

#include "esp_rom_sys.h"


#include "grid_esp32_adc.h"
#include "grid_esp32_encoder.h"

#include "../../grid_common/grid_led.h"

#ifdef __cplusplus
extern "C" {
#endif



static void IRAM_ATTR my_post_setup_cb(spi_transaction_t *trans);
static void IRAM_ATTR my_post_trans_cb(spi_transaction_t *trans);

void grid_esp32_module_ef44_task(void *arg);

extern gpio_dev_t GPIO;

#ifdef __cplusplus
}
#endif
