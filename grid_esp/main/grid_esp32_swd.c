/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_swd.h"

static const char *TAG = "grid_esp32_swd";

static uint8_t swd_pin_swdio;
static uint8_t swd_pin_swclk;
static uint8_t swd_pin_sysclk;

void swd_dummy_clock(void){
    gpio_ll_set_level(&GPIO, swd_pin_swclk, 1);
    ets_delay_us(SWD_CLOCK_PERIOD);
    gpio_ll_set_level(&GPIO, swd_pin_swclk, 0);
    ets_delay_us(SWD_CLOCK_PERIOD);
}

void swd_write_raw(uint32_t data, uint8_t length){

    for(uint8_t i=0; i<length; i++){

        if ((data >> (length-1-i))&0x00000001u){
            gpio_ll_set_level(&GPIO, swd_pin_swdio, 1);
        }
        else{
            gpio_ll_set_level(&GPIO, swd_pin_swdio, 0);
        }
        gpio_ll_set_level(&GPIO, swd_pin_swclk, 0);
        ets_delay_us(SWD_CLOCK_PERIOD);
        gpio_ll_set_level(&GPIO, swd_pin_swclk, 1);
        ets_delay_us(SWD_CLOCK_PERIOD);
 
    }    
    gpio_ll_set_level(&GPIO, swd_pin_swclk, 0);

    gpio_ll_set_level(&GPIO, swd_pin_swdio, 0);

    ets_delay_us(SWD_CLOCK_PERIOD);

}


void swd_write(uint32_t data, uint8_t length){


    uint32_t num_of_ones = 0;

    for(uint8_t i=0; i<length; i++){

        if ((data >> (i))&0x00000001u){
            gpio_ll_set_level(&GPIO, swd_pin_swdio, 1);
            num_of_ones++;
        }
        else{
            gpio_ll_set_level(&GPIO, swd_pin_swdio, 0);
        }
        gpio_ll_set_level(&GPIO, swd_pin_swclk, 0);
        ets_delay_us(SWD_CLOCK_PERIOD);
        gpio_ll_set_level(&GPIO, swd_pin_swclk, 1);
        ets_delay_us(SWD_CLOCK_PERIOD);
 
    }    
    gpio_ll_set_level(&GPIO, swd_pin_swclk, 0);



    if (length == 32){

        // write parity bit
        if (num_of_ones%2==1){
            gpio_ll_set_level(&GPIO, swd_pin_swdio, 1);
        }
        else{
            gpio_ll_set_level(&GPIO, swd_pin_swdio, 0);
        }

        ets_delay_us(SWD_CLOCK_PERIOD);
        gpio_ll_set_level(&GPIO, swd_pin_swclk, 1);
        ets_delay_us(SWD_CLOCK_PERIOD);
    }

    gpio_ll_set_level(&GPIO, swd_pin_swclk, 0);
    gpio_ll_set_level(&GPIO, swd_pin_swdio, 0);

    ets_delay_us(SWD_CLOCK_PERIOD);

}

void swd_linereset(){

    for(uint8_t i=0; i<7; i++){

        ets_delay_us(SWD_CLOCK_PERIOD*1);
        gpio_ll_set_level(&GPIO, swd_pin_swdio, 1);
        ets_delay_us(SWD_CLOCK_PERIOD*1);

        swd_write_raw(0xff, 8);

        ets_delay_us(SWD_CLOCK_PERIOD*1);
        gpio_ll_set_level(&GPIO, swd_pin_swdio, 0);
        ets_delay_us(SWD_CLOCK_PERIOD*1);

    }



}

void swd_switch_from_jtag_to_swd(){

    gpio_ll_set_level(&GPIO, swd_pin_swdio, 1);
    uint32_t switch_sequence = 0b0111100111100111;
    for(uint8_t i=0; i<16; i++){

        if ((switch_sequence >> (15-i))&0x00000001u){
            gpio_ll_set_level(&GPIO, swd_pin_swdio, 1);
        }
        else{
            gpio_ll_set_level(&GPIO, swd_pin_swdio, 0);
        }
        ets_delay_us(SWD_CLOCK_PERIOD);
        gpio_ll_set_level(&GPIO, swd_pin_swclk, 1);
        ets_delay_us(SWD_CLOCK_PERIOD);
        gpio_ll_set_level(&GPIO, swd_pin_swclk, 0);

    }

    ets_delay_us(SWD_CLOCK_PERIOD*5);


}

void swd_turnround_target_next(){

    gpio_set_pull_mode(swd_pin_swdio, GPIO_PULLUP_ENABLE);
    gpio_pullup_en(swd_pin_swdio);
    gpio_set_direction(swd_pin_swdio, GPIO_MODE_INPUT);


    ets_delay_us(SWD_CLOCK_PERIOD);
    gpio_ll_set_level(&GPIO, swd_pin_swclk, 1);
    ets_delay_us(SWD_CLOCK_PERIOD);
    gpio_ll_set_level(&GPIO, swd_pin_swclk, 0);
    ets_delay_us(SWD_CLOCK_PERIOD);
    gpio_ll_set_level(&GPIO, swd_pin_swclk, 1);
    ets_delay_us(SWD_CLOCK_PERIOD);

}

