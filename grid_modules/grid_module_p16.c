#ifdef GRID_MODULE_P16

#include "../../grid_lib/grid_sys.h"

uint8_t grid_module_mapmode_state  = 1;
uint8_t grid_module_mapmode_change = 0;


uint8_t grid_sync_mode_register[2] = {0, 0};


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




//====================== USART GRID INIT ===================================//

void grid_sys_port_reset_dma(GRID_PORT_t* por){
	
	hri_dmac_clear_CHCTRLA_ENABLE_bit(DMAC, por->dma_channel);
	_dma_enable_transaction(por->dma_channel, false);
}

void grid_sys_uart_init(){
		
		
	// RX PULLUP
 	gpio_set_pin_pull_mode(PC28, GPIO_PULL_UP);
 	gpio_set_pin_pull_mode(PC16, GPIO_PULL_UP);
 	gpio_set_pin_pull_mode(PC12, GPIO_PULL_UP);
 	gpio_set_pin_pull_mode(PB09, GPIO_PULL_UP);
		


 	usart_async_register_callback(&USART_NORTH, USART_ASYNC_TXC_CB, tx_cb_USART_GRID_N);
 	usart_async_register_callback(&USART_EAST,  USART_ASYNC_TXC_CB, tx_cb_USART_GRID_E);
 	usart_async_register_callback(&USART_SOUTH, USART_ASYNC_TXC_CB, tx_cb_USART_GRID_S);
 	usart_async_register_callback(&USART_WEST,  USART_ASYNC_TXC_CB, tx_cb_USART_GRID_W);
// 
//  	usart_async_register_callback(&USART_NORTH, USART_ASYNC_RXC_CB, rx_cb_USART_GRID_N);
//  	usart_async_register_callback(&USART_EAST,  USART_ASYNC_RXC_CB, rx_cb_USART_GRID_E);
//  	usart_async_register_callback(&USART_SOUTH, USART_ASYNC_RXC_CB, rx_cb_USART_GRID_S);
//  	usart_async_register_callback(&USART_WEST,  USART_ASYNC_RXC_CB, rx_cb_USART_GRID_W);
	
	usart_async_get_io_descriptor(&USART_NORTH, &grid_sys_north_io);
	usart_async_get_io_descriptor(&USART_EAST,  &grid_sys_east_io);
	usart_async_get_io_descriptor(&USART_SOUTH, &grid_sys_south_io);
	usart_async_get_io_descriptor(&USART_WEST,  &grid_sys_west_io);
		
	usart_async_enable(&USART_NORTH);
	usart_async_enable(&USART_EAST);
	usart_async_enable(&USART_SOUTH);
	usart_async_enable(&USART_WEST);



}


void dma_transfer_complete_n_cb(struct _dma_resource *resource){
	
	dma_transfer_complete(&GRID_PORT_N);
}
void dma_transfer_complete_e_cb(struct _dma_resource *resource){
	
	dma_transfer_complete(&GRID_PORT_E);
}
void dma_transfer_complete_s_cb(struct _dma_resource *resource){
	
	dma_transfer_complete(&GRID_PORT_S);
}
void dma_transfer_complete_w_cb(struct _dma_resource *resource){
	
	dma_transfer_complete(&GRID_PORT_W);
}

void dma_transfer_complete(GRID_PORT_t* por){

	grid_sys_port_reset_dma(por);

}




//====================== DMA CONFIGURATION FOR GRID USART RX C ===================================//

#define DMA_NORTH_RX_CHANNEL	0
#define DMA_EAST_RX_CHANNEL		1
#define DMA_SOUTH_RX_CHANNEL	2
#define DMA_WEST_RX_CHANNEL		3

//====================== USART -> DMA -> EVENT -> TIMER -> MAGIC ===================================//
	
	

static uint8_t string[20];
static uint8_t string2[20];
	

