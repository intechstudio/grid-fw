/*
 * grid_buf.c
 *
 * Created: 4/12/2019 5:27:13 PM
 * Author : SUKU WC
*/

#include "grid_buf.h"



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

#define GRID_BUFFER_TX_SIZE	200 
#define GRID_BUFFER_RX_SIZE	200



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
		
		buf->write_stop = (buf->write_start+length)%buf->buffer_length;
		
		return length;
	}
	else{
		return 0; // failed
	}
	
	
}

uint8_t grid_buffer_write_character(GRID_BUFFER_t* buf, uint8_t character){
	

		
	buf->buffer_storage[buf->write_active] = character;
		
	buf->write_active++;
	buf->write_active %= buf->buffer_length;
		
	return 1;
		

}

uint8_t grid_buffer_write_acknowledge(GRID_BUFFER_t* buf){
	
	if (buf->write_active == buf->write_stop){
		
		
		buf->write_start = buf->write_active;
		return 1;
	}
	else{
		
		while(1){
			
			//TRAP xx
		}
	}
	
	
}

uint8_t grid_buffer_write_cancel(GRID_BUFFER_t* buf){
	
	buf->write_active = buf->write_start;
	buf->write_stop   = buf->write_start;
	
	return 1;
}



uint16_t grid_buffer_read_init(GRID_BUFFER_t* buf){
	
	if (buf->read_active != buf->read_stop) {
		while(1){
		// TRAP: TRANSMISSION WAS NOT OVER YET
		}	
	}
	
	
	if (buf->read_start	 != buf->read_stop) {
		while(1){
		// TRAP: TRANSMISSION WAS NOT OVER YET
		}	
	}
	
	if (buf->read_start == buf->write_start) {
		return 0;
	}
	
	
	
	// Seek message end character	
	for (uint16_t i=0; i<buf->buffer_length; i++){
		
		uint16_t index = (buf->read_start + i)%buf->buffer_length;
			
		// Hit the write pointer, no message
		if (index == buf->write_start) return 0;	
					
		if (buf->buffer_storage[index] == '\n'){
								
			buf->read_stop = (index+1)%buf->buffer_length;
					
			buf->read_length = i+1;
					
			return buf->read_length; // packet length
				
		}
		
		
	}
		
	while(1){
		// TRAP: TRANSMISSION WAS NOT OVER YET
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
