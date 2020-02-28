/*
 * grid_sys.c
 *
 * Created: 4/12/2019 5:27:13 PM
 * Author : SUKU WC
*/

#include "grid_sys.h"


//====================== grid sys unittest ===================================//

uint32_t grid_sys_unittest(void){


	if(grid_unittest_group_init(&grid_unittest_state, "grid_sys::checksum"))
	{
		if(grid_unittest_case_init(&grid_unittest_state, "Checksum Read/Calculate"))
		{ // Read/Calculate
			
			uint8_t length = 8;
			uint8_t packet[8] = {1, 0, 2, 2, 4, 0, 0, 10};
		
			uint8_t checksum_calc = grid_msg_checksum_calculate(packet, length);
			uint8_t checksum_read = grid_msg_checksum_read(packet, length);
				
			char str[100] = {0};
			sprintf(str, "packet{%d, %d, %d, %d, %d, %d, %d, %d} Read: %d, Calculate: %d", packet[0], packet[1], packet[2], packet[3], packet[4], packet[5], packet[6], packet[7], checksum_read, checksum_calc);	
		
			if (checksum_calc != checksum_read){		
				grid_unittest_case_pass(&grid_unittest_state,str);
			}
			else{
				grid_unittest_case_fail(&grid_unittest_state,str);
			}
				
		}
		if (grid_unittest_case_init(&grid_unittest_state, "Checksum Write/Calculate")) // Write/Calculate	
		{	
		
		
			uint8_t length = 8;
			uint8_t packet[8] = {1, 0, 2, 2, 4, 0, 0, 10};
					
			uint8_t checksum_calc = grid_msg_checksum_calculate(packet, length);
		
			grid_msg_checksum_write(packet, length, checksum_calc);
		
			uint8_t checksum_read = grid_msg_checksum_read(packet, length);
				
			char str[100] = {0};
			sprintf(str, "packet{%d, %d, %d, %d, %d, %d, %d, %d} Read: %d, Calculate: %d", packet[0], packet[1], packet[2], packet[3], packet[4], packet[5], packet[6], packet[7], checksum_read, checksum_calc);	
		
		
			if (checksum_calc == checksum_read){		
				grid_unittest_case_pass(&grid_unittest_state,str);
			}
			else{
				grid_unittest_case_fail(&grid_unittest_state,str);
			}
				
		}	
		
		
	}
	
	grid_unittest_case_init(&grid_unittest_state, "Checksum Overwrite");
	grid_unittest_case_fail(&grid_unittest_state, "Parapaprikas");
		
	grid_unittest_group_done(&grid_unittest_state);
		

	//grid_unittest_case_init(&grid_unittest_state, );

	return 1;
}








//=============================== USART TX COMPLETE ==============================//

static void tx_cb_USART_GRID_N(const struct usart_async_descriptor *const descr)
{
	tx_cb_USART_GRID(&GRID_PORT_N);
}

static void tx_cb_USART_GRID_E(const struct usart_async_descriptor *const descr)
{
	tx_cb_USART_GRID(&GRID_PORT_E);
}

static void tx_cb_USART_GRID_S(const struct usart_async_descriptor *const descr)
{
	tx_cb_USART_GRID(&GRID_PORT_S);
}

static void tx_cb_USART_GRID_W(const struct usart_async_descriptor *const descr)
{
	tx_cb_USART_GRID(&GRID_PORT_W);
}

void tx_cb_USART_GRID(struct grid_port* const por){
	
	
	for(uint32_t i=0; i<por->tx_double_buffer_status; i++){
		por->tx_double_buffer[i] = 0;
	}
	por->tx_double_buffer_status = 0;	
}


static void rx_cb_USART_GRID_N(const struct usart_async_descriptor *const descr)
{
	rx_cb_USART_GRID(&GRID_PORT_N);
}

static void rx_cb_USART_GRID_E(const struct usart_async_descriptor *const descr)
{
	rx_cb_USART_GRID(&GRID_PORT_E);
}

static void rx_cb_USART_GRID_S(const struct usart_async_descriptor *const descr)
{
	rx_cb_USART_GRID(&GRID_PORT_S);
}

static void rx_cb_USART_GRID_W(const struct usart_async_descriptor *const descr)
{
	rx_cb_USART_GRID(&GRID_PORT_W);
}

void rx_cb_USART_GRID(struct grid_port* const por){
	
}

volatile dmatest = 0;

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
void dma_transfer_complete(struct grid_port* por){

	grid_sys_port_reset_dma(por);

}


uint32_t grid_sys_hwfcg = -1;



