/*
 * grid_buf.c
 *
 * Created: 4/12/2019 5:27:13 PM
 * Author : SUKU WC
*/

#include "grid_buf.h"


// PORTS

GRID_PORT_t GRID_PORT_N;
GRID_PORT_t GRID_PORT_E;
GRID_PORT_t GRID_PORT_S;
GRID_PORT_t GRID_PORT_W;

GRID_PORT_t GRID_PORT_U;
GRID_PORT_t GRID_PORT_H;


#define GRID_BUFFER_TX_SIZE	200 
#define GRID_BUFFER_RX_SIZE	200

uint8_t usb_tx_double_buffer[GRID_BUFFER_TX_SIZE];
uint8_t usb_rx_double_buffer[GRID_BUFFER_RX_SIZE];


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


uint16_t grid_buffer_write_size(GRID_BUFFER_t* buf){
	
	
	
	uint16_t space = 0;
	
	if (buf->read_start > buf->write_start){
		space = buf->read_start - buf->write_start;
	}
	else{
		space = buf->buffer_length - buf->write_start + buf->read_start;
	}

	return space;

	
	
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


uint16_t grid_buffer_read_size(GRID_BUFFER_t* buf){
	
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
						
			return i+1; // packet length
			
		}
		
		
	}
	
	while(1){
		// TRAP: TRANSMISSION WAS NOT OVER YET
	}
	
	
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



void grid_port_init(GRID_PORT_t* por, uint16_t tx_buf_size, uint16_t rx_buf_size){
	
	grid_buffer_init(&por->tx_buffer, tx_buf_size);
	grid_buffer_init(&por->rx_buffer, rx_buf_size);
	
	por->partner_dx = 0;
	por->partner_dy = 0;
	por->partner_fi = 0;
	
	por->partner_hwcfg = 0;
	por->partner_status = 0;
	
}




void grid_port_init_all(){
	
	grid_port_init(&GRID_PORT_N, GRID_BUFFER_TX_SIZE*4, GRID_BUFFER_RX_SIZE*4);
	grid_port_init(&GRID_PORT_E, GRID_BUFFER_TX_SIZE*4, GRID_BUFFER_RX_SIZE*4);
	grid_port_init(&GRID_PORT_S, GRID_BUFFER_TX_SIZE*4, GRID_BUFFER_RX_SIZE*4);
	grid_port_init(&GRID_PORT_W, GRID_BUFFER_TX_SIZE*4, GRID_BUFFER_RX_SIZE*4);
	
	grid_port_init(&GRID_PORT_U, GRID_BUFFER_TX_SIZE*4, GRID_BUFFER_RX_SIZE*4);
	grid_port_init(&GRID_PORT_H, GRID_BUFFER_TX_SIZE*4, GRID_BUFFER_RX_SIZE*4);	
	
	GRID_PORT_U.partner_status = 1; // UI IS ALWAYS CONNECTED
	GRID_PORT_H.partner_status = 1; // HOST IS ALWAYS CONNECTED (Not really!)
	
	
}

uint8_t grid_port_process_inbound(GRID_PORT_t* por){
	
	uint8_t port_count = 1;
	GRID_PORT_t* port_array[port_count];
	
	port_array[0] = &GRID_PORT_H;
	
	
	uint16_t packet_size = grid_buffer_read_size(&por->rx_buffer);
	
	if (!packet_size){
		
		// NO PACKET IN RX BUFFER
		return 0;
		 
	}else{
			
		

			
		// Check all of the tx buffers for sufficient storage space
		
		for (uint8_t i=0; i<port_count; i++)
		{
			if (packet_size > grid_buffer_write_size(&port_array[i]->tx_buffer)){				
				// sorry one of the buffers cannot store the packet, we will try later
				return 0;				
			}			
		}
			
		// Let's init all of the buffers for transaction
		
		if (packet_size != grid_buffer_read_init(&por->rx_buffer)){
			while(1){			
				// TRAP: WTF
			}
		}
		
		
		 
		
		for (uint8_t i=0; i<port_count; i++)
		{
			grid_buffer_write_init(&port_array[i]->tx_buffer, packet_size);
		}
		
		// Let's do the transaction
												
		for (uint16_t i=0; i<packet_size; i++)
		{
			uint8_t character = grid_buffer_read_character(&por->rx_buffer);
			
			for (uint8_t j=0; j<port_count; j++){
				
				grid_buffer_write_character(&port_array[j]->tx_buffer, character);
				
			}
								
		}
			
								
		// Let's acknowledge all of the transactions
					
		grid_buffer_read_acknowledge(&por->rx_buffer);
					
		for (uint8_t i=0; i<port_count; i++)
		{
			grid_buffer_write_acknowledge(&port_array[i]->tx_buffer);
		}	

		
	}
		
}

uint8_t grid_port_process_outbound_usb(GRID_PORT_t* por){
	
	uint16_t packet_size = grid_buffer_read_size(&por->tx_buffer);
		
	if (!packet_size){
			
		// NO PACKET IN RX BUFFER
		return 0;
	}else{
				
		// Let's transfer the packet to local memory
		grid_buffer_read_init(&por->tx_buffer);		
		
		for (uint8_t i = 0; i<packet_size; i++){
			
			uint8_t character = grid_buffer_read_character(&por->tx_buffer);
			usb_tx_double_buffer[i] = character;
			
		}
		
		// Let's send the packet through USB
				
		cdcdf_acm_write(usb_tx_double_buffer, packet_size);

			
		// Let's acknowledge the transactions	(should wait for partner to send ack)	
		grid_buffer_read_acknowledge(&por->tx_buffer);		
		
	}
	
	
}


