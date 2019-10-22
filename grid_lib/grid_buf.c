/*
 * grid_buf.c
 *
 * Created: 4/12/2019 5:27:13 PM
 * Author : SUKU WC
*/

#include "grid_buf.h"
#include "sam.h"

// PORTS




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



void grid_port_init(GRID_PORT_t* por, uint16_t tx_buf_size, uint16_t rx_buf_size, struct usart_async_descriptor*  usart, uint8_t type, uint8_t dir, uint8_t dma){
	
	grid_buffer_init(&por->tx_buffer, tx_buf_size);
	grid_buffer_init(&por->rx_buffer, rx_buf_size);
	
	por->dma_channel = dma;
	
	por->direction = dir;
	
	por->usart	= usart;
	por->type		= type;
	
	por->tx_double_buffer_status	= 0;
	por->rx_double_buffer_status	= 0;
	
	
	por->partner_fi = 0;
	
	por->partner_hwcfg = 0;
	por->partner_status = 1;
	
	
	
	if (type == GRID_PORT_TYPE_USART){	
		
		por->partner_status = 0;
		por->partner_fi = 0;
		
		
		if (por->direction == GRID_MSG_NORTH){
			por->dx = 0;
			por->dy = 1;
		}
		else if (por->direction == GRID_MSG_EAST){
			por->dx = 1;
			por->dy = 0;
		}
		else if (por->direction == GRID_MSG_SOUTH){
			por->dx = 0;
			por->dy = -1;
		}
		else if (por->direction == GRID_MSG_WEST){
			por->dx = -1;
			por->dy = 0;
		}
		
	}
	else{
		por->partner_status = 1; //UI AND USB are considered to be connected by default
	}
	
}




void grid_port_init_all(){
	
	grid_port_init(&GRID_PORT_N, GRID_BUFFER_TX_SIZE*4, GRID_BUFFER_RX_SIZE*4, &USART_NORTH, GRID_PORT_TYPE_USART, GRID_MSG_NORTH ,0);
	grid_port_init(&GRID_PORT_E, GRID_BUFFER_TX_SIZE*4, GRID_BUFFER_RX_SIZE*4, &USART_EAST,  GRID_PORT_TYPE_USART, GRID_MSG_EAST  ,1);
	grid_port_init(&GRID_PORT_S, GRID_BUFFER_TX_SIZE*4, GRID_BUFFER_RX_SIZE*4, &USART_SOUTH, GRID_PORT_TYPE_USART, GRID_MSG_SOUTH ,2);
	grid_port_init(&GRID_PORT_W, GRID_BUFFER_TX_SIZE*4, GRID_BUFFER_RX_SIZE*4, &USART_WEST,  GRID_PORT_TYPE_USART, GRID_MSG_WEST  ,3);
	
	grid_port_init(&GRID_PORT_U, GRID_BUFFER_TX_SIZE*4, GRID_BUFFER_RX_SIZE*4, NULL, GRID_PORT_TYPE_UI, 0, -1);
	grid_port_init(&GRID_PORT_H, GRID_BUFFER_TX_SIZE*4, GRID_BUFFER_RX_SIZE*4, NULL, GRID_PORT_TYPE_USB, 0, -1);	
	
	GRID_PORT_U.partner_status = 1; // UI IS ALWAYS CONNECTED
	GRID_PORT_H.partner_status = 1; // HOST IS ALWAYS CONNECTED (Not really!)
	
	
}


//=============================== PROCESS INBOUND ==============================//

