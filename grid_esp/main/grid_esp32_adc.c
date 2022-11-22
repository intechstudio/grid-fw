/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "grid_esp32_adc.h"


static const char *TAG = "esp32_adc";


#include "../../grid_common/grid_led.h"


void grid_esp32_adc_task(void *arg)
{

    


    ESP_LOGI(TAG, "Init ADC");

    //-------------ADC1 Init---------------//
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config1 = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_1, &config1));
   
    //-------------ADC2 Init---------------//
    adc_oneshot_unit_handle_t adc2_handle;
    adc_oneshot_unit_init_cfg_t init_config2 = {
        .unit_id = ADC_UNIT_2,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config2, &adc2_handle));

    //-------------ADC2 Config---------------//
    adc_oneshot_chan_cfg_t config2 = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc2_handle, ADC_CHANNEL_7, &config2));

    grid_ain_init(&grid_ain_state, 2, 16);


    while (1) {


    
        int value_1;
        int value_2;

        adc_oneshot_read(adc1_handle, ADC_CHANNEL_1, &value_1);
        adc_oneshot_read(adc2_handle, ADC_CHANNEL_7, &value_2);
        
        int32_t result_resolution = 10;
        int32_t source_resolution = 12;

        grid_ain_add_sample(&grid_ain_state, 0, value_1, source_resolution, result_resolution);
        grid_ain_add_sample(&grid_ain_state, 1, value_2, source_resolution, result_resolution);

        for (uint8_t i=0; i<2; i++){

           if (grid_ain_get_changed(&grid_ain_state, i)){

                int32_t min = 1023;
                int32_t max = 0;

                int32_t scaled = grid_ain_get_average_scaled(&grid_ain_state, i, source_resolution, result_resolution, min, max);

                grid_led_set_layer_phase(&grid_led_state, i, GRID_LED_LAYER_UI_A, scaled/4);


                ESP_LOGI(TAG, "CH %d reading: %ld", i, scaled);  


                
            }         
        }

        vTaskDelay(pdMS_TO_TICKS(10));


    }


    //Wait to be deleted
    vTaskSuspend(NULL);
}