void swd_turnround_host_next(){
    
    gpio_ll_set_level(&GPIO, swd_pin_swclk, 1);
    ets_delay_us(SWD_CLOCK_PERIOD);
    gpio_ll_set_level(&GPIO, swd_pin_swclk, 0);
    ets_delay_us(SWD_CLOCK_PERIOD);
    gpio_set_direction(swd_pin_swdio, GPIO_MODE_OUTPUT);

}

uint8_t swd_read_acknowledge(){

    uint8_t acknowledge = 0;

    for(uint8_t i=0; i<3; i++){
        gpio_ll_set_level(&GPIO, swd_pin_swclk, 1);
        ets_delay_us(SWD_CLOCK_PERIOD);

        acknowledge |= gpio_get_level(swd_pin_swdio)<<(2-i);

        gpio_ll_set_level(&GPIO, swd_pin_swclk, 0);
        ets_delay_us(SWD_CLOCK_PERIOD);
    }

    //printf("ACKNOWLEDGE: %d\r\n", acknowledge);

    return acknowledge;

}


void swd_target_select(uint8_t core_id){

    swd_write_raw(0b10011001, 8); // targetselect should be 0b10011101

    // // fake ack
    // gpio_ll_set_level(&GPIO, swd_pin_swdio, 1);


    // gpio_ll_set_level(&GPIO, swd_pin_swclk, 1);
    // ets_delay_us(SWD_CLOCK_PERIOD);


    // gpio_ll_set_level(&GPIO, swd_pin_swclk, 0);
    // ets_delay_us(SWD_CLOCK_PERIOD);
    
    // gpio_ll_set_level(&GPIO, swd_pin_swdio, 0);

    // gpio_ll_set_level(&GPIO, swd_pin_swclk, 1);
    // ets_delay_us(SWD_CLOCK_PERIOD);
    // gpio_ll_set_level(&GPIO, swd_pin_swclk, 0);
    // ets_delay_us(SWD_CLOCK_PERIOD);    


    // gpio_ll_set_level(&GPIO, swd_pin_swclk, 1);
    // ets_delay_us(SWD_CLOCK_PERIOD);
    // gpio_ll_set_level(&GPIO, swd_pin_swclk, 0);
    // ets_delay_us(SWD_CLOCK_PERIOD);    
    // gpio_ll_set_level(&GPIO, swd_pin_swclk, 1);
    // ets_delay_us(SWD_CLOCK_PERIOD);
    // gpio_ll_set_level(&GPIO, swd_pin_swclk, 0);
    // ets_delay_us(SWD_CLOCK_PERIOD);    
    // gpio_ll_set_level(&GPIO, swd_pin_swclk, 1);
    // ets_delay_us(SWD_CLOCK_PERIOD);
    // gpio_ll_set_level(&GPIO, swd_pin_swclk, 0);
    // ets_delay_us(SWD_CLOCK_PERIOD);    

    swd_turnround_target_next();
    swd_read_acknowledge();
    swd_turnround_host_next();


    #define RP2040_CORE0_ID 0x01002927 // this is reversed
    #define RP2040_CORE1_ID 0x11002927 // parity = 1

    if (core_id == 0){     
        swd_write(RP2040_CORE0_ID, 32);
    }
    else{
        swd_write(RP2040_CORE1_ID, 32);
    }

}