void grid_rx_dma_init_one(GRID_PORT_t* por, uint32_t buffer_length, void* transfer_done_cb() ){
	
	
	uint8_t dma_rx_channel = por->dma_channel;	
	
	_dma_set_source_address(dma_rx_channel, (uint32_t) & (((Sercom *)((*por->usart).device.hw))->USART.DATA.reg));
	_dma_set_destination_address(dma_rx_channel, (uint32_t *)por->rx_double_buffer);
	_dma_set_data_amount(dma_rx_channel, (uint32_t)buffer_length);
	
	struct _dma_resource *resource_rx;
	_dma_get_channel_resource(&resource_rx, dma_rx_channel);
	
	resource_rx->dma_cb.transfer_done = transfer_done_cb;	
	_dma_set_irq_state(dma_rx_channel, DMA_TRANSFER_COMPLETE_CB, true);
	
	//resource_rx->dma_cb.error         = function_cb;	
	_dma_enable_transaction(dma_rx_channel, false);
	
}

void grid_rx_dma_init(){
			
	grid_rx_dma_init_one(&GRID_PORT_N, GRID_DOUBLE_BUFFER_RX_SIZE, dma_transfer_complete_n_cb);
	grid_rx_dma_init_one(&GRID_PORT_E, GRID_DOUBLE_BUFFER_RX_SIZE, dma_transfer_complete_e_cb);
	grid_rx_dma_init_one(&GRID_PORT_S, GRID_DOUBLE_BUFFER_RX_SIZE, dma_transfer_complete_s_cb);
	grid_rx_dma_init_one(&GRID_PORT_W, GRID_DOUBLE_BUFFER_RX_SIZE, dma_transfer_complete_w_cb);
		
}



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
	
	/* Update the multiplexer */
		
	grid_module_mux++;
	grid_module_mux%=8;
			
	gpio_set_pin_level(MUX_A, grid_module_mux/1%2);
	gpio_set_pin_level(MUX_B, grid_module_mux/2%2);
	gpio_set_pin_level(MUX_C, grid_module_mux/4%2);
		
	
		
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
		
		
	
		
	/* Start conversion new conversion*/
	ADC_0_conversion_ready = 0;	
	ADC_1_conversion_ready = 0;
		
	adc_async_start_conversion(&ADC_0);			
	adc_async_start_conversion(&ADC_1);
		
}


