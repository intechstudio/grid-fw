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

#include "driver/spi_master.h"

#include "../../grid_common/grid_module.h"


#include "rom/ets_sys.h" // For ets_printf


#ifdef __cplusplus
extern "C" {
#endif

static esp_err_t ret;
static spi_device_handle_t spi;
static spi_transaction_t t;

static void IRAM_ATTR my_post_setup_cb(spi_transaction_t *trans);
static void IRAM_ATTR  my_post_trans_cb(spi_transaction_t *trans);

void grid_esp32_module_ef44_task(void *arg);



#ifdef __cplusplus
}
#endif
