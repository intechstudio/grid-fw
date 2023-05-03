/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "grid_esp32_module_bu16.h"
#include "../../grid_common/grid_led.h"


static const char *TAG = "esp32_adc";


static void adc_init(adc_oneshot_unit_handle_t* adc1_handle, adc_oneshot_unit_handle_t* adc2_handle){

    ESP_LOGI(TAG, "Init ADC");

    //-------------ADC1 Init---------------//
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, adc1_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config1 = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(*adc1_handle, ADC_CHANNEL_1, &config1));
   
    //-------------ADC2 Init---------------//
    adc_oneshot_unit_init_cfg_t init_config2 = {
        .unit_id = ADC_UNIT_2,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config2, adc2_handle));

    //-------------ADC2 Config---------------//
    adc_oneshot_chan_cfg_t config2 = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(*adc2_handle, ADC_CHANNEL_7, &config2));

}

void grid_esp32_module_bu16_task(void *arg)
{

    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_handle_t adc2_handle;

    adc_init(&adc1_handle, &adc2_handle);


    static uint8_t multiplexer_index =0;
    static const uint8_t multiplexer_lookup[16] = {0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15};

    static uint32_t last_real_time[16] = {0};



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


    while (1) {


	    /* Read conversion results */

        int adcresult_0 = 0;
        int adcresult_1 = 0;
        
        uint8_t adc_index_0 = multiplexer_lookup[multiplexer_index+8];
        uint8_t adc_index_1 = multiplexer_lookup[multiplexer_index+0];


        adc_oneshot_read(adc1_handle, ADC_CHANNEL_1, &adcresult_0);
        adc_oneshot_read(adc2_handle, ADC_CHANNEL_7, &adcresult_1);

	
        /* Update the multiplexer */

        multiplexer_index++;
        multiplexer_index%=8;


        gpio_set_level(GRID_ESP32_PINS_MUX_0_A, multiplexer_index/1%2);
        gpio_set_level(GRID_ESP32_PINS_MUX_0_B, multiplexer_index/2%2);
        gpio_set_level(GRID_ESP32_PINS_MUX_0_C, multiplexer_index/4%2);

        gpio_set_level(GRID_ESP32_PINS_MUX_1_A, multiplexer_index/1%2);
        gpio_set_level(GRID_ESP32_PINS_MUX_1_B, multiplexer_index/2%2);
        gpio_set_level(GRID_ESP32_PINS_MUX_1_C, multiplexer_index/4%2);
        
        int32_t result_resolution = 7;
        int32_t source_resolution = 12;


        grid_ui_button_store_input(adc_index_0, &last_real_time[adc_index_0], adcresult_0, 12); // 12 bit analog values
        grid_ui_button_store_input(adc_index_1, &last_real_time[adc_index_1], adcresult_1, 12); // 12 bit analog values


        if (multiplexer_index == 0){
            vTaskDelay(pdMS_TO_TICKS(10));
        }


    }


    //Wait to be deleted
    vTaskSuspend(NULL);
}
