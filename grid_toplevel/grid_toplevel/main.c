#include <atmel_start.h>
#include "atmel_start_pins.h"

#include "../../grid_lib/grid_led.c" // WS2812 LED
#include "../../grid_lib/grid_ain.c" // Analog input filtering
#include "../../grid_lib/grid_tel.c" // Grid Telemetry

#define GRID_MODULE_P16

#include "../../grid_modules/grid_module_p16.c" // Grid Telemetry


int main(void)
{
	//atmel_start_init();
	//cdcd_acm_example();
	
	
	
	/* Initializes MCU, drivers and middleware */
	
	atmel_start_init();
	
	grid_module_init();
	
	

	// UI RX EVENT fref=5, alert=50;
	
	struct TEL_event_counter* console_tx = grid_tel_event_register(5, 50);
	
	while(console_tx == NULL){/*TRAP*/}	


	uint32_t faketimer = 0;

	uint8_t colorfade = 0;
	uint8_t colorcode = 0;

	while (1) {
		
		if (faketimer > 100){
			grid_tel_frequency_tick();
			faketimer = 0;
			
			gpio_set_pin_direction(HWCFG_SHIFT, GPIO_DIRECTION_OUT);
			gpio_set_pin_direction(HWCFG_CLOCK, GPIO_DIRECTION_OUT);
			gpio_set_pin_direction(HWCFG_DATA, GPIO_DIRECTION_IN);
			
			// LOAD DATA
			gpio_set_pin_level(HWCFG_SHIFT, 0);	
			delay_ms(1);
			
					
			
			uint8_t hwcfg_value = 0;
			
			
			for(uint8_t i = 0; i<8; i++){ // now we need to shift in the remaining 7 values
				
				// SHIFT DATA
				gpio_set_pin_level(HWCFG_SHIFT, 1); //This outputs the first value to HWCFG_DATA
				delay_ms(1);
				
				
				if(gpio_get_pin_level(HWCFG_DATA)){
				
				 	hwcfg_value |= (1<<i);
				
				}else{
				
				
				}
				
				if(i!=7){
									
					// Clock rise
					gpio_set_pin_level(HWCFG_CLOCK, 1);
				
					delay_ms(1);
				
					gpio_set_pin_level(HWCFG_CLOCK, 0);
				}
					
					
				
			}
			
			
			// REPORT OVER SERIAL
			
			struct io_descriptor *io_uart_aux;
			
			usart_async_get_io_descriptor(&GRID_AUX, &io_uart_aux);
			usart_async_enable(&GRID_AUX);

			char example_GRID_AUX[12];
			
			sprintf(example_GRID_AUX, "HWCFG:%d\n", hwcfg_value);

			io_write(io_uart_aux, example_GRID_AUX, 12);

			
		}
		faketimer++;
		
		gpio_toggle_pin_level(LED0);

			
		/* ========================= ANALOG READ ============================= */
		
		delay_ms(1);


		
		// Push out all changes
		for (uint8_t i = 0; i<4; i++)
		{
			if (grid_ain_get_changed(i)){
				
				grid_tel_event_handler(console_tx);
				
				uint16_t average = grid_ain_get_average(i);
				
				char str[26];
				sprintf(str, "ADC: %5d %5d %5d \n", i, average, average/128);

				//USART
				io_write(io, str, 24);
				
								
				uint32_t crc = 0xFFFFFFFF;
 				crc_sync_crc32(&CRC_0, (uint32_t *)str, 5, &crc);
 				crc ^= 0xFFFFFFFF;
							
				delay_ms(5);
				
				char str2[26];
				sprintf(str2, "CRC: %x \n", crc);

				//USART
				io_write(io, str2, 15);

							
				//grid_led_set_phase(i, 0, average/8/4/4);
				
				
			}
			
		}
	
		gpio_toggle_pin_level(LED0);

		
		// ================ WS2812B VIA DMA SPI ================== //
					
	
		delay_ms(1);
		

				
		//grid_led_tick();
		
		
		
		// RENDER ALL OF THE LEDs
		//grid_led_render_all();
		
		
		for (uint8_t i=0; i<16; i++){
			
			//grid_led_set_color(i, 0, 255, 0);
			
			grid_led_set_color(i, colorfade*(colorcode==0), colorfade*(colorcode==1), colorfade*(colorcode==2));
		
		
		}
	
		colorfade++;
		if (colorfade == 0) colorcode++;
		if (colorcode>2) colorcode=0;
		
	
		delay_ms(2);
			
		// SEND DATA TO LEDs 		
		dma_spi_done = 0;
		spi_m_dma_enable(&GRID_LED);
		
		io_write(io2, grid_led_frame_buffer_pointer(), grid_led_frame_buffer_size());
		
 		while (dma_spi_done == 0)
 		{
 		}		
		
		

		
	}
}