static void err_cb_USART_GRID_N(const struct usart_async_descriptor *const descr)
{
	err_cb_USART_GRID(&GRID_PORT_N);
}

static void err_cb_USART_GRID_E(const struct usart_async_descriptor *const descr)
{
	err_cb_USART_GRID(&GRID_PORT_E);
}

static void err_cb_USART_GRID_S(const struct usart_async_descriptor *const descr)
{
	err_cb_USART_GRID(&GRID_PORT_S);
}

static void err_cb_USART_GRID_W(const struct usart_async_descriptor *const descr)
{
	err_cb_USART_GRID(&GRID_PORT_W);
}


void err_cb_USART_GRID(struct grid_port* const por){
	por->usart_error_flag = 1;	
	
	usart_async_disable(por->usart);
}



//====================== DMA CONFIGURATION FOR GRID USART RX C ===================================//

#define DMA_NORTH_RX_CHANNEL	0
#define DMA_EAST_RX_CHANNEL		1
#define DMA_SOUTH_RX_CHANNEL	2
#define DMA_WEST_RX_CHANNEL		3

void grid_sys_port_reset_dma(struct grid_port* por){
	
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
			
	// Set parity for grid uart communication
	usart_async_set_parity(&USART_NORTH, USART_PARITY_ODD);
	usart_async_set_parity(&USART_EAST, USART_PARITY_ODD);	
	usart_async_set_parity(&USART_SOUTH, USART_PARITY_ODD);
	usart_async_set_parity(&USART_WEST, USART_PARITY_ODD);
	
	// Set callback function for parity error
	usart_async_register_callback(&USART_NORTH, USART_ASYNC_ERROR_CB, err_cb_USART_GRID_N);
	usart_async_register_callback(&USART_EAST, USART_ASYNC_ERROR_CB, err_cb_USART_GRID_E);
	usart_async_register_callback(&USART_SOUTH, USART_ASYNC_ERROR_CB, err_cb_USART_GRID_S);
	usart_async_register_callback(&USART_WEST, USART_ASYNC_ERROR_CB, err_cb_USART_GRID_W);
	
	
	
	
	
	
	
// 	usart_async_register_callback(&USART_NORTH, USART_ASYNC_RXC_CB, rx_cb_USART_GRID_N);
// 	usart_async_register_callback(&USART_EAST,  USART_ASYNC_RXC_CB, rx_cb_USART_GRID_E);
// 	usart_async_register_callback(&USART_SOUTH, USART_ASYNC_RXC_CB, rx_cb_USART_GRID_S);
// 	usart_async_register_callback(&USART_WEST,  USART_ASYNC_RXC_CB, rx_cb_USART_GRID_W);

	
	usart_async_get_io_descriptor(&USART_NORTH, &grid_sys_north_io);
	usart_async_get_io_descriptor(&USART_EAST,  &grid_sys_east_io);
	usart_async_get_io_descriptor(&USART_SOUTH, &grid_sys_south_io);
	usart_async_get_io_descriptor(&USART_WEST,  &grid_sys_west_io);
	
	usart_async_enable(&USART_NORTH);
	usart_async_enable(&USART_EAST);
	usart_async_enable(&USART_SOUTH);
	usart_async_enable(&USART_WEST);




}



