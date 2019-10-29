#include "grid_module.h"



uint8_t grid_sync_mode_register[2] = {0, 0};

//====================== GRID ENCODER K+F ===================================//


struct grid_ui_encoder{
	
	uint8_t controller_number;
	
	uint8_t button_value;
	uint8_t button_changed;
	
	uint8_t rotation_value;
	uint8_t rotation_changed;
	
	uint8_t rotation_direction;
	
	uint8_t phase_a_previous;
	uint8_t phase_b_previous;
	
};


struct grid_ui_encoder grid_ui_encoder_array[16];

static uint8_t UI_SPI_TX_BUFFER[14] = "aaaaaaaaaaaaaa";
static uint8_t UI_SPI_RX_BUFFER[14];
static uint8_t UI_SPI_TRANSFER_LENGTH = 10;

static uint8_t UI_SPI_DEBUG = 8;

volatile uint8_t UI_SPI_DONE = 0;


volatile uint8_t UI_SPI_RX_BUFFER_LAST[16];

static uint8_t UI_ENCODER_BUTTON_STATE[16];
static uint8_t UI_ENCODER_BUTTON_STATE_CHANGED[16];

static uint8_t UI_ENCODER_ROTATION_STATE[16];
static uint8_t UI_ENCODER_ROTATION_STATE_CHANGED[16];


static uint8_t UI_ENCODER_LOOKUP[16] = {14, 15, 10, 11, 6, 7, 2, 3, 12, 13, 8, 9, 4, 5, 0, 1} ;





//====================== GRID SYNC ===================================//
enum grid_sync_selector { GRID_SYNC_UNDEFINED, GRID_SYNC_1, GRID_SYNC_2};
enum grid_sync_mode { GRID_SYNC_INITIAL, GRID_SYNC_MASTER, GRID_SYNC_SLAVE};
	
	
void grid_sync_set_mode(enum grid_sync_selector sync_select, enum grid_sync_mode sync_mode){
	
	grid_sync_mode_register[sync_select - 1]  = sync_mode;
	
	if (sync_select == GRID_SYNC_1){		
		
		if (sync_mode == GRID_SYNC_MASTER){
			
			gpio_set_pin_level(PIN_GRID_SYNC_1, true);
			gpio_set_pin_direction(PIN_GRID_SYNC_1, GPIO_DIRECTION_OUT);
		}
		else if (sync_mode == GRID_SYNC_SLAVE){
			gpio_set_pin_direction(PIN_GRID_SYNC_1, GPIO_DIRECTION_IN);
			gpio_set_pin_level(PIN_GRID_SYNC_1, true);
		}
		
	}
	else if (sync_select == GRID_SYNC_2){	
			
		if (sync_mode == GRID_SYNC_MASTER){
			
			gpio_set_pin_level(PIN_GRID_SYNC_2, true);
			gpio_set_pin_direction(PIN_GRID_SYNC_2, GPIO_DIRECTION_OUT);
		}
		else if (sync_mode == GRID_SYNC_SLAVE){
			gpio_set_pin_direction(PIN_GRID_SYNC_2, GPIO_DIRECTION_IN);
			gpio_set_pin_level(PIN_GRID_SYNC_1, true);
		}
		
	}
	
}

enum grid_sync_mode grid_sync_get_mode(enum grid_sync_selector sync_select){
	
	if (grid_sync_mode_register[sync_select - 1] == GRID_SYNC_MASTER){
		return GRID_SYNC_MASTER;
	}
	else if (grid_sync_mode_register[sync_select - 1] == GRID_SYNC_SLAVE){
		return GRID_SYNC_SLAVE;
	}
	else{
		return GRID_SYNC_INITIAL;	
	}	
}

void grid_sync_set_level(enum grid_sync_selector sync_select, uint8_t sync_level){
	
	if (sync_select == GRID_SYNC_1){
		
		if (grid_sync_get_mode(sync_select) == GRID_SYNC_MASTER){
			
			gpio_set_pin_level(PIN_GRID_SYNC_1, sync_level);
		}
		
	}
	else if (sync_select == GRID_SYNC_2){
		
		if (grid_sync_get_mode(sync_select) == GRID_SYNC_MASTER){
			
			gpio_set_pin_level(PIN_GRID_SYNC_2, sync_level);
		}
		
	}
		
}




static uint8_t string[20];
static uint8_t string2[20];
	





	
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
	

// Define all of the peripheral interrupt callbacks
	


volatile static uint32_t transfer_ready = 1;







void grid_module_init_animation(struct grid_led_model* mod){
	
	
	for (uint8_t i = 0; i<255; i++){
			
		// SEND DATA TO LEDs
			
			
		uint8_t color_r   = i;
		uint8_t color_g   = i;
		uint8_t color_b   = i;
			
			
		for (uint8_t i=0; i<mod->led_number; i++){
			//grid_led_set_color(i, 0, 255, 0);
			grid_led_set_color(&grid_led_state, i, color_r, color_g, color_b);
				
		}
			
			
		grid_led_hardware_start_transfer_blocking(&grid_led_state);
			
		delay_ms(1);
			
	}
	
}
	
