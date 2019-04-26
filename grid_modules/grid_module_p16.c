#ifdef GRID_MODULE_P16


	const uint8_t grid_module_mux_lookup[16] = {0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15};
	uint8_t		  grid_module_mux = 0;

	const uint8_t grid_module_led_buffer_size = 16; // number of ws2812 leds
	
	const uint8_t grid_module_ain_buffer_size = 16; // number of analog inputs
	const uint8_t grid_module_din_buffer_size = 0; // number of digital inputs
	
	
		
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
		
		adc_async_read_channel(&ADC_0, 0, &adcresult_0, 2);
		grid_ain_add_sample(grid_module_mux_lookup[grid_module_mux+8], adcresult_0);
		
		
		uint16_t adcresult_1 = 0;
		adc_async_read_channel(&ADC_1, 0, &adcresult_1, 2);
		grid_ain_add_sample(grid_module_mux_lookup[grid_module_mux+0], adcresult_1);
		
		
		/* Update the multiplexer */
		if (grid_module_mux == 0){
			grid_module_mux = 1;
			}else{
			grid_module_mux = 0;
		}
		
		
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
		grid_ain_init(grid_module_ain_buffer_size, 3, 14, 10);		
		grid_led_init(grid_module_led_buffer_size);

		spi_m_dma_get_io_descriptor(&GRID_LED, &io2);
		spi_m_dma_register_callback(&GRID_LED, SPI_M_DMA_CB_TX_DONE, tx_complete_cb_GRID_LED);
	

		//enable pwr!
		gpio_set_pin_level(UI_PWR_EN, true);

		// ADC SETUP	
	
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