uint32_t swd_read(uint8_t length){

    uint32_t retval = gpio_get_level(swd_pin_swdio);
    //printf("Read[0:%d]: %d", length-1, gpio_get_level(swd_pin_swdio));

    ets_delay_us(SWD_CLOCK_PERIOD);
    for(uint8_t i=1; i<length; i++){
        gpio_ll_set_level(&GPIO, swd_pin_swclk, 1);
        ets_delay_us(SWD_CLOCK_PERIOD);
        gpio_ll_set_level(&GPIO, swd_pin_swclk, 0);
        retval |= gpio_get_level(swd_pin_swdio)<<(i);
        //printf("%d", gpio_get_level(swd_pin_swdio));
        if (i%4 == 3){
            //printf(" ");
        }
        ets_delay_us(SWD_CLOCK_PERIOD);

    }
    //printf("= 0x%08lx\r\n", retval);

    if (length == 32){

        gpio_ll_set_level(&GPIO, swd_pin_swclk, 1);
        ets_delay_us(SWD_CLOCK_PERIOD);
        gpio_ll_set_level(&GPIO, swd_pin_swclk, 0);
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
    

    return idcode;

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
    

    return dlcr;

}


void swd_write_select(uint32_t value){

    swd_write(0xb1,8); // w select
    swd_turnround_target_next();
    swd_read_acknowledge();
    swd_turnround_host_next();
    swd_write(value,32);

}

void swd_write_abort(uint32_t value){

    swd_write(0x81,8); // w abort
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

void swd_write_apc(uint32_t value){

    swd_write_raw(0b11011101,8); // w apc
    swd_turnround_target_next();
    swd_read_acknowledge();
    swd_turnround_host_next();
    swd_write(value,32);

}

void swd_write_ap0(uint32_t value){

    swd_write_raw(0b11000101,8); // w ap0
    swd_turnround_target_next();
    swd_read_acknowledge();
    swd_turnround_host_next();
    swd_write(value,32);

}

void swd_write_ap4(uint32_t value){

    swd_write_raw(0b11010001,8); // w ap8
    swd_turnround_target_next();
    swd_read_acknowledge();
    swd_turnround_host_next();
    swd_write(value,32);

}

void swd_write_ap8(uint32_t value){

    swd_write_raw(0b11001001,8); // w ap8
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
    uint32_t ctrlstat = swd_read(32);


    swd_turnround_host_next();
    

    return ctrlstat;

}

uint32_t swd_read_buff(){

    swd_write_raw(0b10111101,8);

    swd_turnround_target_next();

    //ACKNOWLEDGE
    swd_read_acknowledge();

    //DATA
    uint32_t buff = swd_read(32);


    swd_turnround_host_next();
    

    return buff;

}

uint32_t swd_read_apc(){

    swd_write_raw(0b11111001,8);

    swd_turnround_target_next();

    //ACKNOWLEDGE
    swd_read_acknowledge();

    //DATA
    uint32_t apc = swd_read(32);


    swd_turnround_host_next();
    

    return apc;

}


uint32_t swd_read_ap0(){

    swd_write_raw(0b11100001,8);

    swd_turnround_target_next();

    //ACKNOWLEDGE
    swd_read_acknowledge();

    //DATA
    uint32_t ap0 = swd_read(32);


    swd_turnround_host_next();
    

    return ap0;

}

uint32_t swd_read_ap4(){

    swd_write_raw(0b11110101,8);

    swd_turnround_target_next();

    //ACKNOWLEDGE
    swd_read_acknowledge();

    //DATA
    uint32_t ap4 = swd_read(32);


    swd_turnround_host_next();
    

    return ap4;

}

uint32_t swd_read_ap8(){

    swd_write_raw(0b11101101,8);

    swd_turnround_target_next();

    //ACKNOWLEDGE
    swd_read_acknowledge();

    //DATA
    uint32_t ap8 = swd_read(32);


    swd_turnround_host_next();
    

    return ap8;

}

void swd_idle(){

    ets_delay_us(SWD_CLOCK_PERIOD*5);

    // IDLE
    gpio_ll_set_level(&GPIO, swd_pin_swdio, 0);
    for(uint8_t i=0; i<8; i++){
        gpio_ll_set_level(&GPIO, swd_pin_swclk, 1);
        ets_delay_us(SWD_CLOCK_PERIOD);
        gpio_ll_set_level(&GPIO, swd_pin_swclk, 0);
        ets_delay_us(SWD_CLOCK_PERIOD);

    }
    

    swd_dummy_clock();

}


void grid_esp32_swd_pico_pins_init(uint8_t swclk_pin, uint8_t swdio_pin, uint8_t clock_pin){


    swd_pin_swdio = swdio_pin;
    swd_pin_swclk = swclk_pin;
    swd_pin_sysclk = clock_pin;


}

void grid_esp32_swd_pico_clock_init(uint8_t timer_instance, uint8_t channel_instance){


    ledc_timer_config_t pwm_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = timer_instance,
        .freq_hz = 12000000,
        .duty_resolution = LEDC_TIMER_2_BIT,
        .clk_cfg = LEDC_AUTO_CLK
    };

    ESP_ERROR_CHECK(ledc_timer_config(&pwm_config));

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = channel_instance,
        .timer_sel      = timer_instance,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = swd_pin_sysclk,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 2));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));


}

