#include "grid_module_en16.h"







static uint8_t UI_SPI_TX_BUFFER[14] = {0};
static uint8_t UI_SPI_RX_BUFFER[14] = {0};

static volatile uint8_t UI_SPI_RX_BUFFER_LAST[16] = {0};

static uint64_t encoder_last_real_time[16] = {0};
static uint64_t button_last_real_time[16] = {0};

static uint8_t phase_change_lock_array[16] = {0};

static void hardware_start_transfer(void){
	
	gpio_set_pin_level(PIN_UI_SPI_CS0, true);
	spi_m_async_enable(&UI_SPI);
	spi_m_async_transfer(&UI_SPI, UI_SPI_TX_BUFFER, UI_SPI_RX_BUFFER, 8);

}

static void spi_transfer_complete_cb(void){

	/* Transfer completed */

	//printf("%d\r\n", _irq_get_current());
	
	// Set the shift registers to continuously load data until new transaction is issued
	gpio_set_pin_level(PIN_UI_SPI_CS0, false);

	uint8_t encoder_position_lookup[16] = {14, 15, 10, 11, 6, 7, 2, 3, 12, 13, 8, 9, 4, 5, 0, 1} ;


	// Buffer is only 8 bytes but we check all 16 encoders separately
	for (uint8_t j=0; j<16; j++){

		uint8_t new_value = (UI_SPI_RX_BUFFER[j/2]>>(4*(j%2)))&0x0F;
		uint8_t old_value = UI_SPI_RX_BUFFER_LAST[j];

		UI_SPI_RX_BUFFER_LAST[j] = new_value;

		
		uint8_t i = encoder_position_lookup[j];

		grid_ui_encoder_store_input(i, &encoder_last_real_time[i], &button_last_real_time[i], old_value, new_value, &phase_change_lock_array[i]);
			
	}
		

	hardware_start_transfer();
}


static void hardware_init(void){
	
	gpio_set_pin_level(PIN_UI_SPI_CS0, false);
	gpio_set_pin_direction(PIN_UI_SPI_CS0, GPIO_DIRECTION_OUT);


	spi_m_async_set_mode(&UI_SPI, SPI_MODE_3);
	spi_m_async_set_baudrate(&UI_SPI, 1000000); // was 400000 check clock div setting
	

	spi_m_async_register_callback(&UI_SPI, SPI_M_ASYNC_CB_XFER, spi_transfer_complete_cb);

}

void grid_module_en16_init(){
	
	grid_module_en16_ui_init(NULL, &grid_led_state, &grid_ui_state);


	hardware_init();
	
	
	hardware_start_transfer();
	
}
