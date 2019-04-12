/*
 * grid_led.c
 *
 * Created: 4/12/2019 5:27:13 PM
 * Author : SUKU WC
 
 // ===================== WS2812B-Mini SETUP ========================= //

 // The aim is to create a fast WS2812 library optimized for DMA transfers through the SPI interface.
 // Primary goal is to reduce CPU load allowing efficient animations.
 // Memory usage is less of a concern in this implementation.
 //
 // * Each symbol is encoded into a 4bits of serial data.
 //
 // * One LED uses 12 bytes of memory
 // * The reset pulse uses 24 bytes of memory
 */ 


// FRAME FORMAT for WS2812B-Mini
//
// Memory map =>   | <------------------------------------ FRAME BUFFER ------------------------------------------> |
// Memory map =>   |       | <---------------------------- LED DATA BUFFER ---------------------------------------> |
// Size in bytes=> | 24    | 4      | 4      | 4      | 4      | 4      | 4      | 4      | 4      | 4      | 4     |
// Frame format => | RESET | LED1:G | LED1:R | LED1:B | LED2:G | LED2:R | LED2:B | LED3:G | LED3:R | LED3:B | IDLE  |
// Frame timing => | 60us  | 10us   | 10us   | 10us   | 10us   | 10us   | 10us   | 10us   | 10us   | 10us   | N/A   |

// this is need for types like uint8_t 
#include "grid_led.h"

// Two symbols are encoded in one byte, the 4 possible combinations are defined here.
// THe NZR code specification for WS2812B-Mini allows a 0.150us deviation from the nominal pulse widths.
//
// * Zero code nominal: 0.40 + 0.85 (us)
// * Zero code actual:  0.31 + 0.93 (us)
//
// * One code nominal: 0.85 + 0.40 (us)
// * One code actual:  0.93 + 0.31 (us)
#define LED_CODE_Z 0x08 // 0000 1000
#define LED_CODE_O 0x0E // 0000 1110


// Reset requires a 50us LOW pulse on the output.
// At the specified 3.2Mbps serial speed this requires 24 consecutive reset bytes (0x00) bytes to be sent out

#define LED_CODE_R 0x00 // 0000 0000


/** Get pointer of buffer array */
uint8_t* grid_led_buffer_pointer(void){
	
	return led_buffer;
}


/** Get led buffer size */
uint8_t grid_led_buffer_size(void){
	
	return led_buffer_size;
}

/** Set color of a particular LED */
uint8_t grid_led_set_color(uint32_t led_index, uint8_t led_r, uint8_t led_g, uint8_t led_b){
	
	//if index is valid
	if (led_index<led_number){
		
		led_buffer_usable[led_index*3 + 0] = led_color_code[led_g];
		led_buffer_usable[led_index*3 + 1] = led_color_code[led_r];
		led_buffer_usable[led_index*3 + 2] = led_color_code[led_b];
		
		return 0;
		
	}
	else{
		
		return -1;		
		
	}
}

/** Initialize led buffer for a given number of LEDs */
uint8_t grid_led_init(uint8_t num){
	
	led_number = num;	
	led_buffer_size = (24 + num*3*4);
	led_buffer = (uint8_t*) malloc(led_buffer_size * sizeof(uint8_t));

	led_buffer_usable = (uint32_t*) &led_buffer[24];
	
	// Generate the lookup table for fast rendering;
	
	// Generating the lookup table for 2 symbols per byte encoding
	//
	//  * Reading intensity values from lookup table reduces the CPU cycles used for calculating the fram buffer.
	//  * This is especially useful for fast animations using large number of LEDs.
	//
	//	* Calculating the lookup table during startup saves program memory at the expense of RAM
	//  * Memory usage: 256*32bit -> 1kbyte
	//
	//	* Input: 8bit intensity;
	//  * Output: 8bit intensity encoded into 4byte long NZR intensity code;
	//
	//	* The output can be sent directly to the RGB chip through SPI or GPIO.
	//  * Green Red and Blue intensity values are sent to the LEDs in the given order.
	//  * The output should have a baudrate of 3.2Mbps resulting in an effective symbol rate of 800k sym per sec
	//
	//	* MSB first (WS2812B-Mini)
	//	* G7 ... G0 R7 ... R0 B7 ... B0 ;
	
	for(uint16_t i=0; i<256; i++){
		
		uint32_t temp = 0;
		
		temp |= (i/1%2)   ? (LED_CODE_O<<24) : (LED_CODE_Z<<24);
		temp |= (i/2%2)   ? (LED_CODE_O<<28) : (LED_CODE_Z<<28);
		
		temp |= (i/4%2)   ? (LED_CODE_O<<16) : (LED_CODE_Z<<16);
		temp |= (i/8%2)   ? (LED_CODE_O<<20) : (LED_CODE_Z<<20);
		
		temp |= (i/16%2)  ? (LED_CODE_O<<8)  : (LED_CODE_Z<<8);
		temp |= (i/32%2)  ? (LED_CODE_O<<12) : (LED_CODE_Z<<12);
		temp |= (i/64%2)  ? (LED_CODE_O<<0)  : (LED_CODE_Z<<0);
		temp |= (i/128%2) ? (LED_CODE_O<<4)  : (LED_CODE_Z<<4);
		
		led_color_code[i] = temp;
	}

	// Fill the first 24 bytes with the rr_code (reset)
	// This memory is essentially wasted but allows the entire frame to be sent out using DMA

	for (uint8_t i = 0; i<24; i++){
		led_buffer[i] = LED_CODE_R;
	}
	
	
	// Fill the rest of the buffer with rgb=(0,0,0);
	for (uint32_t i = 0; i<led_number; i++){
		grid_led_set_color(i, 0,0,0);
	}


	return 0;
}


