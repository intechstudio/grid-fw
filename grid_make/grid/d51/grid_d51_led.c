/*
 * grid_d51_led.c
 *
 * Created: 6/3/2020 5:02:14 PM
 *  Author: WPC-User
 */ 

#include "grid_d51_led.h"

struct grid_d51_led_model grid_d51_led_state;


uint32_t grid_led_color_code[256];

void grid_d51_led_init(struct grid_d51_led_model* d51_mod, struct grid_led_model* led_mod){

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
	
		temp |= (i/1%2)   ? (GRID_D51_LED_CODE_O<<24) : (GRID_D51_LED_CODE_Z<<24);
		temp |= (i/2%2)   ? (GRID_D51_LED_CODE_O<<28) : (GRID_D51_LED_CODE_Z<<28);
	
		temp |= (i/4%2)   ? (GRID_D51_LED_CODE_O<<16) : (GRID_D51_LED_CODE_Z<<16);
		temp |= (i/8%2)   ? (GRID_D51_LED_CODE_O<<20) : (GRID_D51_LED_CODE_Z<<20);
	
		temp |= (i/16%2)  ? (GRID_D51_LED_CODE_O<<8)  : (GRID_D51_LED_CODE_Z<<8);
		temp |= (i/32%2)  ? (GRID_D51_LED_CODE_O<<12) : (GRID_D51_LED_CODE_Z<<12);
		temp |= (i/64%2)  ? (GRID_D51_LED_CODE_O<<0)  : (GRID_D51_LED_CODE_Z<<0);
		temp |= (i/128%2) ? (GRID_D51_LED_CODE_O<<4)  : (GRID_D51_LED_CODE_Z<<4);
	
		grid_led_color_code[i] = temp;
	}
	



	d51_mod->led_count = led_mod->led_count;


	d51_mod->framebuffer_size = (GRID_D51_LED_RESET_LENGTH + led_mod->led_count*3*4);
	d51_mod->framebuffer = (uint8_t*) malloc(d51_mod->framebuffer_size * sizeof(uint8_t));

	// Fill the first 24 bytes with the rr_code (reset)
	// This memory is essentially wasted but allows the entire frame to be sent out using DMA

	for (uint8_t i = 0; i<GRID_D51_LED_RESET_LENGTH; i++){
		d51_mod->framebuffer[i] = GRID_D51_LED_CODE_R;
	}

	// Fill the rest of the buffer with rgb=(0,0,0);
	for (uint32_t i = 0; i<led_mod->led_count; i++){
		grid_d51_led_set_color(d51_mod,i,0,0,0);
	}

	grid_led_startup_animation(&d51_mod);


}


void grid_d51_led_set_color(struct grid_d51_led_model* mod, uint32_t led_index, uint8_t r, uint8_t g, uint8_t b){

	uint32_t* framebuffer_usable = (uint32_t*) &mod->framebuffer[GRID_D51_LED_RESET_LENGTH];

	framebuffer_usable[led_index*3 + 0] = grid_led_color_code[g];
	framebuffer_usable[led_index*3 + 1] = grid_led_color_code[r];
	framebuffer_usable[led_index*3 + 2] = grid_led_color_code[b];


}



void grid_d51_led_generate_frame(struct grid_d51_led_model* d51_mod, struct grid_led_model* led_mod){

	for(uint32_t i = 0; i<d51_mod->led_count; i++){

		uint8_t gre = led_mod->led_frame_buffer[i*3 + 0];
		uint8_t red = led_mod->led_frame_buffer[i*3 + 1];
		uint8_t blu = led_mod->led_frame_buffer[i*3 + 2];



		grid_d51_led_set_color(d51_mod, i, red, gre, blu);


	}


}

void grid_d51_led_start_transfer(struct grid_d51_led_model* mod){

	spi_m_dma_enable(&GRID_LED);


	struct io_descriptor* io_descr;

	spi_m_dma_get_io_descriptor(&GRID_LED, &io_descr);
	io_write(io_descr, mod->framebuffer, mod->framebuffer_size);

	
}


void grid_led_startup_animation(struct grid_d51_led_model* mod){

	return;

	uint8_t grid_module_reset_cause = hri_rstc_read_RCAUSE_reg(RSTC);
	
	uint8_t color_r   = 1;
	uint8_t color_g   = 1;
	uint8_t color_b   = 1;
	uint8_t s		  = 1;
	
	if (grid_module_reset_cause == RESET_REASON_WDT){
		
		color_r = 1;
		color_g = 0;
		color_b = 0;
		s= 2;

	}else if (grid_module_reset_cause == RESET_REASON_SYST){
		
		color_r = 0;
		color_g = 0;
		color_b = 1;
		s= 2;
		
	}
	
	
	
	for (uint8_t i = 0; i<255; i++){
	
		// SEND DATA TO LEDs
		

		for (uint8_t j=0; j<mod->led_count; j++){
			//grid_led_lowlevel_set_color(i, 0, 255, 0);
			grid_d51_led_set_color(mod, j, (color_r*i*s%256)/2, (color_g*i*s%256)/2, (color_b*i*s%256)/2); // This is not an alert, this is low level shit
			
			
		}
		
		grid_d51_led_start_transfer(mod);

		
		delay_us(500);
		
	}
	

	

	
}