void grid_sys_dma_rx_init_one(struct grid_port* por, uint32_t buffer_length, void* transfer_done_cb() ){
	
	
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

void grid_sys_dma_rx_init(){
	
	grid_sys_dma_rx_init_one(&GRID_PORT_N, GRID_DOUBLE_BUFFER_RX_SIZE, dma_transfer_complete_n_cb);
	grid_sys_dma_rx_init_one(&GRID_PORT_E, GRID_DOUBLE_BUFFER_RX_SIZE, dma_transfer_complete_e_cb);
	grid_sys_dma_rx_init_one(&GRID_PORT_S, GRID_DOUBLE_BUFFER_RX_SIZE, dma_transfer_complete_s_cb);
	grid_sys_dma_rx_init_one(&GRID_PORT_W, GRID_DOUBLE_BUFFER_RX_SIZE, dma_transfer_complete_w_cb);

	NVIC_SetPriority(DMAC_0_IRQn, 0);
	NVIC_SetPriority(DMAC_1_IRQn, 0);
	NVIC_SetPriority(DMAC_2_IRQn, 0);
	NVIC_SetPriority(DMAC_3_IRQn, 0);
	
}

void grid_sys_init(struct grid_sys_model* mod){
	
	mod->bank_select = 0;
	
	mod->bank_color_r[0] = 200;
	mod->bank_color_g[0] = 100;
	mod->bank_color_b[0] = 0;
	
	mod->bank_color_r[1] = 0;
	mod->bank_color_g[1] = 100;
	mod->bank_color_b[1] = 200;
	
	mod->bank_color_r[2] = 50;
	mod->bank_color_g[2] = 200;
	mod->bank_color_b[2] = 50;
	
	mod->bank_color_r[3] = 100;
	mod->bank_color_g[3] = 0;
	mod->bank_color_b[3] = 200;
	
	
	grid_port_init_all();
	
	grid_sys_uart_init();
	grid_sys_dma_rx_init();
	
}

void grid_sys_bank_select(struct grid_sys_model* mod, uint8_t banknumber){
	
	uint32_t hwtype = grid_sys_get_hwcfg();
	
	if (banknumber == 255){
		
		

		
		mod->bank_select = 255;
		
		for(uint8_t i=0; i<grid_led_get_led_number(&grid_led_state); i++){
			
			if (hwtype == GRID_MODULE_EN16_RevA){
				
				grid_led_set_min(&grid_led_state, i, 0, 0, 0, 255);
				grid_led_set_mid(&grid_led_state, i, 0, 5, 5, 5);
				grid_led_set_max(&grid_led_state, i, 0, 255, 0, 0);
			}
			else{			
				
				uint8_t r = 127;
				uint8_t g = 127;
				uint8_t b = 127;

				
				grid_led_set_min(&grid_led_state, i, 0, r/20, g/20, b/20);
				//grid_led_set_min(&grid_led_state, i, 0, 0, 0, 0);
				
				grid_led_set_mid(&grid_led_state, i, 0, r/2, g/2, b/2);
				grid_led_set_max(&grid_led_state, i, 0, r, g, b);
			}
			
			
		}	
		

	}
	else{
		
		mod->bank_select = banknumber%4;

		
		for(uint8_t i=0; i<grid_led_get_led_number(&grid_led_state); i++){
			
			if (hwtype == GRID_MODULE_EN16_RevA){
				
				
				uint8_t r = mod->bank_color_r[mod->bank_select];
				uint8_t g = mod->bank_color_g[mod->bank_select];
				uint8_t b = mod->bank_color_b[mod->bank_select];
				
				grid_led_set_mid(&grid_led_state, i, 0, r/32, g/32, b/32);
			}
			else{
				
				uint8_t r = mod->bank_color_r[mod->bank_select];
				uint8_t g = mod->bank_color_g[mod->bank_select];
				uint8_t b = mod->bank_color_b[mod->bank_select];

				
				grid_led_set_min(&grid_led_state, i, 0, r/32, g/32, b/32);
				grid_led_set_mid(&grid_led_state, i, 0, r/2, g/2, b/2);
				grid_led_set_max(&grid_led_state, i, 0, r, g, b);
			}
			
			
		}	
		
	}

	
}

void grid_sys_bank_select_next(struct grid_sys_model* mod){
	
	grid_sys_bank_select(mod, (mod->bank_select+1)%4);

}


// REALTIME

uint32_t grid_sys_rtc_get_time(struct grid_sys_model* mod){
	return mod->realtime;
}


void grid_sys_rtc_set_time(struct grid_sys_model* mod, uint32_t tvalue){
	
	mod->realtime = tvalue;
}

uint32_t grid_sys_rtc_get_elapsed_time(struct grid_sys_model* mod, uint32_t t_old){
	
	return mod->realtime-t_old;
	
	

}

void grid_sys_rtc_tick_time(struct grid_sys_model* mod){
	mod->realtime++;
}


// =========================== GRID SYS ALERT ============================== //

uint8_t grid_sys_alert_read_color_changed_flag(struct grid_sys_model* mod){
		
	return mod->alert_color_changed;
	
}

void grid_sys_alert_set_color_changed_flag(struct grid_sys_model* mod){
	
	mod->alert_color_changed = 1;
	
}

void grid_sys_alert_clear_color_changed_flag(struct grid_sys_model* mod){
	
	mod->alert_color_changed = 0;
	
}

uint8_t grid_sys_alert_get_color_intensity(struct grid_sys_model* mod){
	
	if (mod->alert_style == 0){ // TRIANGLE
		
		return (125-abs(mod->alert_state/2-125))/2;
	}
	else if (mod->alert_style == 1){ // SQUARE
		
		return 255*(mod->alert_state/250%2);
	}
	else if (mod->alert_style == 2){ // CONST
		
		return 255*(mod->alert_state>100);
	}
	
	
}

void grid_sys_alert_set_color(struct grid_sys_model* mod, uint8_t red, uint8_t green, uint8_t blue){
	
	grid_sys_alert_set_color_changed_flag(mod);
	
	mod->alert_color_red = red;
	mod->alert_color_green = green;
	mod->alert_color_blue = blue;
		
}

void grid_sys_alert_set_alert(struct grid_sys_model* mod, uint8_t red, uint8_t green, uint8_t blue, uint8_t style, uint16_t duration){
	
	grid_sys_alert_set_color(mod, red, green, blue);

	
	mod->alert_state = duration;
	mod->alert_style = style;
	
}

uint8_t grid_sys_alert_get_color_r(struct grid_sys_model* mod){
	
	return mod->alert_color_red;
}

uint8_t grid_sys_alert_get_color_g(struct grid_sys_model* mod){
	
	return mod->alert_color_green;
}

uint8_t grid_sys_alert_get_color_b(struct grid_sys_model* mod){
	
	return mod->alert_color_blue;
}




uint8_t grid_sys_read_hex_char_value(uint8_t ascii, uint8_t* error_flag){
		
	uint8_t result = 0;
	
	if (ascii>47 && ascii<58){
		result = ascii-48;
	}
	else if(ascii>96 && ascii<103){
		result = ascii - 97 + 10;
	}
	else{
		// wrong input
		if (error_flag != NULL){
			*error_flag = ascii;
		}
	}
	
	return result;	
}

uint32_t grid_sys_read_hex_string_value(uint8_t* start_location, uint8_t length, uint8_t* error_flag){
	
	uint32_t result  = 0;
	
	for(uint8_t i=0; i<length; i++){
		
		result += grid_sys_read_hex_char_value(start_location[i], error_flag) << (length-i-1)*4;

		
	}

	return result;
}

void grid_sys_write_hex_string_value(uint8_t* start_location, uint8_t size, uint32_t value){
	
	uint8_t str[10];
	
	sprintf(str, "%08x", value);
		
	for(uint8_t i=0; i<size; i++){	
		start_location[i] = str[8-size+i];	
	}

}




uint32_t grid_sys_get_id(uint32_t* return_array){
			
	return_array[0] = *(uint32_t*)(GRID_SYS_UNIQUE_ID_ADDRESS_0);
	return_array[1] = *(uint32_t*)(GRID_SYS_UNIQUE_ID_ADDRESS_1);
	return_array[2] = *(uint32_t*)(GRID_SYS_UNIQUE_ID_ADDRESS_2);
	return_array[3] = *(uint32_t*)(GRID_SYS_UNIQUE_ID_ADDRESS_3);
	
	return 1;
	
}

uint32_t grid_sys_get_hwcfg(){
	
	// Read the register for the first time, then later just return the saved value

	if (grid_sys_hwfcg == -1){

		gpio_set_pin_direction(HWCFG_SHIFT, GPIO_DIRECTION_OUT);
		gpio_set_pin_direction(HWCFG_CLOCK, GPIO_DIRECTION_OUT);
		gpio_set_pin_direction(HWCFG_DATA, GPIO_DIRECTION_IN);
			
		// LOAD DATA
		gpio_set_pin_level(HWCFG_SHIFT, 0);
		delay_ms(1);
			
			
			
		uint8_t hwcfg_value = 0;
			
			
		for(uint8_t i = 0; i<8; i++){ // now we need to shift in the remaining 7 values
				
			// SHIFT DATA
			gpio_set_pin_level(HWCFG_SHIFT, 1); //This outputs the first value to HWCFG_DATA
			delay_ms(1);
				
				
			if(gpio_get_pin_level(HWCFG_DATA)){
					
				hwcfg_value |= (1<<i);
					
				}else{
					
					
			}
				
			if(i!=7){
					
				// Clock rise
				gpio_set_pin_level(HWCFG_CLOCK, 1);
					
				delay_ms(1);
					
				gpio_set_pin_level(HWCFG_CLOCK, 0);
			}
							
		}
		
		grid_sys_hwfcg = hwcfg_value;
		
	}

	
	return grid_sys_hwfcg;

}


#define GRID_SYS_NORTH	0
#define GRID_SYS_EAST	1
#define GRID_SYS_SOUTH	2
#define GRID_SYS_WEST	3



void grid_sys_ping_all(){
		
	grid_sys_ping(&GRID_PORT_N);
	grid_sys_ping(&GRID_PORT_E);
	grid_sys_ping(&GRID_PORT_S);
	grid_sys_ping(&GRID_PORT_W);
	
}

uint8_t grid_msg_checksum_calculate(uint8_t* str, uint32_t length){
	
	uint8_t checksum = 0;
	for (uint32_t i=0; i<length-3; i++){
		checksum ^= str[i];
	}
	
	return checksum;
	
}

uint8_t grid_msg_checksum_read(uint8_t* str, uint32_t length){
	uint8_t error_flag;
	return grid_sys_read_hex_string_value(&str[length-3], 2, &error_flag);
}

void grid_msg_checksum_write(uint8_t* message, uint32_t length, uint8_t checksum){
	
// 	uint8_t checksum_string[4];
// 
// 	sprintf(checksum_string, "%02x", checksum);
// 
// 	message[length-3] = checksum_string[0];
// 	message[length-2] = checksum_string[1];
	
	grid_sys_write_hex_string_value(&message[length-3], 2, checksum);
	
}


// MESSAGE PARAMETER FUNCTIONS

uint8_t grid_msg_get_id(uint8_t* message){
	
	uint8_t error = 0;
	return grid_sys_read_hex_string_value(&message[4], 2, &error);
	
}
uint8_t grid_msg_get_dx(uint8_t* message){
	
	uint8_t error = 0;
	return grid_sys_read_hex_string_value(&message[6], 2, &error);	
	
}
uint8_t grid_msg_get_dy(uint8_t* message){
	
	uint8_t error = 0;
	return grid_sys_read_hex_string_value(&message[8], 2, &error);	

}
uint8_t grid_msg_get_age(uint8_t* message){
	
	uint8_t error = 0;
	return grid_sys_read_hex_string_value(&message[10], 2, &error);	
	
}

void grid_msg_set_id(uint8_t* message, uint8_t param){
	
	grid_sys_write_hex_string_value(&message[4], 2, param);
	
}
void grid_msg_set_dx(uint8_t* message, uint8_t param){
	
	grid_sys_write_hex_string_value(&message[6], 2, param);
	
}
void grid_msg_set_dy(uint8_t* message, uint8_t param){

	grid_sys_write_hex_string_value(&message[8], 2, param);

}
void grid_msg_set_age(uint8_t* message, uint8_t param){
	
	grid_sys_write_hex_string_value(&message[10], 2, param);
	
}

// RECENT MESSAGES

uint8_t grid_msg_find_recent(struct grid_sys_model* model, uint32_t fingerprint){
	
	for(GRID_SYS_RECENT_MESSAGES_INDEX_T i = 0; i<GRID_SYS_RECENT_MESSAGES_LENGTH; i++){
		
		if (model->recent_messages[i%GRID_SYS_RECENT_MESSAGES_LENGTH] == fingerprint){
			
			return 1;
			
		}
		
	}
	
	return 0;
}

void grid_msg_push_recent(struct grid_sys_model* model, uint32_t fingerprint){
	
	model->recent_messages_index+=1;
	model->recent_messages_index%=GRID_SYS_RECENT_MESSAGES_LENGTH;
	
	model->recent_messages[model->recent_messages_index] = fingerprint;
	
}


// void grid_sys_ping(struct grid_port* por){
// 		
// 		
// 	uint8_t length = 16;
// 	uint32_t hwcfg = grid_sys_get_hwcfg();
// 	char message[16] = {GRID_MSG_START_OF_HEADING, GRID_MSG_DIRECT, GRID_MSG_BELL, por->direction, '0','0','0','0','0','0','0','0',GRID_MSG_END_OF_TRANSMISSION,'0','0','\n'};
// 	
// 	
// 	//char message[20];	
// 	// Create the packet
// 	//sprintf(message, "%c%c%c%c%08x%c00\n", GRID_MSG_START_OF_HEADING, GRID_MSG_DIRECT, GRID_MSG_BELL, por->direction ,hwcfg, GRID_MSG_END_OF_TRANSMISSION);
// 	//length = strlen(message);
// 	
// 
// 	grid_sys_write_hex_string_value(&message[4], 8, hwcfg);
// 	
// 
// 	
//  	grid_msg_set_checksum(message, length, grid_msg_get_checksum(message, length));
// 		
// 	// Put the packet into the tx_buffer
// 	if (grid_buffer_write_init(&por->tx_buffer, length)){
// 		
// 		for(uint16_t i = 0; i<length; i++){
// 			
// 			grid_buffer_write_character(&por->tx_buffer, message[i]);
// 		}
// 		
// 		grid_buffer_write_acknowledge(&por->tx_buffer);
// 	}
// 				
// }
// 
