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
	
			/* CRC Data in flash */
		COMPILER_ALIGNED(4)
		static const uint32_t crc_datas[] = {0x00000000,
												0x11111111,
												0x22222222,
												0x33333333,
												0x44444444,
												0x55555555,
												0x66666666,
												0x77777777,
												0x88888888,
												0x99999999};


		crc_sync_enable(&CRC_0);

		/* The initial value used for the CRC32 calculation usually be 0xFFFFFFFF,
		 * but can be, for example, the result of a previous CRC32 calculation if
		 * generating a common CRC32 of separate memory blocks.
		 */
		uint32_t crc = 0xFFFFFFFF;
		uint32_t crc2;
		uint32_t ind;

		
		crc_sync_crc32(&CRC_0, (uint32_t *)crc_datas, 10, &crc);

		/* The read value must be complemented to match standard CRC32
		 * implementations or kept non-inverted if used as starting point for
		 * subsequent CRC32 calculations.
		 */
		crc ^= 0xFFFFFFFF;

		/* Calculate the same data with subsequent CRC32 calculations, the result
		 * should be same as previous way.
		 */
		crc2 = 0xFFFFFFFF;
		for (ind = 0; ind < 10; ind++) {
			crc_sync_crc32(&CRC_0, (uint32_t *)&crc_datas[ind], 1, &crc2);
		}
		crc2 ^= 0xFFFFFFFF;

		/* The calculate result should be same. */
		while (crc != crc2);


		char str[12];
		sprintf(str, "CRC:%x\n", crc);

		//USART
		io_write(io, str, 12);
	
	
	
	
	
	
	
	
	
	
	// FLASH EFFECT
			
	grid_led_set_min(3, 1, 0x00, 0x00, 0x30);
	grid_led_set_mid(3, 1, 0x30, 0x00, 0x00);
	grid_led_set_max(3, 1, 0x00, 0x00, 0x00);
			
	grid_led_set_frequency(3, 1, 4);
			



	// UI RX EVENT fref=5, alert=50;
	struct TEL_event_counter* console_tx = grid_tel_event_register(5, 50);
	while(console_tx == NULL){/*TRAP*/}	


	uint32_t faketimer = 0;


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
