/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "grid_esp32_adc.h"

struct grid_esp32_adc_model DRAM_ATTR grid_esp32_adc_state;

void continuous_adc_init(adc_continuous_handle_t *out_handle)
{
    adc_continuous_handle_t handle = NULL;

    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = ADC_BUFFER_SIZE,
        .conv_frame_size = ADC_CONVERSION_FRAME_SIZE,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = SOC_ADC_SAMPLE_FREQ_THRES_HIGH/2,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
    };

    dig_cfg.pattern_num = 2;

    adc_digi_pattern_config_t adc_pattern[2] = {0};

    adc_pattern[0].atten = ADC_ATTEN_DB_11;
    adc_pattern[0].channel = ADC_CHANNEL_1 & 0x7;
    adc_pattern[0].unit = ADC_UNIT_1;
    adc_pattern[0].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;


    adc_pattern[1].atten = ADC_ATTEN_DB_11;
    adc_pattern[1].channel = ADC_CHANNEL_0 & 0x7;
    adc_pattern[1].unit = ADC_UNIT_1;
    adc_pattern[1].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;

    dig_cfg.adc_pattern = adc_pattern;
    ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));

    *out_handle = handle;
}



void grid_esp32_adc_mux_pins_init(struct grid_esp32_adc_model* adc){

    gpio_set_direction(GRID_ESP32_PINS_MUX_0_A, GPIO_MODE_OUTPUT);
    gpio_set_direction(GRID_ESP32_PINS_MUX_0_B, GPIO_MODE_OUTPUT);
    gpio_set_direction(GRID_ESP32_PINS_MUX_0_C, GPIO_MODE_OUTPUT);

    gpio_set_direction(GRID_ESP32_PINS_MUX_1_A, GPIO_MODE_OUTPUT);
    gpio_set_direction(GRID_ESP32_PINS_MUX_1_B, GPIO_MODE_OUTPUT);
    gpio_set_direction(GRID_ESP32_PINS_MUX_1_C, GPIO_MODE_OUTPUT);

    gpio_set_level(GRID_ESP32_PINS_MUX_0_A, 0);
    gpio_set_level(GRID_ESP32_PINS_MUX_0_B, 0);
    gpio_set_level(GRID_ESP32_PINS_MUX_0_C, 0);

    gpio_set_level(GRID_ESP32_PINS_MUX_1_A, 0);
    gpio_set_level(GRID_ESP32_PINS_MUX_1_B, 0);
    gpio_set_level(GRID_ESP32_PINS_MUX_1_C, 0);

}


void IRAM_ATTR grid_esp32_adc_mux_increment(struct grid_esp32_adc_model* adc, uint8_t max){
    adc->mux_index++;
    adc->mux_index%=max;
}

void IRAM_ATTR grid_esp32_adc_mux_update(struct grid_esp32_adc_model* adc){

    gpio_ll_set_level(&GPIO, GRID_ESP32_PINS_MUX_0_A, adc->mux_index/1%2);
    gpio_ll_set_level(&GPIO, GRID_ESP32_PINS_MUX_0_B, adc->mux_index/2%2);
    gpio_ll_set_level(&GPIO, GRID_ESP32_PINS_MUX_0_C, adc->mux_index/4%2);

    gpio_ll_set_level(&GPIO, GRID_ESP32_PINS_MUX_1_A, adc->mux_index/1%2);
    gpio_ll_set_level(&GPIO, GRID_ESP32_PINS_MUX_1_B, adc->mux_index/2%2);
    gpio_ll_set_level(&GPIO, GRID_ESP32_PINS_MUX_1_C, adc->mux_index/4%2);

}

uint8_t IRAM_ATTR grid_esp32_adc_mux_get_index(struct grid_esp32_adc_model* adc){
    return adc->mux_index;
}

void grid_esp32_adc_init(struct grid_esp32_adc_model* adc){

    adc->adc_handle = NULL;

    continuous_adc_init(&adc->adc_handle);

    adc->mux_index = 0;

}

void grid_esp32_adc_register_callback(struct grid_esp32_adc_model* adc, void (*callback)(adc_continuous_handle_t, const adc_continuous_evt_data_t*, void*)){
   
    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = callback,
    };

    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(adc->adc_handle, &cbs, (void*)grid_esp32_adc_result_buffer));
    grid_esp32_adc_result_buffer[4]=255;
}


void grid_esp32_adc_start(struct grid_esp32_adc_model* adc){


    ESP_ERROR_CHECK(adc_continuous_start(adc->adc_handle));

}

void grid_esp32_adc_stop(struct grid_esp32_adc_model* adc){


    ESP_ERROR_CHECK(adc_continuous_stop(adc->adc_handle));

}

void grid_esp32_adc_read(struct grid_esp32_adc_model* adc, uint16_t* channel_0_index, uint16_t* channel_1_index, uint16_t* channel_0_value, uint16_t* channel_1_value){

    *channel_0_index = grid_esp32_adc_result_buffer[0];
    *channel_1_index = grid_esp32_adc_result_buffer[1];
    *channel_0_value = grid_esp32_adc_result_buffer[2];
    *channel_1_value = grid_esp32_adc_result_buffer[3];

    grid_esp32_adc_result_buffer[4]=255;

}

