 /*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "grid_esp32_module_tek2.h"


static const char *TAG = "module_tek2";


void grid_esp32_module_tek2_task(void *arg)
{

    uint64_t potmeter_last_real_time[16] = {0};
    
  //static const uint8_t multiplexer_lookup[16] = {10, 8, 11, 9, 14, 12, 15, 13, 2, 0, 3, 1, 6, 4, 7, 5};
    static const uint8_t multiplexer_lookup[16] = {9, 8, 11, 10, 13, 12, -1, -1, 2, 0, 3, 1, 6, 4, 7, 5};

    //static const uint8_t invert_result_lookup[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint8_t multiplexer_overflow = 8;

    grid_esp32_adc_init(&grid_esp32_adc_state, (SemaphoreHandle_t)arg);
    grid_esp32_adc_mux_init(&grid_esp32_adc_state, multiplexer_overflow);
    grid_esp32_adc_start(&grid_esp32_adc_state);

    struct grid_module_endlesspot_state last_endlesspot_state[2] = {0};

    while (1) {


        for (uint16_t i = 0; i<10; i++){

            size_t size = 0;

            struct grid_esp32_adc_result* result;
            result = (struct grid_esp32_adc_result*) xRingbufferReceive(grid_esp32_adc_state.ringbuffer_handle , &size, 0);

            if (result!=NULL){

                uint8_t lookup_index = result->mux_state*2 + result->channel;

                if (multiplexer_lookup[lookup_index] < 8){

                    grid_ui_button_store_input(multiplexer_lookup[lookup_index], &potmeter_last_real_time[lookup_index], result->value, 12); 

                }
                else if(multiplexer_lookup[lookup_index] < 10){ //8, 9

                    uint8_t endlesspot_index = multiplexer_lookup[lookup_index]%2;
                    last_endlesspot_state[endlesspot_index].phase_a_value = result->value;

                }
                else if(multiplexer_lookup[lookup_index] < 12){ //10, 11
                    
                    uint8_t endlesspot_index = multiplexer_lookup[lookup_index]%2;
                    last_endlesspot_state[endlesspot_index].phase_b_value = result->value;
                    //ets_printf("%d \r\n", result->value);

                }
                else if(multiplexer_lookup[lookup_index] < 14){ //12, 13
                    
                    uint8_t endlesspot_index = multiplexer_lookup[lookup_index]%2;
                    last_endlesspot_state[endlesspot_index].button_value = result->value;

                    if (endlesspot_index == 0){
                        //void grid_ui_endlesspot_store_input(uint8_t input_channel, struct grid_module_endlesspot_state* old_value, struct grid_module_endlesspot_state* new_value,  uint8_t adc_bit_depth){
                        grid_ui_endlesspot_store_input(8+endlesspot_index, &last_endlesspot_state[endlesspot_index], &last_endlesspot_state[endlesspot_index], 12);
                    }

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
