/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_intr_alloc.h"
#include "usb/usb_host.h"

#include <string.h>

#include "driver/spi_master.h"
#include "rom/ets_sys.h" // For ets_printf

#include "driver/gpio.h"




#define LED_TASK_PRIORITY 2
extern void led_task(void *arg);


#include <esp_timer.h>



#define SWD_CLK_PIN 12
#define SWD_IO_PIN 13

#define SWD_CLOCK_PERIOD 1



void swd_dummy_clock(void){
    gpio_set_level(SWD_CLK_PIN, 1);
    ets_delay_us(SWD_CLOCK_PERIOD);
    gpio_set_level(SWD_CLK_PIN, 0);
    ets_delay_us(SWD_CLOCK_PERIOD);
}

void swd_write_raw(uint32_t data, uint8_t length){

    for(uint8_t i=0; i<length; i++){

        if ((data >> (length-1-i))&0x00000001u){
            gpio_set_level(SWD_IO_PIN, 1);
        }
        else{
            gpio_set_level(SWD_IO_PIN, 0);
        }
        gpio_set_level(SWD_CLK_PIN, 0);
        ets_delay_us(SWD_CLOCK_PERIOD);
        gpio_set_level(SWD_CLK_PIN, 1);
        ets_delay_us(SWD_CLOCK_PERIOD);
 
    }    
    gpio_set_level(SWD_CLK_PIN, 0);

    gpio_set_level(SWD_IO_PIN, 0);

    ets_delay_us(SWD_CLOCK_PERIOD);

}


void swd_write(uint32_t data, uint8_t length){


    uint32_t num_of_ones = 0;

    for(uint8_t i=0; i<length; i++){

        if ((data >> (i))&0x00000001u){
            gpio_set_level(SWD_IO_PIN, 1);
            num_of_ones++;
        }
        else{
            gpio_set_level(SWD_IO_PIN, 0);
        }
        gpio_set_level(SWD_CLK_PIN, 0);
        ets_delay_us(SWD_CLOCK_PERIOD);
        gpio_set_level(SWD_CLK_PIN, 1);
        ets_delay_us(SWD_CLOCK_PERIOD);
 
    }    
    gpio_set_level(SWD_CLK_PIN, 0);



    if (length == 32){

        // write parity bit
        if (num_of_ones%2==1){
            gpio_set_level(SWD_IO_PIN, 1);
        }
        else{
            gpio_set_level(SWD_IO_PIN, 0);
        }

        ets_delay_us(SWD_CLOCK_PERIOD);
        gpio_set_level(SWD_CLK_PIN, 1);
        ets_delay_us(SWD_CLOCK_PERIOD);
    }

    gpio_set_level(SWD_CLK_PIN, 0);
    gpio_set_level(SWD_IO_PIN, 0);

    ets_delay_us(SWD_CLOCK_PERIOD);

}

void swd_linereset(){

    for(uint8_t i=0; i<7; i++){

        ets_delay_us(SWD_CLOCK_PERIOD*1);
        gpio_set_level(SWD_IO_PIN, 1);
        ets_delay_us(SWD_CLOCK_PERIOD*1);

        swd_write_raw(0xff, 8);

        ets_delay_us(SWD_CLOCK_PERIOD*1);
        gpio_set_level(SWD_IO_PIN, 0);
        ets_delay_us(SWD_CLOCK_PERIOD*1);

    }



}

void swd_switch_from_jtag_to_swd(){

    gpio_set_level(SWD_IO_PIN, 1);
    uint32_t switch_sequence = 0b0111100111100111;
    for(uint8_t i=0; i<16; i++){

        if ((switch_sequence >> (15-i))&0x00000001u){
            gpio_set_level(SWD_IO_PIN, 1);
        }
        else{
            gpio_set_level(SWD_IO_PIN, 0);
        }
        ets_delay_us(SWD_CLOCK_PERIOD);
        gpio_set_level(SWD_CLK_PIN, 1);
        ets_delay_us(SWD_CLOCK_PERIOD);
        gpio_set_level(SWD_CLK_PIN, 0);

    }

    ets_delay_us(SWD_CLOCK_PERIOD*5);


}

void swd_turnround_target_next(){

    gpio_set_pull_mode(SWD_IO_PIN, GPIO_PULLUP_ENABLE);
    gpio_pullup_en(SWD_IO_PIN);
    gpio_set_direction(SWD_IO_PIN, GPIO_MODE_INPUT);


    ets_delay_us(SWD_CLOCK_PERIOD);
    gpio_set_level(SWD_CLK_PIN, 1);
    ets_delay_us(SWD_CLOCK_PERIOD);
    gpio_set_level(SWD_CLK_PIN, 0);
    ets_delay_us(SWD_CLOCK_PERIOD);
    gpio_set_level(SWD_CLK_PIN, 1);
    ets_delay_us(SWD_CLOCK_PERIOD);

}

