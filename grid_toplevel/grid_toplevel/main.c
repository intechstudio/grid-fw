#include <atmel_start.h>
#include "../../grid_lib/grid_led.c" // WS2812 LED
#include "../../grid_lib/grid_ain.c" // Analog input filtering
#include "../../grid_lib/grid_tel.c" // Grid Telemetry

#define GRID_MODULE_P16

#include "../../grid_modules/grid_module_p16.c" // Grid Telemetry


int main(void)
{
	/* Initializes MCU, drivers and middleware */
	
	atmel_start_init();
	
	grid_module_init();
			
	grid_led_init(grid_module_led_buffer_size);
	
	
	
	// FLASH EFFECT
			
	grid_led_set_min(3, 1, 0x00, 0x00, 0x30);
	grid_led_set_mid(3, 1, 0x30, 0x00, 0x00);
	grid_led_set_max(3, 1, 0x00, 0x00, 0x00);
			
	grid_led_set_frequency(3, 1, 4);
			
				
	// Allocate memory for 4 analog input with the filter depth of 3 samples, 14 bit format, 10bit result resolution
	grid_ain_init(grid_module_ain_buffer_size, 3, 14, 10);


	// UI RX EVENT fref=5, alert=50;
	struct TEL_event_counter* console_tx = grid_tel_event_register(5, 50);
	while(console_tx == NULL){/*TRAP*/}	
	
	char str[26];
	sprintf(str, "FreqRef: %5d\n", grid_tel_event_head->frequency);

	//USART
	io_write(io, str, 26);

	uint32_t faketimer = 0;

	uint8_t multiplexer = 0;

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
				sprintf(str, "ADC: %5d %5d %5d\n", i, average/128, console_tx->frequency);

				//USART
				io_write(io, str, 26);
							
				grid_led_set_phase(i, 0, average/8/4/4);
				
				
			}
			
		}
	
		gpio_toggle_pin_level(LED0);

		
		// ================ WS2812B VIA DMA SPI ================== //
					
	
		delay_ms(1);
		

				
		grid_led_tick();
		
		// RENDER ALL OF THE LEDs
		grid_led_render_all();
			
		// SEND DATA TO LEDs 		
		dma_spi_done = 0;
		spi_m_dma_enable(&GRID_LED);
		
		io_write(io2, grid_led_frame_buffer_pointer(), grid_led_frame_buffer_size());
		
 		while (dma_spi_done == 0)
 		{
 		}		
		
		
		
	

		
		
		
		
		
	
	}
}