static void grid_ui_encoder_hardware_transfer_complete_cb(struct _dma_resource *resource)
{
	/* Transfer completed */


	grid_sync_set_mode(GRID_SYNC_1, GRID_SYNC_MASTER);
	grid_sync_set_level(GRID_SYNC_1, 1);
		
	grid_sync_set_mode(GRID_SYNC_1, GRID_SYNC_MASTER);

	// Set the shift registers to continuously load data until new transaction is issued
	gpio_set_pin_level(PIN_UI_SPI_CS0, false);

		
	// Buffer is only 8 bytes but we check all 16 encoders separately
	for (uint8_t i=0; i<16; i++){

		uint8_t new_value = (UI_SPI_RX_BUFFER[i/2]>>(4*(i%2)))&0x0F;
		uint8_t old_value = UI_SPI_RX_BUFFER_LAST[i];
			
		if (old_value != new_value){

				
			UI_SPI_DEBUG = i;
				
			uint8_t button_value = new_value>>2;
			uint8_t phase_a = (new_value>>1)&1;
			uint8_t phase_b = (new_value)&1;
				
			if (button_value != grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].button_value){
				// BUTTON CHANGE
				grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].button_changed = 1;
				grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].button_value = new_value>>2;
			}
				
				
			if (phase_a != grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].phase_a_previous){
					
				grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].phase_a_previous = phase_a;
					
				if (phase_b == 0){
					grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].rotation_direction = phase_a;
				}
				else{
					grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].rotation_direction = !phase_a;
				}
					
				if (phase_a && phase_b){
						
					grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].rotation_value += grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].rotation_direction*2 -1;
					grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].rotation_changed = 1;
				}
			}
				
			if (phase_b != grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].phase_b_previous){
					
				grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].phase_b_previous = phase_b;
					
				if (phase_a == 0){
					grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].rotation_direction = !phase_b;
				}
				else{
					grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].rotation_direction = phase_b;
				}
					
				if (phase_a && phase_b){

					grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].rotation_value += grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].rotation_direction*2 -1;
					grid_ui_encoder_array[UI_ENCODER_LOOKUP[i]].rotation_changed = 1;
				}
			}
				

				
		}
		else{
				
		}
			
	}
		
	UI_SPI_DONE = 1;
		
	for (uint8_t i=0; i<8; i++){

		UI_SPI_RX_BUFFER[i] = 0;
			
	}
		

	grid_sync_set_level(GRID_SYNC_1, 0);
		
	grid_ui_encoder_hardware_start_transfer();
}

void grid_ui_encoder_hardware_init(void)
{
		
	gpio_set_pin_level(PIN_UI_SPI_CS0, false);
	gpio_set_pin_direction(PIN_UI_SPI_CS0, GPIO_DIRECTION_OUT);
		
		
	spi_m_async_set_mode(&UI_SPI, SPI_MODE_3);
		
	spi_m_async_get_io_descriptor(&UI_SPI, &io);


	spi_m_async_register_callback(&UI_SPI, SPI_M_ASYNC_CB_XFER, grid_ui_encoder_hardware_transfer_complete_cb);

}

void grid_ui_encoder_hardware_start_transfer(void)
{
		

	gpio_set_pin_level(PIN_UI_SPI_CS0, true);
		
	spi_m_async_enable(&UI_SPI);
		
	//io_write(io, UI_SPI_TX_BUFFER, 8);
	spi_m_async_transfer(&UI_SPI, UI_SPI_TX_BUFFER, UI_SPI_RX_BUFFER, 8);
}



	
/* ============================== GRID_MODULE_INIT() ================================ */


void grid_module_init(void){
				
	grid_port_init_all();		
	grid_sys_uart_init();	
	grid_rx_dma_init();	
	
	
	//enable pwr!
	gpio_set_pin_level(UI_PWR_EN, true);


	// ADC SETUP	
		
	if (grid_sys_get_hwcfg() == GRID_MODULE_P16_RevB){
		
		// Allocate memory for 16 analog input with the filter depth of 5 samples, 14 bit format, 7bit result resolution
		grid_ain_init(16, 5, 14, 7);
		
		grid_led_init(&grid_led_state, 16);			
		grid_module_init_animation(&grid_led_state);	
						
// 		grid_ui_potentiometer_hardware_init();
// 		grid_ui_potentiometer_hardware_start_transfer();
		
		//grid_module_b16_revb_init(&grid_ui_state);	
		
	}
		
	if (grid_sys_get_hwcfg() == GRID_MODULE_B16_RevB){	
		
		grid_module_bu16_revb_init(&grid_ui_state);	
		
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
		
		// Allocate memory for 4 analog input with the filter depth of 5 samples, 14 bit format, 10bit result resolution
		grid_ain_init(16, 5, 14, 7);
		
		grid_led_init(&grid_led_state, 16);		
		grid_module_init_animation(&grid_led_state);
			
// 		grid_ui_potentiometer_hardware_init();
// 		grid_ui_potentiometer_hardware_start_transfer();
// 					
	}
	
	if (grid_sys_get_hwcfg() == GRID_MODULE_EN16_RevA){
		
		static struct grid_ui_encoder grid_ui_encoder_array[16];
		
		for (uint8_t i = 0; i<16; i++)
		{
			grid_ui_encoder_array[i].controller_number = i;
		}
		
		grid_led_init(&grid_led_state, 16);
		grid_module_init_animation(&grid_led_state);
	
		grid_ui_encoder_hardware_init();
	
	}	
	

	
	// ===================== USART SETUP ========================= //
	
	//usart_async_register_callback(&GRID_AUX, USART_ASYNC_TXC_CB, tx_cb_GRID_AUX);
	/*usart_async_register_callback(&GRID_AUX, USART_ASYNC_RXC_CB, rx_cb);
	usart_async_register_callback(&GRID_AUX, USART_ASYNC_ERROR_CB, err_cb);*/
	
	usart_async_get_io_descriptor(&GRID_AUX, &io);
	usart_async_enable(&GRID_AUX);
	
		
}