uint8_t grid_port_process_inbound(GRID_PORT_t* por){
	
	uint16_t packet_size = grid_buffer_read_size(&por->rx_buffer);
	
	if (!packet_size){
		
		// NO PACKET IN RX BUFFER
		return 0;
		 
	}else{
			
		uint8_t port_count = 6;
		GRID_PORT_t* port_array_default[port_count];
		GRID_PORT_t* port_array[port_count];
		
		
		port_array_default[0] = &GRID_PORT_N;
		port_array_default[1] = &GRID_PORT_E;
		port_array_default[2] = &GRID_PORT_S;
		port_array_default[3] = &GRID_PORT_W;
		
		port_array_default[4] = &GRID_PORT_U;
		port_array_default[5] = &GRID_PORT_H;
		
		uint8_t j=0;
		
		for(uint8_t i=0; i<port_count; i++){
			if (port_array_default[i]->partner_status != 0){
				port_array[j] = port_array_default[i];
				j++;
			}	
		}
		port_count = j;
		

			
		// Check all of the tx buffers for sufficient storage space
		
		for (uint8_t i=0; i<port_count; i++)
		{
			if (port_array[i] != por){
			
				if (packet_size > grid_buffer_write_size(&port_array[i]->tx_buffer)){
					// sorry one of the buffers cannot store the packet, we will try later
					return 0;
				}	
			}	
		}
		
		if (packet_size != grid_buffer_read_init(&por->rx_buffer)){
			while(1){			
				// TRAP: WTF
			}
		}
		
		// Let's init all of the buffers for transaction		 
		
		for (uint8_t i=0; i<port_count; i++)
		{
			if (port_array[i] != por){
				grid_buffer_write_init(&port_array[i]->tx_buffer, packet_size);
			}
		}
		
		// Let's do the transaction
												
		for (uint16_t j=0; j<packet_size; j++)
		{
			
			uint8_t character = grid_buffer_read_character(&por->rx_buffer);
				
			for (uint8_t i=0; i<port_count; i++){
				if (port_array[i] != por){
					grid_buffer_write_character(&port_array[i]->tx_buffer, character);
					
				}
			}					
		}
			
								
		// Let's acknowledge all of the transactions					
		grid_buffer_read_acknowledge(&por->rx_buffer);
					
		for (uint8_t i=0; i<port_count; i++)
		{
			if (port_array[i] != por){
				grid_buffer_write_acknowledge(&port_array[i]->tx_buffer);
			}
		}	

		
	}
		
}


//=============================== PROCESS OUTBOUND ==============================//

volatile uint8_t temp[500];

uint8_t grid_port_process_outbound_usb(GRID_PORT_t* por){
	
	uint16_t length = grid_buffer_read_size(&por->tx_buffer);
	
	if (!length){		
		// NO PACKET IN RX BUFFER
		return 0;	
	}
	
	


	if (length){
		
		//uint8_t temp[length];
		
		// Let's transfer the packet to local memory
		grid_buffer_read_init(&por->tx_buffer);
		
		for (uint8_t i = 0; i<length; i++){
			
			temp[i] = grid_buffer_read_character(&por->tx_buffer);
			
		}
				
		// Let's acknowledge the transactions	(should wait for partner to send ack)
		grid_buffer_read_acknowledge(&por->tx_buffer);
		
		
		
		// GRID-2-HOST TRANSLATOR
		uint8_t id = grid_msg_get_id(temp);		
		int8_t dx = grid_msg_get_dx(temp) - GRID_SYS_DEFAULT_POSITION;
		int8_t dy = grid_msg_get_dy(temp) - GRID_SYS_DEFAULT_POSITION;		
		uint8_t age = grid_msg_get_age(temp);
		
				
		uint8_t current_protocol	= 0;
		uint8_t current_start		= 0;
		uint8_t current_stop		= 0;
		
		uint8_t output_cursor = 0;
		
		uint8_t error_flag = 0;
							
		for (uint16_t i=0; i<length; i++){
			
			if (temp[i] == GRID_MSG_START_OF_TEXT){
				current_start = i;
			}
			else if (temp[i] == GRID_MSG_END_OF_TEXT && current_start!=0){
				current_stop = i;
				uint8_t msg_protocol = grid_sys_read_hex_string_value(&temp[current_start+1], 2, &error_flag);			
				
				if (msg_protocol == GRID_MSG_PROTOCOL_MIDI){
					
				
					uint8_t midi_channel = grid_sys_read_hex_string_value(&temp[current_start+3], 2, &error_flag);
					uint8_t midi_command = grid_sys_read_hex_string_value(&temp[current_start+5], 2, &error_flag);
					uint8_t midi_param1  = grid_sys_read_hex_string_value(&temp[current_start+7], 2, &error_flag);
					uint8_t midi_param2  = grid_sys_read_hex_string_value(&temp[current_start+9], 2, &error_flag);
					
										
					sprintf(&por->tx_double_buffer[output_cursor], "[GRID] %3d %4d %4d %d [MIDI] Ch: %d  Cmd: %d  Param1: %d  Param2: %d\n",					
						id,dx,dy,age,
						midi_channel,
						midi_command,
						midi_param1,
						midi_param2
					);
					
					output_cursor += strlen(&por->tx_double_buffer[output_cursor]);		
							
												
					//audiodf_midi_xfer_packet(0x08, 0x80, 0x64, 0x0);	
									
				}
				else if (msg_protocol == GRID_MSG_PROTOCOL_KEYBOARD){
		
					uint8_t key_array_length = (current_stop-current_start-3)/6;
		
 					struct hiddf_kb_key_descriptors key_array[key_array_length];
		
					for(uint8_t j=0; j<key_array_length; j++){
						
						uint8_t keyboard_command	= grid_sys_read_hex_string_value(&temp[current_start+3+6*j], 2, &error_flag);
						uint8_t keyboard_modifier	= grid_sys_read_hex_string_value(&temp[current_start+5+6*j], 2, &error_flag);
						uint8_t keyboard_key		= grid_sys_read_hex_string_value(&temp[current_start+7+6*j], 2, &error_flag);
						
						sprintf(&por->tx_double_buffer[output_cursor], "[GRID] %3d %4d %4d %d [KEYBOARD] Key: %d Mod: %d Cmd: %d\n", 
							id,dx,dy,age,
							keyboard_key, keyboard_modifier, keyboard_command
						);	
										
						output_cursor += strlen(&por->tx_double_buffer[output_cursor]);
						
						struct hiddf_kb_key_descriptors current_key = {keyboard_key, keyboard_modifier == GRID_MSG_PROTOCOL_KEYBOARD_PARAMETER_MODIFIER, keyboard_command == GRID_MSG_PROTOCOL_KEYBOARD_COMMAND_KEYDOWN};
								
						key_array[j] = current_key;
						
					}
										
					hiddf_keyboard_keys_state_change(key_array, key_array_length);
		
					
				
				}
				else if (msg_protocol == GRID_MSG_PROTOCOL_MOUSE){
					
					//hiddf_mouse_move(-20, HID_MOUSE_X_AXIS_MV);
					
				}	
				else{
					sprintf(&por->tx_double_buffer[output_cursor], "[UNKNOWN] -> Protocol: %d\n", msg_protocol);
					
					output_cursor += strlen(&por->tx_double_buffer[output_cursor]);		
				}
				
				current_start = 0;
				current_stop = 0;
			}
// 			else if (temp[i] == 0 || temp[i] == '\n'){
// 				break;
// 			}
			
						
		}		
		
		
					
		
		// Let's send the packet through USB
		cdcdf_acm_write(por->tx_double_buffer, output_cursor);
				
		
	}
	
	
}

