/*
 * grid_buf.c
 *
 * Created: 4/12/2019 5:27:13 PM
 * Author : SUKU WC
*/

#include "grid_buf.h"

// PORTS




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


uint16_t grid_buffer_write_size(struct grid_buffer* buf){
	
	
	
	uint16_t space = 0;
	
	if (buf->read_start > buf->write_start){
		space = buf->read_start - buf->write_start;
	}
	else{
		space = buf->buffer_length - buf->write_start + buf->read_start;
	}

	return space;

	
	
}


uint16_t grid_buffer_write_init(struct grid_buffer* buf, uint16_t length){
	
	
	
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

uint8_t grid_buffer_write_character(struct grid_buffer* buf, uint8_t character){
	

		
	buf->buffer_storage[buf->write_active] = character;
		
	buf->write_active++;
	buf->write_active %= buf->buffer_length;
		
	return 1;
		

}

uint8_t grid_buffer_write_acknowledge(struct grid_buffer* buf){
	
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

uint8_t grid_buffer_write_cancel(struct grid_buffer* buf){
	
	buf->write_active = buf->write_start;
	buf->write_stop   = buf->write_start;
	
	return 1;
}

uint16_t grid_buffer_read_size(struct grid_buffer* buf){
	
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

uint16_t grid_buffer_read_init(struct grid_buffer* buf){
	
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

uint8_t grid_buffer_read_character(struct grid_buffer* buf){
	
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
uint8_t grid_buffer_read_acknowledge(struct grid_buffer* buf){
	
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
uint8_t grid_buffer_read_nacknowledge(struct grid_buffer* buf){
	
	buf->read_active = buf->read_start;
	
	return 1;
}

// DISCARD PACKET
uint8_t grid_buffer_read_cancel(struct grid_buffer* buf){
	
	buf->read_active = buf->read_stop;
	buf->read_start  = buf->read_stop;
	
	return 1;
}

void grid_port_init(volatile struct grid_port* por, uint16_t tx_buf_size, uint16_t rx_buf_size, struct usart_async_descriptor*  usart, uint8_t type, uint8_t dir, uint8_t dma, struct grid_ui_report* p_report){
	
	grid_buffer_init(&por->tx_buffer, tx_buf_size);
	grid_buffer_init(&por->rx_buffer, rx_buf_size);
	
	por->ping_report = p_report;
	
	por->cooldown = 0;
	
	por->dma_channel = dma;
	
	por->direction = dir;
	
	por->usart	= usart;
	por->type		= type;
	
	por->tx_double_buffer_status	= 0;
	por->rx_double_buffer_status	= 0;
	
	
	for (uint32_t i=0; i<GRID_DOUBLE_BUFFER_TX_SIZE; i++){
		por->tx_double_buffer[i] = 0;		
	}
	for (uint32_t i=0; i<GRID_DOUBLE_BUFFER_RX_SIZE; i++){
		por->rx_double_buffer[i] = 0;
	}
	
	por->partner_fi = 0;
	
	por->partner_hwcfg = 0;
	por->partner_status = 1;
	
	
	
	if (type == GRID_PORT_TYPE_USART){	
		
		por->partner_status = 0;
		por->partner_fi = 0;
		
		
		if (por->direction == GRID_CONST_NORTH){
			por->dx = 0;
			por->dy = 1;
		}
		else if (por->direction == GRID_CONST_EAST){
			por->dx = 1;
			por->dy = 0;
		}
		else if (por->direction == GRID_CONST_SOUTH){
			por->dx = 0;
			por->dy = -1;
		}
		else if (por->direction == GRID_CONST_WEST){
			por->dx = -1;
			por->dy = 0;
		}
		
	}
	else{
		por->partner_status = 1; //UI AND USB are considered to be connected by default
	}
	
}

void grid_port_init_all(void){
	
	struct grid_ui_model* mod = &grid_ui_state;
	
	grid_port_init(&GRID_PORT_N, GRID_BUFFER_TX_SIZE, GRID_BUFFER_RX_SIZE, &USART_NORTH, GRID_PORT_TYPE_USART, GRID_CONST_NORTH ,0, &mod->report_array[GRID_REPORT_INDEX_PING_NORTH]);
	grid_port_init(&GRID_PORT_E, GRID_BUFFER_TX_SIZE, GRID_BUFFER_RX_SIZE, &USART_EAST,  GRID_PORT_TYPE_USART, GRID_CONST_EAST  ,1, &mod->report_array[GRID_REPORT_INDEX_PING_EAST]);
	grid_port_init(&GRID_PORT_S, GRID_BUFFER_TX_SIZE, GRID_BUFFER_RX_SIZE, &USART_SOUTH, GRID_PORT_TYPE_USART, GRID_CONST_SOUTH ,2, &mod->report_array[GRID_REPORT_INDEX_PING_SOUTH]);
	grid_port_init(&GRID_PORT_W, GRID_BUFFER_TX_SIZE, GRID_BUFFER_RX_SIZE, &USART_WEST,  GRID_PORT_TYPE_USART, GRID_CONST_WEST  ,3, &mod->report_array[GRID_REPORT_INDEX_PING_WEST]);
	
	grid_port_init(&GRID_PORT_U, GRID_BUFFER_TX_SIZE, GRID_BUFFER_RX_SIZE, NULL, GRID_PORT_TYPE_UI, 0, -1, NULL);
	grid_port_init(&GRID_PORT_H, GRID_BUFFER_TX_SIZE, GRID_BUFFER_RX_SIZE, NULL, GRID_PORT_TYPE_USB, 0, -1, NULL);	
	
	GRID_PORT_U.partner_status = 1; // UI IS ALWAYS CONNECTED
	GRID_PORT_H.partner_status = 1; // HOST IS ALWAYS CONNECTED (Not really!)
	
	
}


//=============================== PROCESS INBOUND ==============================//


uint8_t grid_port_process_inbound(struct grid_port* por, uint8_t loopback){
	
	uint16_t packet_size = grid_buffer_read_size(&por->rx_buffer);
	
	if (!packet_size){
		
		// NO PACKET IN RX BUFFER
		return 0;
		 
	}else{
		
			
		uint8_t port_count = 6;
		struct grid_port* port_array_default[port_count];
		struct grid_port* port_array[port_count];
		
		
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
			if (port_array[i] != por || loopback){
			
				if (packet_size > grid_buffer_write_size(&port_array[i]->tx_buffer)){
					
					grid_sys_alert_set_alert(&grid_sys_state, 100,100,0,2,200);
					
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
			if (port_array[i] != por || loopback){
				grid_buffer_write_init(&port_array[i]->tx_buffer, packet_size);
			}
		}
		
		// Let's do the transaction
												
		for (uint16_t j=0; j<packet_size; j++)
		{
			
			uint8_t character = grid_buffer_read_character(&por->rx_buffer);
				
			for (uint8_t i=0; i<port_count; i++){
				if (port_array[i] != por || loopback){
					grid_buffer_write_character(&port_array[i]->tx_buffer, character);
					
				}
			}					
		}
			
								
		// Let's acknowledge all of the transactions					
		grid_buffer_read_acknowledge(&por->rx_buffer);
					
		for (uint8_t i=0; i<port_count; i++)
		{
			if (port_array[i] != por || loopback){
				grid_buffer_write_acknowledge(&port_array[i]->tx_buffer);
			}
		}	

		return 1;
	}
		
}



//=============================== PROCESS OUTBOUND ==============================//

uint8_t grid_port_process_outbound_usb(struct grid_port* por){
	
			

	// OLD DEBUG IMPLEMENTATION
	
	
	uint16_t length = grid_buffer_read_size(&por->tx_buffer);
	
	if (!length){		
		// NO PACKET IN RX BUFFER
		return 0;	
	}
	
	


	if (length){
		
		for(uint16_t i=0; i<GRID_DOUBLE_BUFFER_TX_SIZE; i++){
			por->tx_double_buffer[i] = 0;
		}
		
		
		
		uint8_t message[500] = {0};
			
		
		//uint8_t message[length];
		
		// Let's transfer the packet to local memory
		grid_buffer_read_init(&por->tx_buffer);
		
		for (uint8_t i = 0; i<length; i++){
			
			message[i] = grid_buffer_read_character(&por->tx_buffer);
			
			por->tx_double_buffer[i] = message[i];
			
		}
				
		// Let's acknowledge the transactions	(should wait for partner to send ack)
		grid_buffer_read_acknowledge(&por->tx_buffer);
		
// 		cdcdf_acm_write(por->tx_double_buffer, length);
// 
// 		return;


		// GRID-2-HOST TRANSLATOR
		
		uint8_t error=0;
			
		int8_t dx = grid_msg_get_parameter(message, GRID_BRC_DX_offset, GRID_BRC_DX_length, &error) - GRID_SYS_DEFAULT_POSITION;
		int8_t dy = grid_msg_get_parameter(message, GRID_BRC_DY_offset, GRID_BRC_DY_length, &error) - GRID_SYS_DEFAULT_POSITION;	
		
				
		uint8_t current_start		= 0;
		uint8_t current_stop		= 0;
		
		
		uint8_t error_flag = 0;
							
		for (uint16_t i=0; i<length; i++){
			
			if (message[i] == GRID_CONST_STX){
				current_start = i;
			}
			else if (message[i] == GRID_CONST_ETX && current_start!=0){
				current_stop = i;
				uint8_t msg_class = grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_offset], GRID_CLASS_length, &error_flag);
				uint8_t msg_instr = grid_sys_read_hex_string_value(&message[current_start+GRID_INSTR_offset], GRID_INSTR_length, &error_flag);
											
				if (msg_class == GRID_CLASS_MIDIRELATIVE_code && msg_instr == GRID_INSTR_REP_code){
					
										
					uint8_t midi_channel = grid_msg_get_parameter(&message[current_start], GRID_CLASS_MIDIRELATIVE_CABLECOMMAND_offset, GRID_CLASS_MIDIRELATIVE_CABLECOMMAND_length, &error);
					uint8_t midi_command = grid_msg_get_parameter(&message[current_start], GRID_CLASS_MIDIRELATIVE_COMMANDCHANNEL_offset , GRID_CLASS_MIDIRELATIVE_COMMANDCHANNEL_length,  &error);
					uint8_t midi_param1  = grid_msg_get_parameter(&message[current_start], GRID_CLASS_MIDIRELATIVE_PARAM1_offset  , GRID_CLASS_MIDIRELATIVE_PARAM1_length,   &error);
					uint8_t midi_param2  = grid_msg_get_parameter(&message[current_start], GRID_CLASS_MIDIRELATIVE_PARAM2_offset  , GRID_CLASS_MIDIRELATIVE_PARAM2_length,   &error);
											
					midi_channel = ((256-dy*2)%8+grid_sys_state.bank_select*8)%16;
					midi_param1  = (256-32+midi_param1 + 16*dx)%96; // 96-128 reserved
												
					audiodf_midi_write(midi_command>>4, midi_command|midi_channel, midi_param1, midi_param2);	
					
									
				}
				else{
// 					sprintf(&por->tx_double_buffer[output_cursor], "[UNKNOWN] -> Protocol: %d\n", msg_protocol);
// 					
// 					output_cursor += strlen(&por->tx_double_buffer[output_cursor]);		
				}
				
				current_start = 0;
				current_stop = 0;
			}
			
						
		}		
		
		
					
		
		// Let's send the packet through USB
		cdcdf_acm_write(por->tx_double_buffer, length);
				
		
	}
	
	
}

uint8_t grid_port_process_outbound_ui(struct grid_port* por){
	
	
	uint16_t length = grid_buffer_read_size(&por->tx_buffer);
	
	if (!length){
				
		// NO PACKET IN RX BUFFER
		return 0;
	}
	else{
		
		uint8_t message[500] = {0};
		
		// Let's transfer the packet to local memory
		grid_buffer_read_init(&por->tx_buffer);
		
		for (uint8_t i = 0; i<length; i++){
					
			message[i] = grid_buffer_read_character(&por->tx_buffer);
			//usb_tx_double_buffer[i] = character;
					
		}

		grid_buffer_read_acknowledge(&por->tx_buffer);
		
		// GRID-2-UI TRANSLATOR
		
		uint8_t error=0;
		
		int8_t dx = grid_msg_get_parameter(message, GRID_BRC_DX_offset, GRID_BRC_DX_length, &error) - GRID_SYS_DEFAULT_POSITION;
		int8_t dy = grid_msg_get_parameter(message, GRID_BRC_DY_offset, GRID_BRC_DY_length, &error) - GRID_SYS_DEFAULT_POSITION;
			
			
		uint8_t current_start		= 0;
		uint8_t current_stop		= 0;
			
			
		uint8_t error_flag = 0;	
		
		//grid_sys_alert_set_alert(&grid_sys_state, 100,0,0,1,300);
		
		for (uint16_t i=0; i<length; i++){
	
			if (message[i] == GRID_CONST_STX){
				current_start = i;
			}
			else if (message[i] == GRID_CONST_ETX && current_start!=0){
				current_stop = i;
				uint8_t msg_class = grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_offset], GRID_CLASS_length, &error_flag);
				uint8_t msg_instr = grid_sys_read_hex_string_value(&message[current_start+GRID_INSTR_offset], GRID_INSTR_length, &error_flag);
		
				if (msg_class == GRID_CLASS_BANKACTIVE_code){
					
					uint8_t banknumber = grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_BANKACTIVE_BANKNUMBER_offset], GRID_CLASS_BANKACTIVE_BANKNUMBER_length, &error_flag);
									
					if (msg_instr == GRID_INSTR_REP_code){ //SET BANK
									
						if (grid_sys_get_bank(&grid_sys_state) == 255){
							grid_report_sys_set_changed_flag(&grid_ui_state, GRID_REPORT_INDEX_HEARTBEAT);
						}
																		
						grid_sys_set_bank(&grid_sys_state, banknumber);
						grid_report_sys_set_payload_parameter(&grid_ui_state, GRID_REPORT_INDEX_MAPMODE,GRID_CLASS_BANKACTIVE_BANKNUMBER_offset,GRID_CLASS_BANKACTIVE_BANKNUMBER_length, banknumber);
												
						grid_report_sys_clear_changed_flag(&grid_ui_state, GRID_REPORT_INDEX_CFG_REQUEST);
													
					}
					else if (msg_instr == GRID_INSTR_REQ_code){ //GET BANK
						
						if (grid_sys_get_bank(&grid_sys_state) != 255){
									
							grid_report_sys_set_changed_flag(&grid_ui_state, GRID_REPORT_INDEX_MAPMODE);
						}						
						
					}
					
					
				}
				if (msg_class == GRID_CLASS_LEDPHASE_code && msg_instr == GRID_INSTR_REP_code){
						
					if (dx == 0 && dy == 0){
									
						uint8_t led_layer = grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_LEDPHASE_LAYERNUMBER_offset], GRID_CLASS_LEDPHASE_LAYERNUMBER_length, &error_flag);
						uint8_t led_number  = grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_LEDPHASE_LEDNUMBER_offset], GRID_CLASS_LEDPHASE_LEDNUMBER_length, &error_flag);
						uint8_t led_value  = grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_LEDPHASE_PHASE_offset], GRID_CLASS_LEDPHASE_PHASE_length, &error_flag);						
						grid_led_set_phase(&grid_led_state, led_number, led_layer, led_value);		
													
					}
				}
				else{
					//SORRY
				}
		
				current_start = 0;
				current_stop = 0;
			}
	
	
		}
	
		

		
	}
	
	
}

uint8_t grid_port_process_outbound_usart(struct grid_port* por){
	
	if (por->tx_double_buffer_status == 0){ // READY TO SEND MESSAGE, NO TRANSMISSION IS IN PROGRESS
		
		uint32_t packet_size = grid_buffer_read_size(&por->tx_buffer);
		
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
			
			return 1;
		}
		
	}
	
	return 0;
}
