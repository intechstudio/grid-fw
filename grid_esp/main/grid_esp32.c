/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32.h"

static const char *TAG = "grid_esp32";



uint32_t grid_platform_get_hwcfg(){


    gpio_set_direction(GRID_ESP32_PINS_HWCFG_SHIFT, GPIO_MODE_OUTPUT);
    gpio_set_direction(GRID_ESP32_PINS_HWCFG_CLOCK, GPIO_MODE_OUTPUT);
    gpio_set_direction(GRID_ESP32_PINS_HWCFG_DATA, GPIO_MODE_INPUT);

    gpio_set_level(GRID_ESP32_PINS_HWCFG_SHIFT, 0);
    gpio_set_level(GRID_ESP32_PINS_HWCFG_CLOCK, 0);

    ets_delay_us(1000);

    uint8_t hwcfg_value = 0;

    for(uint8_t i = 0; i<8; i++){ // now we need to shift in the remaining 7 values
            
        // SHIFT DATA
        gpio_set_level(GRID_ESP32_PINS_HWCFG_SHIFT, 1); //This outputs the first value to HWCFG_DATA
        ets_delay_us(1000);
            
            
        if(gpio_get_level(GRID_ESP32_PINS_HWCFG_DATA)){
                
            hwcfg_value |= (1<<i);
                
            }else{
                
                
        }
            
        if(i!=7){
                
            // Clock rise
            gpio_set_level(GRID_ESP32_PINS_HWCFG_CLOCK, 1);
                
            ets_delay_us(1000);
                
            gpio_set_level(GRID_ESP32_PINS_HWCFG_CLOCK, 0);
        }
                        
    }

    ESP_LOGI(TAG, "HWCFG value: %d", hwcfg_value);
    return hwcfg_value;

}


uint32_t grid_platform_get_id(uint32_t* return_array){

/*

    struct ESP_FUSE3
    {
        uint8_t crc;
        uint8_t macAddr[6];
        uint8_t reserved[8];
        uint8_t version;
    };
*/



    uint8_t block[32] = {0};

    if (ESP_OK == esp_efuse_read_block(EFUSE_BLK1, block, 0, 6*8)){
        ESP_LOGI(TAG, "CPUID OK");
 
    }

    uint8_t* mac_address = &block[0];

    ESP_LOGI(TAG, "MAC: %02x:%02x:%02x:%02x:%02x:%02x",mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5]);



    uint64_t cpuid = 0;

    for (uint8_t i=0; i<6; i++){

        //ESP_LOGI(TAG, "CPUID: %016llx",cpuid);
        cpuid |= ((uint64_t)mac_address[i])<<((5-i)*8);
    }

    ESP_LOGI(TAG, "CPUID: %016llx",cpuid);

    uint8_t* array = (uint8_t*)return_array;
    array[0] = mac_address[0];
    array[1] = mac_address[1];
    array[2] = mac_address[2];
    array[3] = mac_address[3];
    array[4] = mac_address[4];
    array[5] = mac_address[5];

    return 0;
}



uint8_t grid_platform_get_random_8(){
    uint32_t random_number = esp_random();
    return random_number%256;
}


void grid_platform_delay_ms(uint32_t delay_milliseconds){
    ets_delay_us(delay_milliseconds*1000);
}

uint8_t grid_platform_get_reset_cause(){
    return 0;
}