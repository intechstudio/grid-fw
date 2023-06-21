/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "grid_esp32_module_pbf4.h"

static const char *TAG = "module_pbf4";


void grid_esp32_module_pbf4_task(void *arg)
{

    uint64_t potmeter_last_real_time[16] = {0};
    static const uint8_t multiplexer_lookup[16] = {0, 2, 1, 3, 4, 6, 5, 7, -1, -1, -1, -1, 8, 10, 9, 11};
    static const uint8_t invert_result_lookup[16] = {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint8_t multiplexer_overflow = 8;

    grid_esp32_adc_init(&grid_esp32_adc_state, (SemaphoreHandle_t)arg);
    grid_esp32_adc_mux_init(&grid_esp32_adc_state, multiplexer_overflow);
    grid_esp32_adc_register_callback(&grid_esp32_adc_state, grid_esp32_adc_conv_done_cb);
    grid_esp32_adc_start(&grid_esp32_adc_state);


    while (1) {


        for (uint16_t i = 0; i<10; i++){

            size_t size = 0;

            struct grid_esp32_adc_result* result;
            result = (struct grid_esp32_adc_result*) xRingbufferReceive(grid_esp32_adc_state.ringbuffer_handle , &size, 0);

            if (result != NULL){

                uint8_t lookup_index = result->mux_state*2 + result->channel;

                if (invert_result_lookup[lookup_index]){
                    result->value = 4095-result->value;
                }

                uint8_t element_index = multiplexer_lookup[lookup_index];

                if (element_index<8){ // potmeters and faders

                    grid_ui_potmeter_store_input(element_index, &potmeter_last_real_time[lookup_index], result->value, 12); 
                }
                else if (element_index<12){
                    grid_ui_button_store_input(element_index, &potmeter_last_real_time[lookup_index], result->value, 12); 
                }

                vRingbufferReturnItem(grid_esp32_adc_state.ringbuffer_handle , result);

            }      
            else{
                break;
            }
        }



        vTaskDelay(pdMS_TO_TICKS(GRID_ESP32_ADC_PROCESS_TASK_DELAY_MS));


    }


    //Wait to be deleted
    vTaskSuspend(NULL);
}