void swd_turnround_host_next(){
    
    gpio_set_level(SWD_CLK_PIN, 1);
    ets_delay_us(SWD_CLOCK_PERIOD);
    gpio_set_level(SWD_CLK_PIN, 0);
    ets_delay_us(SWD_CLOCK_PERIOD);
    gpio_set_direction(SWD_IO_PIN, GPIO_MODE_OUTPUT);

}

uint8_t swd_read_acknowledge(){

    uint8_t acknowledge = 0;

    for(uint8_t i=0; i<3; i++){
        gpio_set_level(SWD_CLK_PIN, 1);
        ets_delay_us(SWD_CLOCK_PERIOD);

        acknowledge |= gpio_get_level(SWD_IO_PIN)<<(2-i);

        gpio_set_level(SWD_CLK_PIN, 0);
        ets_delay_us(SWD_CLOCK_PERIOD);
    }

    //printf("ACKNOWLEDGE: %d\r\n", acknowledge);

    return acknowledge;

}


void swd_target_select(){

    swd_write_raw(0b10011001, 8); // targetselect should be 0b10011101

    // fake ack
    gpio_set_level(SWD_IO_PIN, 1);


    gpio_set_level(SWD_CLK_PIN, 1);
    ets_delay_us(SWD_CLOCK_PERIOD);


    gpio_set_level(SWD_CLK_PIN, 0);
    ets_delay_us(SWD_CLOCK_PERIOD);
    
    gpio_set_level(SWD_IO_PIN, 0);

    gpio_set_level(SWD_CLK_PIN, 1);
    ets_delay_us(SWD_CLOCK_PERIOD);
    gpio_set_level(SWD_CLK_PIN, 0);
    ets_delay_us(SWD_CLOCK_PERIOD);    


    gpio_set_level(SWD_CLK_PIN, 1);
    ets_delay_us(SWD_CLOCK_PERIOD);
    gpio_set_level(SWD_CLK_PIN, 0);
    ets_delay_us(SWD_CLOCK_PERIOD);    
    gpio_set_level(SWD_CLK_PIN, 1);
    ets_delay_us(SWD_CLOCK_PERIOD);
    gpio_set_level(SWD_CLK_PIN, 0);
    ets_delay_us(SWD_CLOCK_PERIOD);    
    gpio_set_level(SWD_CLK_PIN, 1);
    ets_delay_us(SWD_CLOCK_PERIOD);
    gpio_set_level(SWD_CLK_PIN, 0);
    ets_delay_us(SWD_CLOCK_PERIOD);    


    #define RP2040_CORE0_ID 0x01002927 // this is reversed
    #define RP2040_CORE1_ID 0x11002927

    swd_write(RP2040_CORE0_ID, 32);

}

uint32_t swd_read(uint8_t length){

    uint32_t retval = gpio_get_level(SWD_IO_PIN);
    //printf("Read[0:%d]: %d", length-1, gpio_get_level(SWD_IO_PIN));

    ets_delay_us(SWD_CLOCK_PERIOD);
    for(uint8_t i=1; i<length; i++){
        gpio_set_level(SWD_CLK_PIN, 1);
        ets_delay_us(SWD_CLOCK_PERIOD);
        gpio_set_level(SWD_CLK_PIN, 0);
        retval |= gpio_get_level(SWD_IO_PIN)<<(i);
        //printf("%d", gpio_get_level(SWD_IO_PIN));
        if (i%4 == 3){
            //printf(" ");
        }
        ets_delay_us(SWD_CLOCK_PERIOD);

    }
    //printf("= 0x%08lx\r\n", retval);

    if (length == 32){

        gpio_set_level(SWD_CLK_PIN, 1);
        ets_delay_us(SWD_CLOCK_PERIOD);
        gpio_set_level(SWD_CLK_PIN, 0);
        ets_delay_us(SWD_CLOCK_PERIOD);

    }

    return retval;

}

uint32_t swd_read_idcode(){


    // IDCODE

    swd_write(0b10100101,8);

    swd_turnround_target_next();

    //ACKNOWLEDGE
    swd_read_acknowledge();

    //DATA
    uint32_t idcode = swd_read(32);


    swd_turnround_host_next();
    

    return 0;

}

uint32_t swd_read_dlcr(){


    // DLCR

    swd_write_raw(0b10110001,8);

    swd_turnround_target_next();

    //ACKNOWLEDGE
    swd_read_acknowledge();

    //DATA
    uint32_t dlcr = swd_read(32);


    swd_turnround_host_next();
    

    return 0;

}


