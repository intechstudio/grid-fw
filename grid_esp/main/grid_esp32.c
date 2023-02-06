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

void grid_platform_printf(char const *fmt, ...){


	va_list ap;

	char temp[200] = {0};

	va_start(ap, fmt);

	vsnprintf(temp, 199, fmt, ap);

	va_end(ap);

    ets_printf(temp);


}


uint8_t grid_platform_disable_grid_transmitter(uint8_t direction){

    ets_printf("grid_platform_disable_grid_transmitter NOT IMPLEMENTED!!!");
    return 1;
}

uint8_t grid_platform_reset_grid_transmitter(uint8_t direction){
    
    ets_printf("grid_platform_reset_grid_transmitter NOT IMPLEMENTED!!!");
    return 1;
}

uint8_t grid_platform_enable_grid_transmitter(uint8_t direction){
    
    ets_printf("grid_platform_enable_grid_transmitter NOT IMPLEMENTED!!!");
    return 1;
}

int32_t grid_platform_usb_serial_write(char* buffer, uint32_t length){


    
    
    uint32_t queued = tinyusb_cdcacm_write_queue(0, (const uint8_t*) buffer, length);


    /*
    char temp[length+1];
    temp[length] = '\0';
    for (uint16_t i=0; i<length; i++){

        temp[i] = buffer[i];
    }
    ets_printf("CDC: %d %s\r\n", queued, temp);
    */

    //tinyusb_cdcacm_write_flush(0, pdMS_TO_TICKS(1000));


    return 1;
}

void* grid_platform_find_actionstring_file(uint8_t page, uint8_t element, uint8_t event_type){

    ets_printf("MOCK!!!\r\n");
    return NULL; //not found
}


uint16_t grid_platform_get_actionstring_file_size(void* file_pointer){

    ets_printf("grid_platform_get_actionstring_file_size NOT IMPLEMENTED!!!");
    return 0;
}


uint32_t grid_platform_read_actionstring_file_contents(void* file_pointer, char* targetstring){

    ets_printf("grid_platform_read_actionstring_file_contents NOT IMPLEMENTED!!!");
    return 0;
}


void grid_platform_delete_actionstring_file(void* file_pointer){

    ets_printf("grid_platform_delete_actionstring_file NOT IMPLEMENTED!!!");
    return;
}


void grid_platform_write_actionstring_file(uint8_t page, uint8_t element, uint8_t event_type, char* buffer, uint16_t length){

    ets_printf("grid_platform_write_actionstring_file NOT IMPLEMENTED!!!");
    return;
}


uint8_t grid_platform_get_nvm_state(){


    ets_printf("grid_platform_get_nvm_state NOT IMPLEMENTED!!!");
    return 1; //ready
}


void	grid_platform_clear_actionstring_files_from_page(uint8_t page){


    ets_printf("grid_platform_clear_actionstring_files_from_page NOT IMPLEMENTED!!!");
    return;
}
;
void grid_platform_delete_actionstring_files_all(){


    ets_printf("grid_platform_delete_actionstring_files_all NOT IMPLEMENTED!!!");
    return;
}



uint8_t grid_platform_erase_nvm_next(){

    ets_printf("grid_platform_erase_nvm_next NOT IMPLEMENTED!!!");
    return 0; // done

}

uint8_t grid_platform_send_grid_message(uint8_t direction, char* buffer, uint16_t length){

    ets_printf("grid_platform_send_grid_message NOT IMPLEMENTED!!!");
    return 0; // done

}


uint32_t grid_plaform_get_nvm_nextwriteoffset(){

    ets_printf("grid_plaform_get_nvm_nextwriteoffset NOT IMPLEMENTED!!!");
    return 0; // done

}



void grid_platform_system_reset(){

    ets_printf("grid_platform_system_reset NOT IMPLEMENTED!!!");

}

void grid_platform_nvm_defrag(){

    ets_printf("grid_platform_nvm_defrag NOT IMPLEMENTED!!!");

}

int32_t grid_platform_usb_midi_write(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3){

    const uint8_t buffer[] =  {byte0, byte1, byte2, byte3};

    tud_midi_packet_write(buffer);
    ets_printf("PLATFORM MIDI WRITE \r\n");
    return 0;
   

}


int32_t grid_platform_usb_midi_write_status(void){

    ets_printf("PLATFORM MIDI STATUS \r\n");
    //ets_printf("grid_platform_usb_midi_write_status NOT IMPLEMENTED!!!");
    return 0;

}