uint8_t grid_port_process_outbound_ui(GRID_PORT_t* por){
	
	// DUMMY HANDLER, DOES NOT DO ANYTHING  !!!!!!!!!!!!!!
	
	uint16_t packet_size = grid_buffer_read_size(&por->tx_buffer);
	
	if (!packet_size){
		
		// NO PACKET IN RX BUFFER
		return 0;
		}else{
		
		// Let's transfer the packet to local memory
		grid_buffer_read_init(&por->tx_buffer);
		
		for (uint8_t i = 0; i<packet_size; i++){
			
			uint8_t character = grid_buffer_read_character(&por->tx_buffer);
			//usb_tx_double_buffer[i] = character;
			
		}

		//cdcdf_acm_write(usb_tx_double_buffer, packet_size);

		
		// Let's acknowledge the transactions	(should wait for partner to send ack)
		grid_buffer_read_acknowledge(&por->tx_buffer);
		
	}
	
	
}

uint8_t grid_port_process_outbound_usart(GRID_PORT_t* por){
	
	if (por->tx_double_buffer_status == 0){ // READY TO SEND MESSAGE, NO TRANSMISSION IS IN PROGRESS
		
		uint16_t packet_size = grid_buffer_read_size(&por->tx_buffer);
		
		if (!packet_size){
			
			// NO PACKET IN RX BUFFER
			return 0;
		}else{
			
			// Let's transfer the packet to local memory
			grid_buffer_read_init(&por->tx_buffer);
			
			por->tx_double_buffer_status = packet_size;
			
			for (uint8_t i = 0; i<packet_size; i++){
				
				uint8_t character = grid_buffer_read_character(&por->tx_buffer);
				por->tx_double_buffer[i] = character;
				
			}
		
			// Let's acknowledge the transaction
			grid_buffer_read_acknowledge(&por->tx_buffer);
			
			// Let's send the packet through USART
			io_write(&por->usart->io, por->tx_double_buffer, por->tx_double_buffer_status);		
			
		}
		
	}
	
}