void grid_esp32_swd_pico_program_sram(uint8_t swclk_pin, uint8_t swdio_pin, uint8_t* buffer, uint32_t length){




    gpio_set_direction(swclk_pin, GPIO_MODE_OUTPUT);
    gpio_ll_set_level(&GPIO, swclk_pin, 0);

    gpio_ll_set_level(&GPIO, swdio_pin, 1);
    gpio_set_direction(swdio_pin, GPIO_MODE_OUTPUT);




    ets_delay_us(100000ul);

    // initialization
    if (1){
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
        gpio_ll_set_level(&GPIO, swd_pin_swdio, 0);
        for(uint8_t i=0; i<8; i++){
            gpio_ll_set_level(&GPIO, swd_pin_swclk, 1);
            ets_delay_us(SWD_CLOCK_PERIOD);
            gpio_ll_set_level(&GPIO, swd_pin_swclk, 0);
            ets_delay_us(SWD_CLOCK_PERIOD);

        }

        swd_linereset();


        ets_delay_us(SWD_CLOCK_PERIOD*5);

        // IDLE
        gpio_ll_set_level(&GPIO, swd_pin_swdio, 0);
        for(uint8_t i=0; i<8; i++){
            gpio_ll_set_level(&GPIO, swd_pin_swclk, 1);
            ets_delay_us(SWD_CLOCK_PERIOD);
            gpio_ll_set_level(&GPIO, swd_pin_swclk, 0);
            ets_delay_us(SWD_CLOCK_PERIOD);

        }
        

        swd_dummy_clock();
        swd_target_select(0);
        swd_dummy_clock();

        swd_read_idcode();
        swd_dummy_clock();
        
        swd_write_abort(0x0000001e);
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

    }


    ets_delay_us(5000ul);


    // reset
    if (1){
        swd_linereset();
        swd_idle();
        swd_target_select(0); swd_dummy_clock();
        swd_read_idcode(); swd_dummy_clock();
        swd_write_abort(0x00000010);    swd_dummy_clock();
        swd_read_ctrlstat();            swd_dummy_clock(); //0x00000001 
        swd_read_ap0();                 swd_dummy_clock(); //0x0
        swd_read_buff();                swd_dummy_clock(); //0x40001

        swd_linereset();
        swd_idle();
        swd_target_select(1); swd_dummy_clock();
        swd_read_idcode(); swd_dummy_clock();
        swd_write_abort(0x00000010);    swd_dummy_clock();
        swd_read_ctrlstat();            swd_dummy_clock(); //0x10000001 
        swd_read_ap0();                 swd_dummy_clock(); //0x0
        swd_read_buff();                swd_dummy_clock(); //0x40001

        swd_linereset();
        swd_idle();
        swd_target_select(0); swd_dummy_clock();
        swd_read_idcode(); swd_dummy_clock();
        swd_write_abort(0x00000010);    swd_dummy_clock();
        swd_read_ctrlstat();            swd_dummy_clock(); //0x00000001 
        swd_write_select(0x000000f3);   swd_dummy_clock();
        swd_read_apc();                
        swd_idle();
        swd_read_buff();                swd_dummy_clock(); //0x04770031 
        swd_write_select(0x00000003);   swd_dummy_clock();
        swd_write_ap0(0xa2000020);      swd_dummy_clock();
        swd_write_ap4(0x00000000);      swd_dummy_clock();
        swd_read_ap0();                 swd_dummy_clock(); //0x0
        swd_read_buff();                swd_dummy_clock(); //0x03000040
        swd_write_select(0x000000f3);   swd_dummy_clock();
        swd_read_ap4();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0x00

        swd_linereset();
        swd_idle();
        swd_target_select(1); swd_dummy_clock();
        swd_read_idcode(); swd_dummy_clock();
        swd_write_abort(0x00000010);    swd_dummy_clock();
        swd_read_dlcr();                swd_dummy_clock(); //0x10000001 
        swd_write_select(0x000000f3);   swd_dummy_clock();
        swd_read_apc();                
        swd_idle();
        swd_read_buff();                swd_dummy_clock(); //0x04770031 
        swd_write_select(0x00000003);   swd_dummy_clock();
        swd_write_ap0(0xa2000020);      swd_dummy_clock();
        swd_write_ap4(0x00000000);      swd_dummy_clock();
        swd_read_ap0();                 swd_dummy_clock(); //0x0
        swd_read_buff();                swd_dummy_clock(); //0x03000040
        swd_write_select(0x000000f3);   swd_dummy_clock();
        swd_read_ap4();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0x00

        swd_linereset();
        swd_idle();
        swd_target_select(0); swd_dummy_clock();
        swd_read_idcode(); swd_dummy_clock();
        swd_write_abort(0x00000010);    swd_dummy_clock();
        swd_read_dlcr();                swd_dummy_clock(); //0x00000001 
        swd_write_select(0x00000003);   swd_dummy_clock();
        swd_write_ap0(0xa2000022);      swd_dummy_clock();
        swd_write_ap4(0xe000edf0);      swd_dummy_clock();
        swd_write_select(0x00000013);   swd_dummy_clock();
        swd_read_ap0();                 swd_dummy_clock(); //0x00 
        swd_read_buff();                swd_dummy_clock(); //0x40001
        swd_write_ap0(0xa05f0003);      swd_dummy_clock();
        swd_write_ap8(0x00000000);      swd_dummy_clock();
        swd_write_ap0(0xa05f0003);      swd_dummy_clock();
        swd_write_select(0x00000003);   swd_dummy_clock();
        swd_write_ap4(0xe000ed30);      swd_dummy_clock();
        swd_write_select(0x00000013);   swd_dummy_clock();
        swd_read_ap0();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0x01
        swd_write_ap0(0x00000001);      swd_dummy_clock();
        swd_write_select(0x00000003);   swd_dummy_clock();
        swd_write_ap4(0xe000edf0);      swd_dummy_clock();
        swd_write_select(0x00000013);   swd_dummy_clock();
        swd_write_ap0(0xa05f0001);      swd_dummy_clock();
        swd_write_select(0x00000003);   swd_dummy_clock();
        swd_write_ap4(0xe000ed00);      swd_dummy_clock();
        swd_write_select(0x00000013);   swd_dummy_clock();
        swd_write_apc(0x05fa0004);      swd_dummy_clock();
        swd_write_select(0x00000010);   swd_dummy_clock();
        swd_read_ctrlstat();            swd_dummy_clock(); //0xf0000041 
        swd_write_select(0x00000000);   swd_dummy_clock();
        swd_write_ctrlstat(0x50000020); swd_dummy_clock();
        swd_read_ctrlstat();            swd_dummy_clock(); //0xf0000040
        swd_write_ctrlstat(0x50000000); swd_dummy_clock();
        swd_read_ctrlstat();            swd_dummy_clock(); //0xf0000040
        swd_read_ctrlstat();            swd_dummy_clock(); //0xf0000040
        swd_read_ctrlstat();            swd_dummy_clock(); //0xf0000040
        swd_write_ctrlstat(0x50000001); swd_dummy_clock();
        swd_read_ctrlstat();            swd_dummy_clock(); //0xf0000041
        swd_write_ap0(0xa2000002);      swd_dummy_clock();
        swd_write_ap4(0xe000ed00);      swd_dummy_clock();
        swd_write_select(0x00000010);   swd_dummy_clock();
        swd_read_apc();                 
        swd_idle();
        swd_read_buff();                swd_dummy_clock(); //0xfa050000
        //END OF FIRST BIG BLOCK

        swd_linereset();
        swd_idle();
        swd_target_select(1); swd_dummy_clock();
        swd_read_idcode(); swd_dummy_clock();
        swd_write_abort(0x00000010);    swd_dummy_clock();
        swd_read_dlcr();                swd_dummy_clock(); //0x10000001 
        swd_write_select(0x00000003);   swd_dummy_clock();
        swd_write_ap0(0xa2000022);      swd_dummy_clock();
        swd_write_ap4(0xe000edf0);      swd_dummy_clock();
        swd_write_select(0x00000013);   swd_dummy_clock();
        swd_read_ap0();                 swd_dummy_clock(); //0x00 
        swd_read_buff();                swd_dummy_clock(); //0x40001
        swd_write_ap0(0xa05f0003);      swd_dummy_clock();
        swd_write_ap8(0x00000000);      swd_dummy_clock();
        swd_write_ap0(0xa05f0003);      swd_dummy_clock();
        swd_write_select(0x00000003);   swd_dummy_clock();
        swd_write_ap4(0xe000ed30);      swd_dummy_clock();
        swd_write_select(0x00000013);   swd_dummy_clock();
        swd_read_ap0();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0x01
        swd_write_ap0(0x00000001);      swd_dummy_clock();
        swd_write_select(0x00000003);   swd_dummy_clock();
        swd_write_ap4(0xe000edf0);      swd_dummy_clock();
        swd_write_select(0x00000013);   swd_dummy_clock();
        swd_write_ap0(0xa05f0001);      swd_dummy_clock();
        swd_write_select(0x00000003);   swd_dummy_clock();
        swd_write_ap4(0xe000ed00);      swd_dummy_clock();
        swd_write_select(0x00000013);   swd_dummy_clock();
        swd_write_apc(0x05fa0004);      swd_dummy_clock();
        swd_write_select(0x00000010);   swd_dummy_clock();
        swd_read_ctrlstat();            swd_dummy_clock(); //0xf0000041 
        swd_write_select(0x00000000);   swd_dummy_clock();
        swd_write_ctrlstat(0x50000020); swd_dummy_clock();
        swd_read_ctrlstat();            swd_dummy_clock(); //0xf0000040
        swd_write_ctrlstat(0x50000000); swd_dummy_clock();
        swd_read_ctrlstat();            swd_dummy_clock(); //0xf0000040
        swd_read_ctrlstat();            swd_dummy_clock(); //0xf0000040
        swd_read_ctrlstat();            swd_dummy_clock(); //0xf0000040
        swd_write_ctrlstat(0x50000001); swd_dummy_clock();
        swd_read_ctrlstat();            swd_dummy_clock(); //0xf0000041
        swd_write_ap0(0xa2000002);      swd_dummy_clock();
        swd_write_ap4(0xe000ed00);      swd_dummy_clock();
        swd_write_select(0x00000010);   swd_dummy_clock();
        swd_read_apc();                 
        swd_idle();
        swd_read_buff();                swd_dummy_clock(); //0xfa050000
        //END OF SECOND BIG BLOCK

        swd_linereset();
        swd_idle();
        swd_target_select(0); swd_dummy_clock();
        swd_read_idcode(); swd_dummy_clock();
        swd_write_abort(0x00000010);    swd_dummy_clock();
        swd_write_select(0x00000013);   swd_dummy_clock();
        swd_read_dlcr();                swd_dummy_clock(); //0x00000001 
        swd_write_select(0x00000003);   swd_dummy_clock();
        swd_write_ap4(0xe000edf0);      swd_dummy_clock();
        swd_write_select(0x00000013);   swd_dummy_clock();
        swd_read_ap0();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0x03040001

        swd_linereset();
        swd_idle();
        swd_target_select(1); swd_dummy_clock();
        swd_read_idcode(); swd_dummy_clock();
        swd_write_abort(0x00000010);    swd_dummy_clock();
        swd_write_select(0x00000013);   swd_dummy_clock();
        swd_read_dlcr();                swd_dummy_clock(); //0x10000001 
        swd_write_select(0x00000003);   swd_dummy_clock();
        swd_write_ap4(0xe000edf0);      swd_dummy_clock();
        swd_write_select(0x00000013);   swd_dummy_clock();
        swd_read_ap0();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0x03040001
        // END OF THIRD BLOCK

        swd_linereset();
        swd_idle();
        swd_target_select(0); swd_dummy_clock();
        swd_read_idcode(); swd_dummy_clock();
        swd_write_abort(0x00000010);    swd_dummy_clock();
        swd_read_dlcr();                swd_dummy_clock(); //0x00000001 
        swd_read_ap0();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0x00040001
        swd_read_apc();             
        swd_idle();           
        swd_read_buff();                swd_dummy_clock(); //0x0x01000000
        swd_write_ap8(0x00000000);      swd_dummy_clock();
        swd_read_ap0();                 swd_dummy_clock(); //0xe00ff003
        swd_read_buff();                swd_dummy_clock(); //0x00040001
        swd_write_apc(0x01000000);      swd_dummy_clock();
        swd_write_select(0x00000003);   swd_dummy_clock();
        swd_write_ap0(0xa2000012);      swd_dummy_clock();
        swd_write_ap4(0xe0002000);      swd_dummy_clock();
        swd_write_apc(0x00000003);      swd_dummy_clock();
        swd_write_ap4(0xe0002000);      swd_dummy_clock();
        swd_read_apc();             
        swd_idle();           
        swd_read_buff();                swd_dummy_clock(); //0x00000041
        swd_write_ap4(0xe0002008);      swd_dummy_clock();
        swd_write_apc(0x00000000);      swd_dummy_clock();
        swd_write_apc(0x00000000);      swd_dummy_clock();
        swd_write_apc(0x00000000);      swd_dummy_clock();
        swd_write_apc(0x00000000);      swd_dummy_clock();
        swd_write_ap4(0xe0001020);      swd_dummy_clock();
        swd_write_apc(0x00000000);      swd_dummy_clock();
        swd_write_apc(0x00000000);      swd_dummy_clock();
        swd_write_apc(0x00000000);      swd_dummy_clock();
        swd_write_ap4(0xe0001030);      swd_dummy_clock();
        swd_write_apc(0x00000000);      swd_dummy_clock();
        swd_write_apc(0x00000000);      swd_dummy_clock();
        swd_write_apc(0x00000000);      swd_dummy_clock();
        swd_write_ap4(0xe000edf0);      swd_dummy_clock();
        swd_write_select(0x00000013);   swd_dummy_clock();
        swd_read_ap0();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0x00040001

        swd_linereset();
        swd_idle();
        swd_target_select(1); swd_dummy_clock();
        swd_read_idcode(); swd_dummy_clock();
        swd_write_abort(0x00000010);    swd_dummy_clock();
        swd_read_dlcr();                swd_dummy_clock(); //0x10000001 
        swd_read_ap0();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0x00040001
        swd_read_apc();             
        swd_idle();           
        swd_read_buff();                swd_dummy_clock(); //0x0x01000000
        swd_write_ap8(0x00000000);      swd_dummy_clock();
        swd_read_ap0();                 swd_dummy_clock(); //0xe00ff003
        swd_read_buff();                swd_dummy_clock(); //0x00040001
        swd_write_apc(0x01000000);      swd_dummy_clock();
        swd_write_select(0x00000003);   swd_dummy_clock();
        swd_write_ap0(0xa2000012);      swd_dummy_clock();
        swd_write_ap4(0xe0002000);      swd_dummy_clock();
        swd_write_apc(0x00000003);      swd_dummy_clock();
        swd_write_ap4(0xe0002000);      swd_dummy_clock();
        swd_read_apc();             
        swd_idle();           
        swd_read_buff();                swd_dummy_clock(); //0x00000041
        swd_write_ap4(0xe0002008);      swd_dummy_clock();
        swd_write_apc(0x00000000);      swd_dummy_clock();
        swd_write_apc(0x00000000);      swd_dummy_clock();
        swd_write_apc(0x00000000);      swd_dummy_clock();
        swd_write_apc(0x00000000);      swd_dummy_clock();
        swd_write_ap4(0xe0001020);      swd_dummy_clock();
        swd_write_apc(0x00000000);      swd_dummy_clock();
        swd_write_apc(0x00000000);      swd_dummy_clock();
        swd_write_apc(0x00000000);      swd_dummy_clock();
        swd_write_ap4(0xe0001030);      swd_dummy_clock();
        swd_write_apc(0x00000000);      swd_dummy_clock();
        swd_write_apc(0x00000000);      swd_dummy_clock();
        swd_write_apc(0x00000000);      swd_dummy_clock();
        swd_write_ap4(0xe000edf0);      swd_dummy_clock();
        swd_write_select(0x00000013);   swd_dummy_clock();
        swd_read_ap0();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0x00040001



    }
   
    ets_delay_us(5000ul);

    // halt
    if (1){
        swd_linereset();
        swd_idle();
        swd_target_select(0); swd_dummy_clock();
        swd_read_idcode(); swd_dummy_clock();
        swd_write_abort(0x00000010);    swd_dummy_clock();
        swd_read_ctrlstat();            swd_dummy_clock(); //0x00000001 
        swd_read_ap0();                 swd_dummy_clock(); //0x0
        swd_read_buff();                swd_dummy_clock(); //0x40001

        swd_linereset();
        swd_idle();
        swd_target_select(1); swd_dummy_clock();
        swd_read_idcode(); swd_dummy_clock();
        swd_write_abort(0x00000010);    swd_dummy_clock();
        swd_read_ctrlstat();            swd_dummy_clock(); //0x10000001 
        swd_read_ap0();                 swd_dummy_clock(); //0x0
        swd_read_buff();                swd_dummy_clock(); //0x40001

        swd_linereset();
        swd_idle();
        swd_target_select(0); swd_dummy_clock();
        swd_read_idcode(); swd_dummy_clock();
        swd_write_abort(0x00000010);    swd_dummy_clock();
        swd_read_ctrlstat();            swd_dummy_clock(); //0x00000001 
        swd_write_ap0(0xa05f0003);      swd_dummy_clock();
        swd_read_ap0();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0x01030003
        swd_write_ap0(0xa05f0003);      swd_dummy_clock();
        swd_write_select(0x00000003);   swd_dummy_clock();
        swd_write_ap4(0xe000ed30);      swd_dummy_clock();
        swd_write_select(0x00000013);   swd_dummy_clock();
        swd_read_ap0();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0x01
        swd_write_ap0(0x00000001);      swd_dummy_clock();
        swd_write_select(0x00000003);   swd_dummy_clock();    
        swd_write_ap4(0xe000edf0);      swd_dummy_clock();
        swd_write_select(0x00000013);   swd_dummy_clock();    
        swd_read_ap0();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0x00030003
        swd_write_ap4(0x00000000);      swd_dummy_clock();
        swd_read_ap8();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0x501008b0
        swd_write_ap4(0x00000001);      swd_dummy_clock();
        swd_read_ap8();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0x00
        swd_write_ap4(0x00000002);      swd_dummy_clock();
        swd_read_ap8();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0x3ecc
        swd_write_ap4(0x00000003);      swd_dummy_clock();
        swd_read_ap8();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0x00
        swd_write_ap4(0x00000004);      swd_dummy_clock();
        swd_read_ap8();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0x50100e84
        swd_write_ap4(0x00000005);      swd_dummy_clock();
        swd_read_ap8();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0x00
        swd_write_ap4(0x00000006);      swd_dummy_clock();
        swd_read_ap8();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0x00
        swd_write_ap4(0x00000007);      swd_dummy_clock();
        swd_read_ap8();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0x50100eac
        swd_write_ap4(0x00000008);      swd_dummy_clock();
        swd_read_ap8();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0xffffffff
        swd_write_ap4(0x00000009);      swd_dummy_clock();
        swd_read_ap8();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0xffffffff
        swd_write_ap4(0x0000000a);      swd_dummy_clock();
        swd_read_ap8();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0xffffffff
        swd_write_ap4(0x0000000b);      swd_dummy_clock();
        swd_read_ap8();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0xffffffff
        swd_write_ap4(0x0000000c);      swd_dummy_clock();
        swd_read_ap8();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0x50100000
        swd_write_ap4(0x0000000d);      swd_dummy_clock();
        swd_read_ap8();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0x50100898
        swd_write_ap4(0x0000000e);      swd_dummy_clock();
        swd_read_ap8();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0x000002b3
        swd_write_ap4(0x0000000f);      swd_dummy_clock();
        swd_read_ap8();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0x000020f0
        swd_write_ap4(0x00000010);      swd_dummy_clock();
        swd_read_ap8();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0x61000000
        swd_write_ap4(0x00000011);      swd_dummy_clock();
        swd_read_ap8();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0x50100898
        swd_write_ap4(0x00000012);      swd_dummy_clock();
        swd_read_ap8();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0xfffffffc
        swd_write_ap4(0x00000014);      swd_dummy_clock();
        swd_read_ap8();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0x00000000

        swd_linereset();
        swd_idle();
        swd_target_select(1); swd_dummy_clock();
        swd_read_idcode(); swd_dummy_clock();
        swd_write_abort(0x00000010); swd_dummy_clock();
        swd_read_dlcr(); swd_dummy_clock(); // 0x10000001
        swd_read_ap0(); swd_dummy_clock(); // 0x00
        swd_read_buff(); swd_dummy_clock();  //0x00040001


    }

    ets_delay_us(5000ul);

    // program_sram
    if (1){

         // reandom idle
/*

        swd_linereset();
        swd_idle();
        swd_target_select(0); swd_dummy_clock();
        swd_read_idcode(); swd_dummy_clock();
        swd_write_abort(0x00000010);    swd_dummy_clock();
        swd_read_dlcr();            swd_dummy_clock(); //0x00000001 
        swd_read_ap0();            swd_dummy_clock(); //0x00000000
        swd_read_buff();                swd_dummy_clock(); //0x30003

        swd_linereset();
        swd_idle();
        swd_target_select(1); swd_dummy_clock();
        swd_read_idcode(); swd_dummy_clock();
        swd_write_abort(0x00000010);    swd_dummy_clock();
        swd_read_dlcr();            swd_dummy_clock(); //0x10000001 
        swd_read_ap0();            swd_dummy_clock(); //0x00000000
        swd_read_buff();                swd_dummy_clock(); //0x00040001
*/

        // start programming

        swd_linereset();
        swd_idle();
        swd_target_select(0); swd_dummy_clock();
        swd_read_idcode(); swd_dummy_clock();
        swd_write_abort(0x00000010);    swd_dummy_clock();
        swd_read_ctrlstat();            swd_dummy_clock(); //0x00000001 
        swd_read_ap0();            swd_dummy_clock(); //0x00000000
        swd_read_buff();                swd_dummy_clock(); //0x30003

        swd_linereset();
        swd_idle();
        swd_target_select(1); swd_dummy_clock();
        swd_read_idcode(); swd_dummy_clock();
        swd_write_abort(0x00000010);    swd_dummy_clock();
        swd_read_ctrlstat();            swd_dummy_clock(); //0x10000001 
        swd_read_ap0();            swd_dummy_clock(); //0x00000000
        swd_read_buff();                swd_dummy_clock(); //0x00040001

        swd_linereset();
        swd_idle();
        swd_target_select(0); swd_dummy_clock();
        swd_read_idcode(); swd_dummy_clock();
        swd_write_abort(0x00000010);    swd_dummy_clock();
        swd_read_ctrlstat();            swd_dummy_clock(); //0x00000001 
        swd_write_select(0x00000003);   swd_dummy_clock();
        swd_write_ap4(0x20000000);      swd_dummy_clock();

        printf("\r\n START OF DUMP \r\n");

        // burst size: how may words in page
        #define BURST_SIZE 256
        #define BIN_SIZE 18060



        swd_write_select(0x00000003);   swd_dummy_clock();
        for (uint32_t i = 0; i<length; i+=4){

            if (i%0x400 == 0){
                swd_write_ap4(0x20000000+i);      swd_dummy_clock();
                //printf("\r\n\r\n%08lx :: \r\n", i);
            }

            uint32_t word = *(uint32_t*)(buffer+i);
            swd_write_apc(word); // 0x491c481b

            //printf(" %08lx", word);

        }

        /*

        swd_write_select(0x00000003);   swd_dummy_clock();
        for(uint32_t i = 0; i<BIN_SIZE+BURST_SIZE; i+=4*BURST_SIZE) {

            uint8_t buffer[4*BURST_SIZE] = {0};
            esp_flash_read(NULL, buffer, 0x450000 + i, 4*BURST_SIZE);  

            // select next page
            swd_write_ap4(0x20000000+i);      swd_dummy_clock();

            printf("\r\n\r\n%08lx :: \r\n", i);

            for (uint32_t j=0; j<BURST_SIZE; j++){

                if (i + j*4<BIN_SIZE){ // BIN_SIZE

                    uint32_t word = *(uint32_t*)(buffer+j*4);
                    swd_write_apc(word); // 0x491c481b
                    //printf(" %08lx", word);

                }


            }



        }
        */

        printf("\r\n END OF DUMP \r\n");


        swd_write_ap4(0xe000edf0);      swd_dummy_clock();
        swd_write_select(0x00000013);   swd_dummy_clock();
        swd_read_ap0();                 swd_dummy_clock(); //0x00
        swd_read_buff();                swd_dummy_clock(); //0x000030003


        swd_linereset();
        swd_idle();
        swd_target_select(1); swd_dummy_clock();
        swd_read_idcode(); swd_dummy_clock();
        swd_write_abort(0x00000010);    swd_dummy_clock();
        swd_read_dlcr();            swd_dummy_clock(); //0x10000001 
        swd_read_ap0();            swd_dummy_clock(); //0x00000000
        swd_read_buff();                swd_dummy_clock(); //0x00040001

    }


    ets_delay_us(5000ul);
    


    // resume 0x200000000
    if (1){
        swd_linereset();
        swd_idle();
        swd_target_select(0); swd_dummy_clock();
        swd_read_idcode(); swd_dummy_clock();
        swd_write_abort(0x00000010);    swd_dummy_clock();
        swd_read_ctrlstat();            swd_dummy_clock(); //0x00000001 
        swd_read_ap0();                 swd_dummy_clock(); //0x0
        swd_read_buff();                swd_dummy_clock(); //0x30003



        swd_linereset();
        swd_idle();
        swd_target_select(1); swd_dummy_clock();
        swd_read_idcode(); swd_dummy_clock();
        swd_write_abort(0x00000010);    swd_dummy_clock();
        swd_read_ctrlstat();            swd_dummy_clock(); //0x10000001 
        swd_read_ap0();                 swd_dummy_clock(); //0x0
        swd_read_buff();                swd_dummy_clock(); //0x30003

        // 3rd

        swd_linereset();
        swd_idle();
        swd_target_select(0); swd_dummy_clock();
        swd_read_idcode(); swd_dummy_clock();
        swd_write_abort(0x00000010);    swd_dummy_clock();
        swd_read_ctrlstat();            swd_dummy_clock(); //0x00000001
        swd_write_ap8(0x20000000);      swd_dummy_clock();
        swd_write_ap4(0x0001000f);      swd_dummy_clock();

        // 4th

        swd_linereset();
        swd_idle();
        swd_target_select(1); swd_dummy_clock();
        swd_read_idcode(); swd_dummy_clock();
        swd_write_abort(0x00000010);    swd_dummy_clock();
        swd_read_ctrlstat();            swd_dummy_clock(); //0x10000001 
        swd_write_ap0(0xa05f0001);      swd_dummy_clock();

        // 5th

        swd_linereset();
        swd_idle();
        swd_target_select(0); swd_dummy_clock();
        swd_read_idcode(); swd_dummy_clock();
        swd_write_abort(0x00000010);    swd_dummy_clock();
        swd_read_ctrlstat();            swd_dummy_clock(); //0x01 
        swd_write_ap0(0xa05f0001);      swd_dummy_clock();
    }








}