void grid_port_process_ui(GRID_PORT_t* por){
	
	uint8_t message[256];			
	uint32_t length=0;
			
			
	uint8_t len = 0;
	uint8_t id = grid_sys_state.next_broadcast_message_id;
	uint8_t dx = GRID_SYS_DEFAULT_POSITION;
	uint8_t dy = GRID_SYS_DEFAULT_POSITION;	
	uint8_t age = 0;
			
	uint8_t packetvalid = 0;
			
	sprintf(&message[length],
	"%c%c%02x%02x%02x%02x%02x%c",
	GRID_MSG_START_OF_HEADING,
	GRID_MSG_BROADCAST,
	len, id, dx, dy, age,
	GRID_MSG_END_OF_BLOCK
	);
			
	length += strlen(&message[length]);
	
	if (grid_module_mapmode_state != gpio_get_pin_level(MAP_MODE)){
	
		if (grid_module_mapmode_state == 1){
			
// 			static struct hiddf_kb_key_descriptors key_array[]      = {
// 				{HID_CAPS_LOCK, false, HID_KB_KEY_DOWN},
// 				{HID_A, false, HID_KB_KEY_DOWN},
// 			};	
// 			hiddf_keyboard_keys_state_change(key_array, 2);
			
			
			grid_module_mapmode_state = 0;

			sprintf(&message[length], "%c%02x%02x%02x%02x%c",			
				GRID_MSG_START_OF_TEXT,
				GRID_MSG_PROTOCOL_KEYBOARD,
				GRID_MSG_PROTOCOL_KEYBOARD_COMMAND_KEYDOWN,
				GRID_MSG_PROTOCOL_KEYBOARD_PARAMETER_NOT_MODIFIER,
				HID_CAPS_LOCK,
				GRID_MSG_END_OF_TEXT
			);
			length += strlen(&message[length]);
			packetvalid++;
						
		}	
		else{
				
// 			static struct hiddf_kb_key_descriptors key_array[]      = {
// 				{HID_CAPS_LOCK, false, HID_KB_KEY_UP},
// 				{HID_A, false, HID_KB_KEY_UP},
// 			};
// 			hiddf_keyboard_keys_state_change(key_array, 2);
			
			grid_module_mapmode_state = 1;		
			
			sprintf(&message[length], "%c%02x%02x%02x%02x%c",			
				GRID_MSG_START_OF_TEXT,
				GRID_MSG_PROTOCOL_KEYBOARD,
				GRID_MSG_PROTOCOL_KEYBOARD_COMMAND_KEYUP,
				GRID_MSG_PROTOCOL_KEYBOARD_PARAMETER_NOT_MODIFIER,
				HID_CAPS_LOCK,
				GRID_MSG_END_OF_TEXT
			);
			length += strlen(&message[length]);
			packetvalid++;
		
		}
					
	}
		
		
	// PORENTIOMETER/BUTTON READINGS FOR THE UI	
	for (uint8_t i = 0; i<16; i++)
	{
				
				
		if (grid_ain_get_changed(i)){
					
			packetvalid++;
					
			uint16_t average = grid_ain_get_average(i);
					
					
			sprintf(&message[length], "%c%02x%02x%02x%02x%02x%c",
					
			GRID_MSG_START_OF_TEXT,
			GRID_MSG_PROTOCOL_MIDI,
			0, // (cable<<4) + channel
			GRID_MSG_COMMAND_MIDI_CONTROLCHANGE,
			i,
			average/64,
			GRID_MSG_END_OF_TEXT
			);
					

			length += strlen(&message[length]);
					
			// UPDATE LEDS (SHOULD USE UI_TX but whatever)
					
			if (grid_sys_get_hwcfg()==64 && i>11){
				grid_led_set_phase(i-4, 0, average*2/128); // 0...255
			}
			else{
				grid_led_set_phase(i, 0, average*2/128); // 0...255
			}
					
		}
				
	}
			
	if (packetvalid){
			
			
				
		grid_sys_state.next_broadcast_message_id++;
				
		// Close the packet
		sprintf(&message[length], "%c", GRID_MSG_END_OF_TRANSMISSION); // CALCULATE AND ADD CRC HERE
		length += strlen(&message[length]);
				
		// Calculate packet length and insert it into the header
		char length_string[8];
		sprintf(length_string, "%02x", length);
				
		message[2] = length_string[0];
		message[3] = length_string[1];
				
				
		// Add placeholder checksum and linebreak
		sprintf(&message[length], "00\n");
		length += strlen(&message[length]);
			
		grid_msg_set_checksum(message, length, grid_msg_get_checksum(message, length));
		
		// Put the packet into the UI_RX buffer
		if (grid_buffer_write_init(&GRID_PORT_U.rx_buffer, length)){
					
			for(uint16_t i = 0; i<length; i++){
						
				grid_buffer_write_character(&GRID_PORT_U.rx_buffer, message[i]);
			}
					
			grid_buffer_write_acknowledge(&GRID_PORT_U.rx_buffer);
		}
				
				
	}
			
	
}





// DMA SPI CALLBACK
static void tx_complete_cb_GRID_LED(struct _dma_resource *resource)
{
	dma_spi_done = 1;
}





	
/* ============================== GRID_MODULE_INIT() ================================ */
void grid_module_init(void){
		
					

	grid_port_init_all();	
		
		
						
	// Allocate memory for 4 analog input with the filter depth of 3 samples, 14 bit format, 10bit result resolution
	grid_ain_init(grid_module_ain_buffer_size, 5, 14, 8);		
	grid_led_init(grid_module_led_buffer_size);

	spi_m_dma_get_io_descriptor(&GRID_LED, &io2);
	spi_m_dma_register_callback(&GRID_LED, SPI_M_DMA_CB_TX_DONE, tx_complete_cb_GRID_LED);
		


//	grid_rx_timout_init();
	
		
	grid_sys_uart_init();
	
	grid_rx_dma_init();	
	


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
	
	grid_sys_state.error_style = 0;
	grid_sys_state.error_state = 500;
	grid_sys_state.error_code = 7;
		
		
}


#endif