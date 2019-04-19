#include <atmel_start.h>
#include "../../grid_lib/grid_led.c"


volatile uint8_t adc_buffer[2];


volatile static uint32_t conversion_ready = 0;
volatile static uint32_t dma_spi_done = 0;

volatile static uint32_t transfer_ready = 1;

static uint8_t example_GRID_AUX[2] = "He";



static void tx_cb_GRID_AUX(const struct usart_async_descriptor *const io_descr)
{
	

}


static void complete_cb_GRID_LED(const struct spi_m_async_descriptor *const io_descr)
{
	/* Transfer completed */
	

	
	//gpio_set_pin_level(PB21, 0);
	
}




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
	uint8_t adc_result_buffer[2];
			
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
	
		
	// ANALOG MAGIC
			
	#define BSIZE 32
	#define ANA_SAMPLE_RESOLUTION 16
	#define ANA_RESULT_RESOLUTION 14
	#define ANA_DIV (1<<(ANA_SAMPLE_RESOLUTION-ANA_RESULT_RESOLUTION))
		
	uint32_t bufferPtr = 0;
	uint32_t bufferMinPtr = 0;
	uint32_t bufferMaxPtr = 0;
	uint16_t buffer[BSIZE];
	
	
	
	//ANIMATION
	uint8_t intensity = 0; 
	
	while (1) {
		


		
		

		
		
		
				
		gpio_toggle_pin_level(LED0);
		
		uint16_t ana_min=-1;
		uint16_t ana_max=0;
		uint32_t ana_sum=0;
		uint32_t ana_avg=0;
			
		for(uint32_t i = 0; i<BSIZE; i++){
			
			
			if (buffer[i]<ana_min){
				
				ana_min = buffer[i];
				bufferMinPtr = i;
			}
			
			if (buffer[i]>ana_max){
				
				ana_max = buffer[i];
				bufferMaxPtr = i;
			}
			
			
			ana_sum += buffer[i];
		}
		ana_avg = ana_sum/BSIZE;
			
			
			
		// HW Averaging
		uint32_t quicksum = 0;
		uint32_t adcresult	 =  0;
		for (uint16_t i=0; i<64; i++){
			
			conversion_ready = 0;
			adc_async_start_conversion(&ADC_1);
			while(conversion_ready==0){}	
			
			adc_async_read_channel(&ADC_1, 0, adc_result_buffer, 2);
			
			adcresult = 256*adc_result_buffer[1] + adc_result_buffer[0];
			
			// OFSET + GAIN COMPENSATION
			#define ADC_OFFSET 86
			#define ADC_GAIN 1.0015283
			
		
			
			//adcresult = (   ((adcresult>ADC_OFFSET)?(adcresult-ADC_OFFSET):0)   )*ADC_GAIN;
			//adcresult = (adcresult>65535)?65535:adcresult;
			
			quicksum += adcresult;			
			
			
			
		}

		quicksum = quicksum/64;
		uint16_t someInt = quicksum;
		
		
					
		if (someInt>ana_avg){
			
			buffer[bufferMinPtr] = someInt;
		}
		else{		
			buffer[bufferMaxPtr] = someInt;
		}
		
			
		gpio_toggle_pin_level(LED0);
		

			
		
		uint32_t ana_best=0;
		uint32_t ana_valid=0;
		
		
		for(uint32_t i = 0; i<BSIZE; i++){
					
			if ((ana_min+(ana_max-ana_min)/5-1) <buffer[i] && buffer[i] <(ana_max-(ana_max-ana_min)/5)+1){
				
				ana_best+=buffer[i];
				ana_valid++;
			}

		}	
		
		if (ana_valid == 0){
			ana_best=ana_avg;
			ana_valid=1;
		}
		
		
		
		// ================ WS2812B VIA DMA SPI ================== //
					

	
		delay_ms(1);
		
		
		grid_led_set_min(0, 0, 0x00, 0x00, 0x00);	
		grid_led_set_mid(0, 0, 0x00, 0x00, 0x60);	
		grid_led_set_max(0, 0, 0x00, 0x00, 0xE0);
		
		grid_led_set_phase(0, 0, ana_avg/16/4/4);
		grid_led_set_frequency(0, 0, 0);
		
		grid_led_set_min(1, 1, 0x00, 0x00, 0x30);	
		grid_led_set_mid(1, 1, 0x30, 0x00, 0x00);	
		grid_led_set_max(1, 1, 0x00, 0x00, 0x00);
		
		grid_led_set_frequency(1, 1, 4);
				
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
		
		
		
		// PREPARE DEBUG CONSOLE DATA 
		char str[26];
		sprintf(str, "ADC: %5d %5d %5d\n", (ana_best/ana_valid/ANA_DIV), (ana_max-ana_min), adcresult);

		//USART
		io_write(io, str, 26);
	

		
		
		
		
		
	
	}
}
