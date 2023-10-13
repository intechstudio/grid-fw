 /*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "grid_esp32_module_bu16.h"


static const char *TAG = "module_bu16";


void grid_esp32_module_bu16_task(void *arg)
{

    uint64_t potmeter_last_real_time[16] = {0};
    static const uint8_t multiplexer_lookup[16] = {2, 0, 3, 1, 6, 4, 7, 5, 10, 8, 11, 9, 14, 12, 15, 13};
    static const uint8_t invert_result_lookup[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint8_t multiplexer_overflow = 8;
    ESP_LOGI("IN", "0");
    grid_esp32_adc_init(&grid_esp32_adc_state, (SemaphoreHandle_t)arg);
    ESP_LOGI("IN", "1");
    grid_esp32_adc_mux_init(&grid_esp32_adc_state, multiplexer_overflow);    
    ESP_LOGI("IN", "2");
    grid_esp32_adc_start(&grid_esp32_adc_state);    
    ESP_LOGI("IN", "3");

    while (1) {

        for (uint16_t i = 0; i<10; i++){

            size_t size = 0;

            struct grid_esp32_adc_result* result;
            result = (struct grid_esp32_adc_result*) xRingbufferReceive(grid_esp32_adc_state.ringbuffer_handle , &size, 0);

            if (result!=NULL){

                uint8_t lookup_index = result->mux_state*2 + result->channel;

                if (invert_result_lookup[lookup_index]){
                    result->value = 4095-result->value;
                }

                grid_ui_button_store_input(multiplexer_lookup[lookup_index], &potmeter_last_real_time[lookup_index], result->value, 12); 
                vRingbufferReturnItem(grid_esp32_adc_state.ringbuffer_handle , result);

            }      
            else{
                break;
            }
        }

        //ESP_LOGI("IN", "END");

        taskYIELD();

        vTaskDelay(pdMS_TO_TICKS(GRID_ESP32_ADC_PROCESS_TASK_DELAY_MS*10));


    }


    //Wait to be deleted
    vTaskSuspend(NULL);
}
