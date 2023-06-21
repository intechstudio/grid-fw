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

#include "freertos/ringbuf.h"
#include "freertos/semphr.h"
#include "esp_heap_caps.h"


#ifdef __cplusplus
extern "C" {
#endif

#define GRID_ESP32_ADC_PROCESS_TASK_DELAY_MS 2

#define ADC_CONVERSION_FRAME_SIZE         64*SOC_ADC_DIGI_DATA_BYTES_PER_CONV
#define ADC_BUFFER_SIZE                   ADC_CONVERSION_FRAME_SIZE*4


#define BUFFER_SIZE     4*25      //32-bit aligned size
#define BUFFER_TYPE     RINGBUF_TYPE_ALLOWSPLIT

struct grid_esp32_adc_model
{
    adc_continuous_handle_t adc_handle;
    uint8_t adc_interrupt_state;
    uint8_t* adc_result_buffer;
    uint8_t mux_index;
    uint8_t mux_overflow;
    StaticRingbuffer_t *buffer_struct;
    uint8_t *buffer_storage;
    RingbufHandle_t ringbuffer_handle;
    SemaphoreHandle_t nvm_semaphore;
};



struct grid_esp32_adc_preprocessor {

    uint64_t sum; 
    uint16_t count; 
    uint16_t average; 
    uint8_t channel;

};



struct grid_esp32_adc_result {

    uint8_t channel;
    uint8_t mux_state;
    uint16_t value;

};

extern StaticRingbuffer_t *adc_buffer_struct;
extern uint8_t *adc_buffer_storage;

//Create a ring buffer with manually allocated memory
extern RingbufHandle_t adc_buffer_handle;



extern struct grid_esp32_adc_model DRAM_ATTR grid_esp32_adc_state;

bool IRAM_ATTR grid_esp32_adc_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data);

void grid_esp32_adc_init(struct grid_esp32_adc_model* adc, SemaphoreHandle_t nvm_semaphore);
void grid_esp32_adc_register_callback(struct grid_esp32_adc_model* adc, void (*callback)(adc_continuous_handle_t, const adc_continuous_evt_data_t*, void*));


void continuous_adc_init(adc_continuous_handle_t* out_handle);

void grid_esp32_adc_mux_init(struct grid_esp32_adc_model* adc, uint8_t mux_overflow);

void IRAM_ATTR grid_esp32_adc_mux_increment(struct grid_esp32_adc_model* adc);
void IRAM_ATTR grid_esp32_adc_mux_update(struct grid_esp32_adc_model* adc);
uint8_t IRAM_ATTR grid_esp32_adc_mux_get_index(struct grid_esp32_adc_model* adc);

void grid_esp32_adc_start(struct grid_esp32_adc_model* adc);
void grid_esp32_adc_stop(struct grid_esp32_adc_model* adc);


#ifdef __cplusplus
}
#endif
