#ifdef GRID_MODULE_P16

	#include "../../grid_lib/grid_sys.h"

	const uint8_t grid_module_mux_lookup[16] = {0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15};

		
		
	uint8_t		  grid_module_mux = 0;

	const uint8_t grid_module_led_buffer_size = 16; // number of ws2812 leds
	
	const uint8_t grid_module_ain_buffer_size = 16; // number of analog inputs
	const uint8_t grid_module_din_buffer_size = 0; // number of digital inputs
	
	
	
	
	#define GRID_ADC_CFG_REVERSED	0
	#define GRID_ADC_CFG_BINARY		1

	
	uint8_t grid_adc_cfg[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};	
	
	void grid_adc_set_config(uint8_t register_offset, uint8_t bit_offest, uint8_t value){
		
		if (value){
			grid_adc_cfg[register_offset] |= (1<<bit_offest);
		}
		else{
			grid_adc_cfg[register_offset] &= ~(1<<bit_offest);
		}
		
	}
	
	void grid_adc_set_config_default(uint8_t register_offset){
		
		grid_adc_cfg[register_offset] = 0;
	}	
	
	uint8_t grid_adc_get_config(uint8_t register_offset, uint8_t bit_offest){
		
		return (grid_adc_cfg[register_offset] & (1<<bit_offest));
		
	}	
		
		
		
	struct io_descriptor *io;
	
	struct io_descriptor *io2;
	
	// Define all of the peripheral interrupt callbacks
	
	volatile static uint32_t dma_spi_done = 0;

	volatile static uint32_t transfer_ready = 1;


	volatile static uint8_t ADC_0_conversion_ready = 0;
	volatile static uint8_t ADC_1_conversion_ready = 0;

	static void convert_cb_ADC_0(const struct adc_async_descriptor *const descr, const uint8_t channel)
	{
		ADC_0_conversion_ready = 1;
	}

	static void convert_cb_ADC_1(const struct adc_async_descriptor *const descr, const uint8_t channel)
	{
		ADC_1_conversion_ready = 1;
		
		
		/* Make sure both results are ready */
		
		while(ADC_0_conversion_ready==0){}
		while(ADC_1_conversion_ready==0){}
		
		/* Read conversion results */
		
		uint16_t adcresult_0 = 0;
		uint16_t adcresult_1 = 0;
		
		uint8_t adc_index_0 = grid_module_mux_lookup[grid_module_mux+8];
		uint8_t adc_index_1 = grid_module_mux_lookup[grid_module_mux+0];
		
		adc_async_read_channel(&ADC_0, 0, &adcresult_0, 2);
		adc_async_read_channel(&ADC_1, 0, &adcresult_1, 2);
		
					
		if (grid_adc_get_config(adc_index_0, GRID_ADC_CFG_REVERSED)){
			adcresult_0 = 65535 - adcresult_0;
		}		
		
		if (grid_adc_get_config(adc_index_1, GRID_ADC_CFG_REVERSED)){
			adcresult_1 = 65535 - adcresult_1;
		}	
		
		if (grid_adc_get_config(adc_index_0, GRID_ADC_CFG_BINARY)){
			adcresult_0 = (adcresult_0>10000)*65535;
		}
		
		if (grid_adc_get_config(adc_index_1, GRID_ADC_CFG_BINARY)){
			adcresult_1 = (adcresult_1>10000)*65535;
		}
		
		
		
		grid_ain_add_sample(adc_index_0, adcresult_0);
		grid_ain_add_sample(adc_index_1, adcresult_1);
		
		
		/* Update the multiplexer */
		
		grid_module_mux++;
		grid_module_mux%=8;
		
		
		
		gpio_set_pin_level(MUX_A, grid_module_mux/1%2);
		gpio_set_pin_level(MUX_B, grid_module_mux/2%2);
		gpio_set_pin_level(MUX_C, grid_module_mux/4%2);
		
		/* Start conversion new conversion*/
		ADC_0_conversion_ready = 0;	
		ADC_1_conversion_ready = 0;
		
		adc_async_start_conversion(&ADC_0);			
		adc_async_start_conversion(&ADC_1);
		
	}


	// DMA SPI CALLBACK
	static void tx_complete_cb_GRID_LED(struct _dma_resource *resource)
	{
		dma_spi_done = 1;
	}


	/* ============================== GRID_MODULE_INIT() ================================ */

	void grid_module_init(void){
		
						
		// Allocate memory for 4 analog input with the filter depth of 3 samples, 14 bit format, 10bit result resolution
		grid_ain_init(grid_module_ain_buffer_size, 5, 14, 8);		
		grid_led_init(grid_module_led_buffer_size);

		spi_m_dma_get_io_descriptor(&GRID_LED, &io2);
		spi_m_dma_register_callback(&GRID_LED, SPI_M_DMA_CB_TX_DONE, tx_complete_cb_GRID_LED);
	
		grid_sys_uart_init();
		
		grid_buffer_init_all();


		//enable pwr!
		gpio_set_pin_level(UI_PWR_EN, true);

		// ADC SETUP	
		
		if (grid_sys_get_hwcfg() == GRID_MODULE_P16_RevB){
						
		}
		
		if (grid_sys_get_hwcfg() == GRID_MODULE_B16_RevB){
			
			grid_adc_set_config(0, GRID_ADC_CFG_REVERSED, 1);
			grid_adc_set_config(1, GRID_ADC_CFG_REVERSED, 1);
			grid_adc_set_config(2, GRID_ADC_CFG_REVERSED, 1);
			grid_adc_set_config(3, GRID_ADC_CFG_REVERSED, 1);
			grid_adc_set_config(4, GRID_ADC_CFG_REVERSED, 1);
			grid_adc_set_config(5, GRID_ADC_CFG_REVERSED, 1);
			grid_adc_set_config(6, GRID_ADC_CFG_REVERSED, 1);
			grid_adc_set_config(7, GRID_ADC_CFG_REVERSED, 1);
			grid_adc_set_config(8, GRID_ADC_CFG_REVERSED, 1);
			grid_adc_set_config(9, GRID_ADC_CFG_REVERSED, 1);
			grid_adc_set_config(10, GRID_ADC_CFG_REVERSED, 1);
			grid_adc_set_config(11, GRID_ADC_CFG_REVERSED, 1);
			grid_adc_set_config(12, GRID_ADC_CFG_REVERSED, 1);
			grid_adc_set_config(13, GRID_ADC_CFG_REVERSED, 1);
			grid_adc_set_config(14, GRID_ADC_CFG_REVERSED, 1);
			grid_adc_set_config(15, GRID_ADC_CFG_REVERSED, 1);
			
			grid_adc_set_config(0, GRID_ADC_CFG_BINARY, 1);
			grid_adc_set_config(1, GRID_ADC_CFG_BINARY, 1);
			grid_adc_set_config(2, GRID_ADC_CFG_BINARY, 1);
			grid_adc_set_config(3, GRID_ADC_CFG_BINARY, 1);
			grid_adc_set_config(4, GRID_ADC_CFG_BINARY, 1);
			grid_adc_set_config(5, GRID_ADC_CFG_BINARY, 1);
			grid_adc_set_config(6, GRID_ADC_CFG_BINARY, 1);
			grid_adc_set_config(7, GRID_ADC_CFG_BINARY, 1);		
			grid_adc_set_config(8, GRID_ADC_CFG_BINARY, 1);
			grid_adc_set_config(9, GRID_ADC_CFG_BINARY, 1);
			grid_adc_set_config(10, GRID_ADC_CFG_BINARY, 1);
			grid_adc_set_config(11, GRID_ADC_CFG_BINARY, 1);
			grid_adc_set_config(12, GRID_ADC_CFG_BINARY, 1);
			grid_adc_set_config(13, GRID_ADC_CFG_BINARY, 1);
			grid_adc_set_config(14, GRID_ADC_CFG_BINARY, 1);
			grid_adc_set_config(15, GRID_ADC_CFG_BINARY, 1);
			
		}
		
		if (grid_sys_get_hwcfg() == GRID_MODULE_PBF4_RevA){
			
			grid_adc_set_config(0, GRID_ADC_CFG_REVERSED, 1);
			grid_adc_set_config(1, GRID_ADC_CFG_REVERSED, 1);
			grid_adc_set_config(2, GRID_ADC_CFG_REVERSED, 1);
			grid_adc_set_config(3, GRID_ADC_CFG_REVERSED, 1);
					
			grid_adc_set_config(12, GRID_ADC_CFG_REVERSED, 1);
			grid_adc_set_config(13, GRID_ADC_CFG_REVERSED, 1);
			grid_adc_set_config(14, GRID_ADC_CFG_REVERSED, 1);
			grid_adc_set_config(15, GRID_ADC_CFG_REVERSED, 1);
			
			grid_adc_set_config(12, GRID_ADC_CFG_BINARY, 1);
			grid_adc_set_config(13, GRID_ADC_CFG_BINARY, 1);
			grid_adc_set_config(14, GRID_ADC_CFG_BINARY, 1);
			grid_adc_set_config(15, GRID_ADC_CFG_BINARY, 1);
					
		}
				
		
	
		adc_async_register_callback(&ADC_0, 0, ADC_ASYNC_CONVERT_CB, convert_cb_ADC_0);
		adc_async_enable_channel(&ADC_0, 0);
		adc_async_start_conversion(&ADC_0);
				
		adc_async_register_callback(&ADC_1, 0, ADC_ASYNC_CONVERT_CB, convert_cb_ADC_1);
		adc_async_enable_channel(&ADC_1, 0);
		adc_async_start_conversion(&ADC_1);
		
	

	
		// ===================== USART SETUP ========================= //
	
		//usart_async_register_callback(&GRID_AUX, USART_ASYNC_TXC_CB, tx_cb_GRID_AUX);
		/*usart_async_register_callback(&GRID_AUX, USART_ASYNC_RXC_CB, rx_cb);
		usart_async_register_callback(&GRID_AUX, USART_ASYNC_ERROR_CB, err_cb);*/
	
		usart_async_get_io_descriptor(&GRID_AUX, &io);
		usart_async_enable(&GRID_AUX);


		// GRID_LED Library NEW NEW NEW NEW
	
		
		
	}


#endif