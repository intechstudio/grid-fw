/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "grid_esp32_adc.h"

struct grid_esp32_adc_model DRAM_ATTR grid_esp32_adc_state;



void grid_esp32_adc_mux_init(struct grid_esp32_adc_model* adc, uint8_t mux_overflow){

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

    adc->mux_overflow = mux_overflow;
}


void IRAM_ATTR grid_esp32_adc_mux_increment(struct grid_esp32_adc_model* adc){
    adc->mux_index++;
    adc->mux_index%=adc->mux_overflow;
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



static void adc_init(struct grid_esp32_adc_model* adc){

    //-------------ADC1 Init---------------//
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc->adc_handle_0));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config1 = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc->adc_handle_0, ADC_CHANNEL_1, &config1));
   
    //-------------ADC2 Init---------------//
    adc_oneshot_unit_init_cfg_t init_config2 = {
        .unit_id = ADC_UNIT_2,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config2, &adc->adc_handle_1));

    //-------------ADC2 Config---------------//
    adc_oneshot_chan_cfg_t config2 = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc->adc_handle_1, ADC_CHANNEL_7, &config2));

}



void grid_esp32_adc_init(struct grid_esp32_adc_model* adc, SemaphoreHandle_t nvm_semaphore){

    adc->nvm_semaphore = nvm_semaphore;

    adc->adc_handle_0 = NULL;
    adc->adc_handle_1 = NULL;

    adc->buffer_struct = (StaticRingbuffer_t *)heap_caps_malloc(sizeof(StaticRingbuffer_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    adc->buffer_storage = (struct grid_esp32_adc_result *)heap_caps_malloc(sizeof(struct grid_esp32_adc_result)*BUFFER_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    adc->ringbuffer_handle = xRingbufferCreateStatic(BUFFER_SIZE, BUFFER_TYPE, adc->buffer_storage, adc->buffer_struct);


    adc_init(adc);


    adc->mux_index = 0;

}



void grid_esp32_adc_start(struct grid_esp32_adc_model* adc){

    //  start periodic task

    esp_timer_create_args_t periodic_adc_args = {
        .callback = &grid_esp32_adc_convert,
        .name = "adc millisecond"
    };

   esp_timer_handle_t periodic_adc_timer;
   ESP_ERROR_CHECK(esp_timer_create(&periodic_adc_args, &periodic_adc_timer));
   ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_adc_timer, 1000));


}

void grid_esp32_adc_stop(struct grid_esp32_adc_model* adc){



}



void IRAM_ATTR grid_esp32_adc_convert(void)
{


    struct grid_esp32_adc_model* adc = &grid_esp32_adc_state;

    if (xSemaphoreTakeFromISR(adc->nvm_semaphore, NULL) == pdTRUE){


        grid_esp32_adc_mux_increment(&grid_esp32_adc_state);
        grid_esp32_adc_mux_update(&grid_esp32_adc_state);



        int adcresult_0 = 0;
        int adcresult_1 = 0;
    
        adc_oneshot_read_isr(adc->adc_handle_0, ADC_CHANNEL_1, &adcresult_0);
        adc_oneshot_read_isr(adc->adc_handle_1, ADC_CHANNEL_7, &adcresult_1);

        struct grid_esp32_adc_result result_0;
        result_0.channel = 0;
        result_0.mux_state = grid_esp32_adc_mux_get_index(&grid_esp32_adc_state);;
        result_0.value = adcresult_0;

        struct grid_esp32_adc_result result_1;
        result_1.channel = 1;
        result_1.mux_state = grid_esp32_adc_mux_get_index(&grid_esp32_adc_state);;
        result_1.value = adcresult_1;


        xRingbufferSendFromISR(adc->ringbuffer_handle , &result_0, sizeof(struct grid_esp32_adc_result), NULL);
        xRingbufferSendFromISR(adc->ringbuffer_handle , &result_1, sizeof(struct grid_esp32_adc_result), NULL);
            
      
        xSemaphoreGiveFromISR(adc->nvm_semaphore, NULL);
    }



    
}

