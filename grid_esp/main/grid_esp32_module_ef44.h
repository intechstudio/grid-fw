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

#include "driver/spi_master.h"

#include "../../grid_common/grid_module.h"


#include "rom/ets_sys.h" // For ets_printf

#include "esp_rom_sys.h"

#include "esp_rom_gpio.h"
#include "hal/gpio_ll.h"


#include "grid_esp32_adc.h"

#include "../../grid_common/grid_led.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SPI_ENCODER_HOST    SPI3_HOST

static esp_err_t ret;
static spi_device_handle_t spi;
static spi_transaction_t t;

static void IRAM_ATTR my_post_setup_cb(spi_transaction_t *trans);
static void IRAM_ATTR my_post_trans_cb(spi_transaction_t *trans);

void grid_esp32_module_ef44_task(void *arg);

extern gpio_dev_t GPIO;

#ifdef __cplusplus
}
#endif
