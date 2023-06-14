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

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#include "esp_rom_gpio.h"
#include "hal/gpio_ll.h"

#include "rom/ets_sys.h" // For ets_printf

#include "grid_esp32_pins.h"

#ifdef __cplusplus
extern "C" {
#endif


#define ADC_CONVERSION_FRAME_SIZE         64*SOC_ADC_DIGI_DATA_BYTES_PER_CONV
#define ADC_BUFFER_SIZE                   ADC_CONVERSION_FRAME_SIZE*4

struct grid_esp32_adc_model
{
    adc_continuous_handle_t adc_handle;

};

extern struct grid_esp32_adc_model DRAM_ATTR grid_esp32_adc_state;


static uint16_t DRAM_ATTR grid_esp32_adc_result_buffer[ADC_CONVERSION_FRAME_SIZE+4];

void grid_esp32_adc_init(struct grid_esp32_adc_model* adc);
void grid_esp32_adc_register_callback(struct grid_esp32_adc_model* adc, void (*callback)(adc_continuous_handle_t, const adc_continuous_evt_data_t*, void*));


void continuous_adc_init(adc_continuous_handle_t* out_handle);

void grid_esp32_adc_read(struct grid_esp32_adc_model* adc, uint16_t* channel_0_index, uint16_t* channel_1_index, uint16_t* channel_0_value, uint16_t* channel_1_value);
void grid_esp32_adc_mux_pins_init(void);
void IRAM_ATTR grid_esp32_adc_mux_update(uint8_t mux_index);

void grid_esp32_adc_start(struct grid_esp32_adc_model* adc);
void grid_esp32_adc_stop(struct grid_esp32_adc_model* adc);


#ifdef __cplusplus
}
#endif
