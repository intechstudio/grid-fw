/*
 * grid_sys.c
 *
 * Created: 4/12/2019 5:27:13 PM
 * Author : SUKU WC
*/

#include "grid_sys.h"

// NORTH
GRID_BUFFER_t GRID_BUFFER_N_TX;
GRID_BUFFER_t GRID_BUFFER_N_RX;

// EAST
GRID_BUFFER_t GRID_BUFFER_E_TX;
GRID_BUFFER_t GRID_BUFFER_E_RX;

// SOUTH
GRID_BUFFER_t GRID_BUFFER_S_TX;
GRID_BUFFER_t GRID_BUFFER_S_RX;

// WEST
GRID_BUFFER_t GRID_BUFFER_W_TX;
GRID_BUFFER_t GRID_BUFFER_W_RX;

// USER INTERFACE
GRID_BUFFER_t GRID_BUFFER_U_TX;
GRID_BUFFER_t GRID_BUFFER_U_RX;

// HOST (PC)
GRID_BUFFER_t GRID_BUFFER_H_TX;
GRID_BUFFER_t GRID_BUFFER_H_RX;

#define GRID_BUFFER_TX_SIZE	1024
#define GRID_BUFFER_RX_SIZE	1024



uint8_t grid_buffer_init(struct grid_buffer* buf, uint16_t length){
	
	buf->buffer_length = length;
	
	buf->read_length   = 0;
	
	buf->read_start    = 0;
	buf->read_stop     = 0;
	buf->read_active   = 0;
	
	buf->write_start    = 0;
	buf->write_stop     = 0;
	buf->write_active   = 0;
	

	buf->buffer_storage = (uint8_t*) malloc(sizeof(uint8_t)*buf->buffer_length);
	
	while (buf->buffer_storage == NULL){
		// TRAP: MALLOC FAILED
	}

	for (uint16_t i=0; i<buf->buffer_length; i++){
		buf->buffer_storage[i] = 0;
	}
	
	return 1;
	
}


uint8_t grid_buffer_init_all(){
	
	
	grid_buffer_init(&GRID_BUFFER_N_TX, GRID_BUFFER_TX_SIZE*4);
	grid_buffer_init(&GRID_BUFFER_N_RX, GRID_BUFFER_RX_SIZE*4);
	
	grid_buffer_init(&GRID_BUFFER_E_TX, GRID_BUFFER_TX_SIZE*4);
	grid_buffer_init(&GRID_BUFFER_E_RX, GRID_BUFFER_RX_SIZE*4);
	
	grid_buffer_init(&GRID_BUFFER_S_TX, GRID_BUFFER_TX_SIZE*4);
	grid_buffer_init(&GRID_BUFFER_S_RX, GRID_BUFFER_RX_SIZE*4);
	
	grid_buffer_init(&GRID_BUFFER_W_TX, GRID_BUFFER_TX_SIZE*4);
	grid_buffer_init(&GRID_BUFFER_W_RX, GRID_BUFFER_RX_SIZE*4);
	
	grid_buffer_init(&GRID_BUFFER_U_TX, GRID_BUFFER_TX_SIZE*1);
	grid_buffer_init(&GRID_BUFFER_U_RX, GRID_BUFFER_RX_SIZE*4);
	
	grid_buffer_init(&GRID_BUFFER_H_TX, GRID_BUFFER_TX_SIZE*8);
	grid_buffer_init(&GRID_BUFFER_H_RX, GRID_BUFFER_RX_SIZE*8);
	
	return 1;
	
}


uint16_t grid_buffer_write_init(GRID_BUFFER_t* buf, uint16_t length){
	
	uint16_t space = 0;
	
	if (buf->read_start > buf->write_start){
		space = buf->read_start - buf->write_start;
	}
	else{
		space = buf->buffer_length - buf->write_start + buf->read_start;
	}
	
	
	
	if (space>length){
		return length;	
	}
	else{
		return 0; // failed
	}
	
	
}

uint8_t grid_buffer_write_character(GRID_BUFFER_t* buf, uint8_t character){
	
	if (buf->write_active != buf->read_start){
		
		buf->buffer_storage[buf->write_active] = character;
		
		buf->write_active++;
		buf->write_active %= buf->buffer_length;
		
		return 1;
		
	}
	else{
		
		while(1){
			//TRAP: BUFFER OVERRUN
		}
		
	}
	
	
	return 1;
}

uint8_t grid_buffer_write_acknowledge(GRID_BUFFER_t* buf){
	
	buf->write_start = buf->write_active;
	buf->write_stop  = buf->write_active;
	
	return 1;
}

uint8_t grid_buffer_write_cancel(GRID_BUFFER_t* buf){
	
	buf->write_active = buf->write_start;
	buf->write_stop   = buf->write_start;
	
	return 1;
}



