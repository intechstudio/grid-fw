#include <atmel_start.h>
#include "../../grid_lib/grid_led.c" // WS2812 LED
#include "../../grid_lib/grid_ain.c" // Analog input filtering
#include "../../grid_lib/grid_tel.c" // Grid Telemetry


volatile uint8_t adc_buffer[2];


volatile static uint32_t conversion_ready = 0;
volatile static uint32_t dma_spi_done = 0;

volatile static uint32_t transfer_ready = 1;




static void convert_cb_ADC_1(const struct adc_async_descriptor *const descr, const uint8_t channel)
{
	
	conversion_ready = 1;

}


// DMA SPI CALLBACK
static void tx_complete_cb_GRID_LED(struct _dma_resource *resource)
{
	dma_spi_done = 1;
}


int main(void)
{
	/* Initializes MCU, drivers and middleware */
	
	atmel_start_init();
	
	
	struct io_descriptor *io2;
	spi_m_dma_get_io_descriptor(&GRID_LED, &io2);
	spi_m_dma_register_callback(&GRID_LED, SPI_M_DMA_CB_TX_DONE, tx_complete_cb_GRID_LED);
	

	//enable pwr!
	gpio_set_pin_level(UI_PWR_EN, true);
	adc_buffer[0] = 0;
	adc_buffer[1] = 0;



	// ADC SETUP	
	
			
	adc_async_register_callback(&ADC_1, 0, ADC_ASYNC_CONVERT_CB, convert_cb_ADC_1);
	adc_async_enable_channel(&ADC_1, 0);

	adc_async_start_conversion(&ADC_1);
	
	
	// ===================== USART SETUP ========================= //
	
	struct io_descriptor *io;
	//usart_async_register_callback(&GRID_AUX, USART_ASYNC_TXC_CB, tx_cb_GRID_AUX);
	/*usart_async_register_callback(&GRID_AUX, USART_ASYNC_RXC_CB, rx_cb);
	usart_async_register_callback(&GRID_AUX, USART_ASYNC_ERROR_CB, err_cb);*/
	
	usart_async_get_io_descriptor(&GRID_AUX, &io);
	usart_async_enable(&GRID_AUX);


	// GRID_LED Library NEW NEW NEW NEW
	
	

	// Allocate memory for 4 leds and initialize the structure!
	grid_led_init(4);
	
	// ================ LED DEFAULT CONFIG ================== //
	grid_led_set_min(0, 0, 0x00, 0x00, 0x00);
	grid_led_set_mid(0, 0, 0x00, 0x00, 0x60);
	grid_led_set_max(0, 0, 0x00, 0x00, 0xE0);
			
			
	grid_led_set_frequency(0, 0, 0);
			
	grid_led_set_min(1, 1, 0x00, 0x00, 0x30);
	grid_led_set_mid(1, 1, 0x30, 0x00, 0x00);
	grid_led_set_max(1, 1, 0x00, 0x00, 0x00);
			
	grid_led_set_frequency(1, 1, 4);
			
			
	
	// Allocate memory for 1 analog input with the filter depth of 8 samples, 14 bit format, 10bit result resolution
	grid_ain_init(1, 3, 14, 10);


	// UI RX EVENT fref=5, alert=50;
	struct TEL_event_counter* console_tx = grid_tel_event_register(5, 50);
	while(console_tx == NULL){/*TRAP*/}	
	
	char str[26];
	sprintf(str, "FreqRef: %5d\n", grid_tel_event_head->frequency);

	//USART
	io_write(io, str, 26);

	uint32_t faketimer = 0;

	while (1) {
		
		if (faketimer == 10){
			grid_tel_frequency_tick();
			faketimer = 0;
		}
		faketimer++;
		
				
		gpio_toggle_pin_level(LED0);
		
		
		uint8_t adc_result_buffer[2];
		conversion_ready = 0;
		adc_async_start_conversion(&ADC_1);
		while(conversion_ready==0){}
		adc_async_read_channel(&ADC_1, 0, adc_result_buffer, 2);
		
		uint16_t adcresult = 256*adc_result_buffer[1] + adc_result_buffer[0];	
			
		grid_ain_add_sample(0,adcresult);
		
		
		if (grid_ain_get_changed(0) && !grid_tel_event_alert_status(console_tx)){
			
			if (grid_ain_get_changed(0)){grid_tel_event_handler(console_tx);}
				
				
			uint16_t average = grid_ain_get_average(0);
			
			char str[26];
			sprintf(str, "ADC: %5d %5d %5d\n", average, average/128, console_tx->frequency);

			//USART
			io_write(io, str, 26);	
			
			grid_led_set_phase(0, 0, average/8/4/4);
			
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
