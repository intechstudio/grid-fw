/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32.h"

static const char *TAG = "grid_esp32";


void vTaskGetRunTimeStats2( char *pcWriteBuffer ){

    TaskStatus_t *pxTaskStatusArray;
    volatile UBaseType_t uxArraySize, x;
    uint32_t ulTotalRunTime, ulStatsAsPercentage;

        // Make sure the write buffer does not contain a string.
    *pcWriteBuffer = 0x00;

    // Take a snapshot of the number of tasks in case it changes while this
    // function is executing.
    uxArraySize = uxTaskGetNumberOfTasks();

    // Allocate a TaskStatus_t structure for each task.  An array could be
    // allocated statically at compile time.
    pxTaskStatusArray = pvPortMalloc( uxArraySize * sizeof( TaskStatus_t ) );

    if( pxTaskStatusArray != NULL )
    {
        // Generate raw status information about each task.
        uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, &ulTotalRunTime );

        //grid_platform_printf("Task Count : %d Core: %d\r\n\r\n", uxArraySize, xPortGetCoreID());

        // For percentage calculations.
        ulTotalRunTime /= 100UL;

        // Avoid divide by zero errors.
        if( ulTotalRunTime > 0 )
        {
            // For each populated position in the pxTaskStatusArray array,
            // format the raw data as human readable ASCII data
            for( x = 0; x < uxArraySize; x++ )
            {

                char taskName[10] = ".........\0";
                snprintf(taskName, 8, pxTaskStatusArray[ x ].pcTaskName);

                uint8_t core = xTaskGetAffinity(pxTaskStatusArray[ x ].xHandle);


                /* Inspect our own high water mark on entering the task. */
                unsigned long uxHighWaterMark = uxTaskGetStackHighWaterMark( pxTaskStatusArray[ x ].xHandle );


                // What percentage of the total run time has the task used?
                // This will always be rounded down to the nearest integer.
                // ulTotalRunTimeDiv100 has already been divided by 100.
                ulStatsAsPercentage = pxTaskStatusArray[ x ].ulRunTimeCounter / ulTotalRunTime;


                TaskHandle_t task = pxTaskStatusArray[ x ].xHandle;

                if( ulStatsAsPercentage > 0UL )
                {

                    //xCoreID
                    sprintf( pcWriteBuffer, "%d-%s\t\t%lu\t\t%lu pcnt\r\n", core,  taskName, uxHighWaterMark, ulStatsAsPercentage );
                    
                }
                else
                {
                    // If the percentage is zero here then the task has
                    // consumed less than 1% of the total run time.
                    sprintf( pcWriteBuffer, "%d-%s\t\t%lu\t\t<1 pcnt\r\n", core, taskName, uxHighWaterMark );
                }

                pcWriteBuffer += strlen( ( char * ) pcWriteBuffer );
            }
        }

        // The array is no longer needed, free the memory it consumes.
        vPortFree( pxTaskStatusArray );
    }
}



void grid_esp32_housekeeping_task(void *arg)
{


    char stats[3000] = {0};

    while (1) {


        vTaskGetRunTimeStats2(stats);
        
        //ets_printf("%s\r\n\r\n", stats);

        vTaskDelay(pdMS_TO_TICKS(1000));
     


    }


    //Wait to be deleted
    vTaskSuspend(NULL);
}


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

    ets_printf("grid_platform_disable_grid_transmitter NOT IMPLEMENTED!!!\r\n");
    return 1;
}

uint8_t grid_platform_reset_grid_transmitter(uint8_t direction){
    
    ets_printf("grid_platform_reset_grid_transmitter NOT IMPLEMENTED!!!\r\n");
    return 1;
}

uint8_t grid_platform_enable_grid_transmitter(uint8_t direction){
    
    ets_printf("grid_platform_enable_grid_transmitter NOT IMPLEMENTED!!!\r\n");
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


    void* fp = grid_esp32_nvm_find_file(&grid_esp32_nvm_state, page, element, event_type);

    //ets_printf("FILE IS: %lx (%d %d %d)!!!\r\n", fp, page, element, event_type);
    return fp;
}


void grid_platform_close_actionstring_file(void* file_pointer){
    
    
    fclose(file_pointer);
    //ets_printf("CLOSE_FILE\r\n");

}

uint16_t grid_platform_get_actionstring_file_size(void* file_pointer){

    uint16_t fsize = grid_esp32_nvm_get_file_size(&grid_esp32_nvm_state, file_pointer);


    //ets_printf("FILE SIZE IS: %d bytes!!!\r\n", fsize);
    return fsize;
}


uint32_t grid_platform_read_actionstring_file_contents(void* file_pointer, char* targetstring){



    //ets_printf("READ FILE \r\n");
  
  
    grid_esp32_nvm_read_config(&grid_esp32_nvm_state, file_pointer, targetstring);
    

    return 0;
}


void grid_platform_delete_actionstring_file(void* file_pointer){

    ets_printf("grid_platform_delete_actionstring_file NOT IMPLEMENTED!!!\r\n");
    return;
}


void grid_platform_write_actionstring_file(uint8_t page, uint8_t element, uint8_t event_type, char* buffer, uint16_t length){

    grid_esp32_nvm_save_config(&grid_esp32_nvm_state, page, element, event_type, buffer);

    return;
}


uint8_t grid_platform_get_nvm_state(){

    return 1; //ready, always ready
}


void	grid_platform_clear_actionstring_files_from_page(uint8_t page){

    grid_esp32_nvm_clear_page(&grid_esp32_nvm_state, page);
    ets_printf("grid_platform_clear_actionstring_files_from_page NOT IMPLEMENTED!!!\r\n");
    return;
}
;
void grid_platform_delete_actionstring_files_all(){

    grid_esp32_nvm_erase(&grid_esp32_nvm_state);

    return;
}



uint8_t grid_platform_erase_nvm_next(){

    ets_printf("ERASE WAS ALREADY DONE ON INIT!!!\r\n");
    
    return 0; // done

}


uint32_t grid_plaform_get_nvm_nextwriteoffset(){

    ets_printf("grid_plaform_get_nvm_nextwriteoffset NOT IMPLEMENTED!!!\r\n");
    return 0; // done

}



void grid_platform_system_reset(){

    ets_printf("grid_platform_system_reset NOT IMPLEMENTED!!!\r\n");

}

void grid_platform_nvm_defrag(){

    ets_printf("grid_platform_nvm_defrag NOT IMPLEMENTED!!!\r\n");

}

int32_t grid_platform_usb_midi_write(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3){

    const uint8_t buffer[] =  {byte0, byte1, byte2, byte3};

    tud_midi_packet_write(buffer);
    ets_printf("MIDI\r\n");
    return 0;
   

}


int32_t grid_platform_usb_midi_write_status(void){

    //ets_printf("PLATFORM MIDI STATUS \r\n");
    //ets_printf("grid_platform_usb_midi_write_status NOT IMPLEMENTED!!!");
    return 0;

}

uint8_t grid_platform_get_adc_bit_depth(){
    return 12;
}

uint64_t grid_platform_rtc_get_micros(void){

	return esp_timer_get_time();
}

uint64_t grid_platform_rtc_get_elapsed_time(uint64_t told){

	return grid_platform_rtc_get_micros() - told;

}