void swd_write_select(uint32_t value){

    swd_write(0xb1,8); // w select
    swd_turnround_target_next();
    swd_read_acknowledge();
    swd_turnround_host_next();
    swd_write(value,32);

}

void swd_write_ctrlstat(uint32_t value){

    swd_write_raw(0b10010101,8); // w ctrlstat
    swd_turnround_target_next();
    swd_read_acknowledge();
    swd_turnround_host_next();
    swd_write(value,32);

}

uint32_t swd_read_ctrlstat(){

    swd_write_raw(0b10110001,8);

    swd_turnround_target_next();

    //ACKNOWLEDGE
    swd_read_acknowledge();

    //DATA
    uint32_t dlcr = swd_read(32);


    swd_turnround_host_next();
    

    return 0;

}


void app_main(void)
{

    static const char *TAG = "main";



    #define LDO_PIN 17
    gpio_set_direction(LDO_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LDO_PIN, 1);


    vTaskDelay(100);
    
    #define TRIGGER_PIN 9
    gpio_set_direction(TRIGGER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(TRIGGER_PIN, 1);



    gpio_set_direction(SWD_CLK_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(SWD_CLK_PIN, 0);

    gpio_set_level(SWD_IO_PIN, 1);
    gpio_set_direction(SWD_IO_PIN, GPIO_MODE_OUTPUT);



    ets_delay_us(SWD_CLOCK_PERIOD*5);

    // magic write packets, no response from target, confirmed

    swd_write_raw(0b11111111, 8);
    swd_write_raw(0b01001001, 8);
    swd_write_raw(0b11001111, 8);
    swd_write_raw(0b10010000, 8);

    swd_write_raw(0b01000110, 8);
    swd_write_raw(0b10101001, 8);
    swd_write_raw(0b10110100, 8);
    swd_write_raw(0b10100001, 8);
    
    swd_write_raw(0b01100001, 8);
    swd_write_raw(0b10010111, 8);
    swd_write_raw(0b11110101, 8);
    swd_write_raw(0b10111011, 8);
    
    swd_write_raw(0b11000111, 8);
    swd_write_raw(0b01000101, 8);
    swd_write_raw(0b01110000, 8);
    swd_write_raw(0b00111101, 8);
    
    swd_write_raw(0b10011000, 8);
    swd_write_raw(0b00000101, 8);
    swd_write_raw(0b10001111, 8);


    // initialization
    swd_linereset();
    //swd_switch_from_jtag_to_swd();

    // IDLE
    gpio_set_level(SWD_IO_PIN, 0);
    for(uint8_t i=0; i<8; i++){
        gpio_set_level(SWD_CLK_PIN, 1);
        ets_delay_us(SWD_CLOCK_PERIOD);
        gpio_set_level(SWD_CLK_PIN, 0);
        ets_delay_us(SWD_CLOCK_PERIOD);

    }

    swd_linereset();


    ets_delay_us(SWD_CLOCK_PERIOD*5);

    // IDLE
    gpio_set_level(SWD_IO_PIN, 0);
    for(uint8_t i=0; i<8; i++){
        gpio_set_level(SWD_CLK_PIN, 1);
        ets_delay_us(SWD_CLOCK_PERIOD);
        gpio_set_level(SWD_CLK_PIN, 0);
        ets_delay_us(SWD_CLOCK_PERIOD);

    }
    

    swd_dummy_clock();
    swd_target_select();
    swd_dummy_clock();

    swd_read_idcode();
    swd_dummy_clock();

    swd_write(0x81,8); // w abort
    swd_turnround_target_next();
    swd_read_acknowledge();
    swd_turnround_host_next();
    swd_write(0x0000001e,32);


    swd_dummy_clock();

    swd_write_select(0x00000003);
    swd_dummy_clock();

    swd_read_dlcr();
    swd_dummy_clock();

    swd_write_select(0x00000000);
    swd_dummy_clock();

    swd_write_ctrlstat(0x50000020);
    swd_dummy_clock();

    swd_read_ctrlstat();
    swd_dummy_clock();

    swd_write_ctrlstat(0x50000000);
    swd_dummy_clock();

    swd_read_ctrlstat();
    swd_dummy_clock();

    swd_read_ctrlstat();
    swd_dummy_clock();

    swd_read_ctrlstat();
    swd_dummy_clock();  
        
    swd_write_ctrlstat(0x50000001);
    swd_dummy_clock();

    swd_read_ctrlstat();
    swd_dummy_clock();

    SemaphoreHandle_t signaling_sem = xSemaphoreCreateBinary();



    TaskHandle_t led_task_hdl;

    //Create the class driver task
    xTaskCreatePinnedToCore(led_task,
                            "led",
                            4096,
                            (void *)signaling_sem,
                            LED_TASK_PRIORITY,
                            &led_task_hdl,
                            0);


}