uint16_t grid_buffer_read_init(GRID_BUFFER_t* buf){
	
	if (buf->read_active == buf->read_stop && buf->read_start == buf->read_stop){
		
		for (uint16_t i=1; i<buf->buffer_length; i++){
			
			if (buf->buffer_storage[(buf->read_start + i)%buf->buffer_length] == 0){
				
				if (i>1){ // (found the write start buffer)
										
					buf->read_stop = (buf->read_start + i)%buf->buffer_length;
					buf->read_start++;
					buf->read_start%=buf->buffer_length;
					buf->read_active = buf->read_start;
					
					buf->read_length = i-1;
					
					return i-1; // packet length
					
				}
				else{
					
					return 0;
				}
				
			}
		}
		
	}else{
		
		while(1){
			// TRAP: TRANSMISSION WAS NOT OVER YET
		}
	}
	
}


uint8_t grid_buffer_read_character(GRID_BUFFER_t* buf){
	
	// Check if packet is not over
	if (buf->read_active != buf->read_stop){
		
		uint8_t character = buf->buffer_storage[buf->read_active];
		
		buf->read_active++;
		buf->read_active %= buf->buffer_length;
				
		return character;
		
	}else{
		
		while(1){
			// TRAP: TRANSMISSION WAS OVER ALREADY
		}
	}
	

}

// TRANSMISSION WAS ACKNOWLEDGED, PACKET CAN BE DELETED
uint8_t grid_buffer_read_acknowledge(GRID_BUFFER_t* buf){
	
	// Check if packet is really over
	if (buf->read_active == buf->read_stop){			
		buf->read_start = buf->read_stop;
		return 1;
	}else{
		
		while(1){
			// TRAP: TRANSMISSION WAS NOT OVER YET
		}
	}
	

}


// JUMP BACK TO THE BEGINNING OF THE PACKET
uint8_t grid_buffer_read_nacknowledge(GRID_BUFFER_t* buf){
	
	buf->read_active = buf->read_start;
	
	return 1;
}


// DISCARD PACKET
uint8_t grid_buffer_read_cancel(GRID_BUFFER_t* buf){
	
	buf->read_active = buf->read_stop;
	buf->read_start  = buf->read_stop;
	
	return 1;
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

struct io_descriptor *grid_sys_north_io;
struct io_descriptor *grid_sys_east_io;
struct io_descriptor *grid_sys_south_io;
struct io_descriptor *grid_sys_west_io;

static uint8_t grid_sys_pingmessage[2] = "P";

volatile uint8_t grid_sys_ping_counter[4] = {0, 0, 0, 0};

volatile uint32_t grid_sys_rx_counter[4] = {0, 0, 0, 0};
volatile uint32_t grid_sys_tx_counter[4] = {0, 0, 0, 0};




static void tx_cb_USART_GRID(const struct usart_async_descriptor *const descr)
{
	/* Transfer completed */
		
	if (descr == &USART_EAST){
		grid_sys_tx_counter[GRID_SYS_EAST]++;
	}
	if (descr == &USART_WEST){
		grid_sys_tx_counter[GRID_SYS_WEST]++;
	}
	
}

static void rx_cb_USART_GRID(const struct usart_async_descriptor *const descr)
{
	/* Transfer completed */
	
	if (descr == &USART_EAST){
		grid_sys_rx_counter[GRID_SYS_EAST]++;
	}
	if (descr == &USART_WEST){
		grid_sys_rx_counter[GRID_SYS_WEST]++;
	}
	
}

void grid_sys_uart_init(){

	usart_async_register_callback(&USART_EAST, USART_ASYNC_TXC_CB, tx_cb_USART_GRID);
	usart_async_register_callback(&USART_EAST, USART_ASYNC_RXC_CB, rx_cb_USART_GRID);	
	
	usart_async_get_io_descriptor(&USART_EAST, grid_sys_east_io);
	
	usart_async_enable(&USART_EAST);
	
	
	usart_async_register_callback(&USART_WEST, USART_ASYNC_TXC_CB, tx_cb_USART_GRID);
	usart_async_register_callback(&USART_WEST, USART_ASYNC_RXC_CB, rx_cb_USART_GRID);
	
	usart_async_get_io_descriptor(&USART_WEST, grid_sys_west_io);
	
	usart_async_enable(&USART_WEST);

}

	
void grid_msg_process_all(){
	
	grid_msg_process(&USART_EAST, GRID_SYS_EAST);
	grid_msg_process(&USART_WEST, GRID_SYS_WEST);
		
}	

void grid_msg_process(const struct usart_async_descriptor *const descr, uint8_t offset){
	
	uint8_t character;
	
	
	if (usart_async_is_rx_not_empty(descr)){
		
		while(io_read(&(*descr).io, &character, 1) == 1){
			
			if (character == GRID_MSG_PING){
				
				grid_sys_ping_counter[offset] += 1;
				
				
			}
			
		}	
		
	}
	

	
	
}

void grid_sys_ping_all(){
	
	grid_sys_ping(&USART_EAST);
	grid_sys_ping(&USART_WEST);

}

void grid_sys_ping(const struct usart_async_descriptor *const descr){
	
		
	io_write(&(*descr).io, grid_sys_pingmessage ,1);
	
}


	
	
	
	