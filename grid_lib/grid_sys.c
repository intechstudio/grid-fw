/*
 * grid_sys.c
 *
 * Created: 4/12/2019 5:27:13 PM
 * Author : SUKU WC
*/

#include "grid_sys.h"
#include "grid_buf.h"
#include "stdio.h"
#include "string.h"




uint8_t grid_sys_error_intensity(struct grid_sys_model* mod){
	
	if (mod->error_style == 0){ // TRIANGLE
		
		return (125-abs(mod->error_state/2-125))/2;
	}
	else if (mod->error_style == 1){ // SQUARE
		
		return 255*(mod->error_state/250%2);
	}
	else if (mod->error_style == 2){ // CONST
		
		return 255*(mod->error_state>100);
	}
	
	
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

volatile uint8_t grid_sys_ping_counter[4] = {0, 0, 0, 0};

volatile uint32_t grid_sys_rx_counter[4] = {0, 0, 0, 0};
volatile uint32_t grid_sys_tx_counter[4] = {0, 0, 0, 0};



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

void tx_cb_USART_GRID(GRID_PORT_t* const por){
	
	grid_sys_tx_counter[por->direction - 0x11]++;

	
	if(por->tx_double_buffer[1] == GRID_MSG_BROADCAST){
		// SHOULD WAIT FOR ACKNOWLEDGE
		
		// BUT WE DON'T
		for(uint8_t i=0; i<GRID_DOUBLE_BUFFER_TX_SIZE; i++){
			por->tx_double_buffer[i] = 0;
		}
		
		por->tx_double_buffer_status = 0;
	}
	else if (por->tx_double_buffer[1] == GRID_MSG_DIRECT){
		// NO NEED TO WAIT FOR ACKNOWLEDGE
		for(uint8_t i=0; i<GRID_DOUBLE_BUFFER_TX_SIZE; i++){
			por->tx_double_buffer[i] = 0;
		}
		
		por->tx_double_buffer_status = 0;
	}
	else{
		//TRAP
		for(uint8_t i=0; i<GRID_DOUBLE_BUFFER_TX_SIZE; i++){
			por->tx_double_buffer[i] = 0;
		}
		
		por->tx_double_buffer_status = 0;
	}
	
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

void rx_cb_USART_GRID(GRID_PORT_t* const por){
			
}





void grid_port_process_inbound_all(){
	
// 	grid_port_process_inbound(&GRID_PORT_N);
// 	grid_port_process_inbound(&GRID_PORT_E);
// 	grid_port_process_inbound(&GRID_PORT_S);
// 	grid_port_process_inbound(&GRID_PORT_W);
		
}	

void grid_sys_ping_all(){
		
	grid_sys_ping(&GRID_PORT_N);
	grid_sys_ping(&GRID_PORT_E);
	grid_sys_ping(&GRID_PORT_S);
	grid_sys_ping(&GRID_PORT_W);
	
}

uint8_t grid_sys_calculate_checksum(char* str, uint32_t len){
	
	uint8_t checksum = 0;
	for (uint32_t i=0; i<len; i++){
		checksum ^= str[i]; 
	}
	
	return checksum;
	
}


void grid_sys_ping(GRID_PORT_t* por){
	
	char str[20];
	uint8_t len = 0;
	
	// Create the packet
	sprintf(str, "%c%c%c%c%08x%c", GRID_MSG_START_OF_HEADING, GRID_MSG_DIRECT, GRID_MSG_BELL, por->direction ,grid_sys_get_hwcfg(), GRID_MSG_END_OF_TRANSMISSION);
	
	// Calculate packet length
	len = strlen(str);
	
	// Concatonate the calculated CHECKSUM + \n
	sprintf(&str[len], "%02x\n", grid_sys_calculate_checksum(str, len));
	
	// Calculate the new packet length
	len += strlen(&str[len]);
	
	// Put the packet into the tx_buffer
	if (grid_buffer_write_init(&por->tx_buffer, len)){
		
		for(uint16_t i = 0; i<len; i++){
			
			grid_buffer_write_character(&por->tx_buffer, str[i]);
		}
		
		grid_buffer_write_acknowledge(&por->tx_buffer);
	}
				
	
}


	
	
	
	