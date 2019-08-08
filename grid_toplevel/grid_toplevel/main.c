#include <atmel_start.h>
#include "atmel_start_pins.h"

#include "../../grid_lib/grid_led.c" // WS2812 LED
#include "../../grid_lib/grid_ain.c" // Analog input filtering
#include "../../grid_lib/grid_tel.c" // Grid Telemetry

#define GRID_MODULE_P16

#include "../../grid_modules/grid_module_p16.c" // Grid Telemetry


int main(void)
{
	atmel_start_init();
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
		
		if (faketimer == 10){
			grid_tel_frequency_tick();
			faketimer = 0;
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
			
			grid_led_set_color(i, 0, 0, 0);
			grid_led_set_color(0, 255, 0, 0);
		
		
		}
	/*
		grid_led_set_color(4, colorfade*(colorcode==0), colorfade*(colorcode==1), colorfade*(colorcode==2));
		grid_led_set_color(0, colorfade*(colorcode==0), colorfade*(colorcode==1), colorfade*(colorcode==2));
		
		colorfade++;
		if (colorfade == 0) colorcode++;
		if (colorcode>2) colorcode=0;
		
	*/	
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
