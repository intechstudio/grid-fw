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

#include "driver/spi_master.h"

#include "esp_rom_gpio.h"
#include "hal/gpio_ll.h"

#include "rom/ets_sys.h" // For ets_printf

#include "grid_esp32_pins.h"

#ifdef __cplusplus
extern "C" {
#endif



#define SPI_ENCODER_HOST    SPI3_HOST
#define GRID_ESP32_ENCODER_BUFFER_SIZE 14



struct grid_esp32_encoder_model
{

    spi_host_device_t spi_host;
    spi_device_handle_t spi_device_handle;
    spi_transaction_t transaction;

    uint8_t* rx_buffer_previous;
    uint8_t* rx_buffer;
    uint8_t* tx_buffer;

};

extern struct grid_esp32_encoder_model grid_esp32_encoder_state;

void grid_esp32_encoder_pins_init(void);
void grid_esp32_encoder_latch_data(void);

void grid_esp32_encoder_spi_init(struct grid_esp32_encoder_model* encoder, void (*post_setup_cb)(spi_transaction_t*), void (*post_trans_cb)(spi_transaction_t*));
void grid_esp32_encoder_init(struct grid_esp32_encoder_model* encoder, void (*post_setup_cb)(spi_transaction_t*), void (*post_trans_cb)(spi_transaction_t*));

void grid_esp32_encoder_spi_start_transfer(struct grid_esp32_encoder_model* encoder);

#ifdef __cplusplus
}
#endif
