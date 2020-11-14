/*
 * grid_buf.c
 *
 * Created: 4/12/2019 5:27:13 PM
 * Author : SUKU WC
*/

#include "grid_buf.h"

// PORTS


void grid_port_reset_receiver(struct grid_port* por){
	
	usart_async_disable(por->usart);
	
	por->rx_double_buffer_seek_start_index = 0;
	por->rx_double_buffer_read_start_index = 0;
	por->partner_status = 0;
	
	
	por->ping_partner_token = 255;
	por->ping_local_token = 255;
	
	grid_sys_write_hex_string_value(&por->ping_packet[8], 2, por->ping_partner_token);
	grid_sys_write_hex_string_value(&por->ping_packet[6], 2, por->ping_local_token);
	grid_msg_checksum_write(por->ping_packet, por->ping_packet_length, grid_msg_calculate_checksum_of_packet_string(por->ping_packet, por->ping_packet_length));


	
	por->rx_double_buffer_timeout = 0;
	grid_sys_port_reset_dma(por);
	
	for(uint16_t i=0; i<GRID_DOUBLE_BUFFER_RX_SIZE; i++){
		por->rx_double_buffer[i] = 0;
	}
	
	for(uint16_t i=0; i<GRID_DOUBLE_BUFFER_TX_SIZE; i++){
		por->tx_double_buffer[i] = 0;
	}
	
	usart_async_enable(por->usart);
	
}



void grid_port_receive_decode(struct grid_port* por, uint16_t startcommand, uint16_t len){
	

	uint8_t error_flag = 0;
	uint8_t checksum_calculated = 0;
	uint8_t checksum_received = 0;
	
	// Copy data from cyrcular buffer to temporary linear array;
	uint8_t* message;
	
	uint16_t length = len;
	uint8_t buffer[length];

	
	// Store message in temporary buffer (MAXMSGLEN = 250 character)
	for (uint16_t i = 0; i<length; i++){
		buffer[i] = por->rx_double_buffer[(por->rx_double_buffer_read_start_index + i)%GRID_DOUBLE_BUFFER_RX_SIZE];
		por->rx_double_buffer[(por->rx_double_buffer_read_start_index + i)%GRID_DOUBLE_BUFFER_RX_SIZE]=0;
	}
	
	message = &buffer[0];
	
	// Clear data from rx double buffer
	for (uint16_t i = 0; i<length; i++){
		por->rx_double_buffer[(por->rx_double_buffer_read_start_index + i)%GRID_DOUBLE_BUFFER_RX_SIZE] = 0;
	}
	
	uint32_t readstartindex = por->rx_double_buffer_read_start_index;
	
	por->rx_double_buffer_read_start_index = (por->rx_double_buffer_read_start_index + length)%GRID_DOUBLE_BUFFER_RX_SIZE;
	por->rx_double_buffer_seek_start_index =  por->rx_double_buffer_read_start_index;
	
	por->rx_double_buffer_status = 0;
	
	// Correct the incorrect frame start location
	for (uint16_t i = 1; i<length; i++){
		
		if (buffer[i] == GRID_CONST_SOH){
			
			length -= i;
			message = &buffer[i];
			
			
			GRID_DEBUG_WARNING(GRID_DEBUG_CONTEXT_PORT, "Frame Start Offset");
			
			
		}
		
	}
	
	

	
	
	
	
	// frame validator
	if (message[0] == GRID_CONST_SOH && message[length-1] == GRID_CONST_LF){
		
				
		
		checksum_received = grid_msg_checksum_read(message, length);
		
		checksum_calculated = grid_msg_calculate_checksum_of_packet_string(message, length);
		
		// checksum validator
		if (checksum_calculated == checksum_received && error_flag == 0){
			

			
			if (message[1] == GRID_CONST_BRC){ // Broadcast message
				
				uint8_t error=0;
				
				// Read the received id age values
				uint8_t received_id  = grid_msg_get_parameter(message, GRID_BRC_ID_offset, GRID_BRC_ID_length, &error);
				uint8_t received_age = grid_msg_get_parameter(message, GRID_BRC_AGE_offset, GRID_BRC_AGE_length, &error);
				
				// Read the received X Y values (SIGNED INT)
				int8_t received_dx  = grid_msg_get_parameter(message, GRID_BRC_DX_offset, GRID_BRC_DX_length, &error) - GRID_SYS_DEFAULT_POSITION;
				int8_t received_dy  = grid_msg_get_parameter(message, GRID_BRC_DY_offset, GRID_BRC_DY_length, &error) - GRID_SYS_DEFAULT_POSITION;
				
				uint8_t received_rot = grid_msg_get_parameter(message, GRID_BRC_ROT_offset, GRID_BRC_ROT_length, &error);
				

				// DO THE DX DY AGE calculations
				
				uint8_t updated_id  = received_id;
				
				int8_t rotated_dx = 0;
				int8_t rotated_dy = 0;
				
				uint8_t updated_rot = (received_rot + por->partner_fi)%4;

				// APPLY THE 2D ROTATION MATRIX
				
				if (por->partner_fi == 0){ // 0 deg
					rotated_dx  += received_dx;
					rotated_dy  += received_dy;
				}
				else if(por->partner_fi == 1){ // 90 deg
					rotated_dx  -= received_dy;
					rotated_dy  += received_dx;
				}
				else if(por->partner_fi == 2){ // 180 deg
					rotated_dx  -= received_dx;
					rotated_dy  -= received_dy;
				}
				else if(por->partner_fi == 3){ // 270 deg
					rotated_dx  += received_dy;
					rotated_dy  -= received_dx;
				}
				else{
					// TRAP INVALID MESSAGE
				}
				
				uint8_t updated_dx = rotated_dx + GRID_SYS_DEFAULT_POSITION + por->dx;
				uint8_t updated_dy = rotated_dy + GRID_SYS_DEFAULT_POSITION + por->dy;
				
				
				
				uint8_t updated_age = received_age;
				
				if (received_dx + GRID_SYS_DEFAULT_POSITION == 0 && received_dy + GRID_SYS_DEFAULT_POSITION == 0)
				{
					// EDITOR GENERATED GLOBAL MESSAGE
					
				}
				else if (received_dx + GRID_SYS_DEFAULT_POSITION == 255 && received_dy + GRID_SYS_DEFAULT_POSITION == 255){
					
					// GRID GENERATED GLOBAL MESSAGE
					
				}
				else{
					
					// Update message with the new values
					grid_msg_set_parameter(message, GRID_BRC_ID_offset, GRID_BRC_ID_length, updated_id, &error);
					grid_msg_set_parameter(message, GRID_BRC_DX_offset, GRID_BRC_DX_length, updated_dx, &error);
					grid_msg_set_parameter(message, GRID_BRC_DY_offset, GRID_BRC_DY_length, updated_dy, &error);
					grid_msg_set_parameter(message, GRID_BRC_AGE_offset, GRID_BRC_AGE_length, updated_age, &error);
					grid_msg_set_parameter(message, GRID_BRC_ROT_offset, GRID_BRC_ROT_length, updated_rot, &error);
				}
				
				

				
				uint32_t fingerprint = updated_id*256*256*256 + updated_dx*256*256 + updated_dy*256 + updated_age;
				
				
				if (0 == grid_msg_find_recent(&grid_sys_state, fingerprint)){
					// WE HAVE NOT HEARD THIS MESSAGE BEFORE
					
					// Recalculate and update the checksum
					
					grid_msg_checksum_write(message, length, grid_msg_calculate_checksum_of_packet_string(message, length));
					

					// IF WE CAN STORE THE MESSAGE IN THE RX BUFFER
					if (grid_buffer_write_init(&por->rx_buffer, length)){
						
		
						
						for (uint16_t i=0; i<length; i++){
							
							grid_buffer_write_character(&por->rx_buffer, message[i]);
							
						}
						
						grid_buffer_write_acknowledge(&por->rx_buffer);
						
						//grid_port_process_inbound(por);
						
						grid_msg_push_recent(&grid_sys_state, fingerprint);
						
					}
					
					
					
				}
				else{
					// WE ALREADY HEARD THIS MESSAGE
					
				}
				
				

				
				// 				uint32_t response_length = strlen(response);
				//
				// 				if(grid_buffer_write_init(&por->tx_buffer, response_length)){
				//
				//
				// 					uint8_t checksum = grid_msg_get_checksum(response, response_length);
				// 					grid_msg_set_checksum(response, response_length, checksum);
				//
				// 					for (uint32_t i=0; i<response_length; i++)
				// 					{
				// 						grid_buffer_write_character(&por->tx_buffer, response[i]);
				// 					}
				//
				// 					grid_buffer_write_acknowledge(&por->tx_buffer);
				//
				// 				}
				
			}
			else if (message[1] == GRID_CONST_DCT){ // Direct Message
				
				//process direct message
				
				if (message[2] == GRID_CONST_ACK){

					//grid_sys_alert_set_alert(&grid_sys_state, 30, 30, 30, 0, 250); // LIGHT WHITE PULSE
				}
				else if (message[2] == GRID_CONST_NAK){
					//grid_sys_alert_set_alert(&grid_sys_state, 50, 0, 0, 0, 250); // LIGHT RED PULSE
					// RESEND PREVIOUS
				}
				else if (message[2] == GRID_CONST_CAN){
					// RESEND PREVIOUS
				}
				else if (message[2] == GRID_CONST_BELL){
					
					
					// Handshake logic
				
					
					uint8_t local_token_received = grid_sys_read_hex_string_value(&message[8], 2, error_flag);
					uint8_t partner_token_received = grid_sys_read_hex_string_value(&message[6], 2, error_flag);
							
					if (por->partner_status == 0){
												
						if (por->ping_local_token == 255){ // I have no clue
							
							// Generate new local
							
							por->ping_local_token  = grid_sys_rtc_get_time(&grid_sys_state)%128;
							
							//NEW
							grid_sys_write_hex_string_value(&por->ping_packet[6], 2, por->ping_local_token);
							grid_msg_checksum_write(por->ping_packet, por->ping_packet_length, grid_msg_calculate_checksum_of_packet_string(por->ping_packet, por->ping_packet_length));
								
							// No chance to connect now
			
						}
						else if (partner_token_received == 255){
							
							// Remote is clueless
							// No chance to connect now
							
						}
						if (partner_token_received != por->ping_partner_token){
							
							por->ping_partner_token = partner_token_received;							
							
							//NEW
							grid_sys_write_hex_string_value(&por->ping_packet[8], 2, partner_token_received);
							grid_msg_checksum_write(por->ping_packet, por->ping_packet_length, grid_msg_calculate_checksum_of_packet_string(por->ping_packet, por->ping_packet_length));
								

							
							// Store remote
							// No chance to connect now
						}
						if (por->ping_local_token != local_token_received){
							
							// Remote is clueless
							// No chance to connect now
							
						}
						else{
							
							// CONNECT
							por->partner_fi = (message[3] - por->direction + 6)%4;
							por->partner_hwcfg = grid_sys_read_hex_string_value(&message[length-10], 2, error_flag);
							por->partner_status = 1;
							
							grid_sys_state.age = grid_sys_rtc_get_time(&grid_sys_state);
							
							
							GRID_DEBUG_WARNING(GRID_DEBUG_CONTEXT_PORT, "Connect");
							
							grid_sys_alert_set_alert(&grid_sys_state, 0, 255, 0, 0, 500); // GREEN
							

							
						}
				
						
						// PUT DIRECT MESSAGE INTO TXBUFFER
						por->ping_flag = 1;
						
											
					
					}
					else{
						
						// VALIDATE CONNECTION
						uint8_t validator = 1;
						
						validator &= local_token_received == por->ping_local_token;
						validator &= partner_token_received == por->ping_partner_token;
						
						validator &= por->partner_fi == (message[3] - por->direction + 6)%4;
						validator &= por->partner_hwcfg == grid_sys_read_hex_string_value(&message[length-10], 2, error_flag);
						
						
						if (validator == 1){
							
							// OK nice job!
							
							//printf("LS: %d RS: %d LR: %d RR: %d  (Validate)\r\n",local_stored,remote_stored,local_received,remote_received);
							
							//OK
							//grid_sys_alert_set_alert(&grid_sys_state, 6, 6, 6, 0, 200); // LIGHT WHITE
							
						}
						else{
							
							//FAILED, DISCONNECT
							por->partner_status = 0;
							
							por->ping_partner_token = 255;
							por->ping_local_token = 255;
							
							grid_sys_write_hex_string_value(&por->ping_packet[8], 2, por->ping_partner_token);
							grid_sys_write_hex_string_value(&por->ping_packet[6], 2, por->ping_local_token);
							grid_msg_checksum_write(por->ping_packet, por->ping_packet_length, grid_msg_calculate_checksum_of_packet_string(por->ping_packet, por->ping_packet_length));
							
							
							//printf("LS: %d RS: %d LR: %d RR: %d  (Invalid)\r\n",local_stored,remote_stored,local_received,remote_received);
							
							//grid_sys_alert_set_alert(&grid_sys_state, 255, 0, 255, 2, 200); // Purple
							
							
						}
					}
					
					
				}
				
			}
			else{ // Unknown Message Type
				
				//grid_sys_alert_set_alert(&grid_sys_state, 0, 255, 0, 2, 200); // RED SHORT now GREEN
				printf("{\"type\": \"WARNING\", \"data\": [\"Unknow Message Type\"]}\r\n");
				
			}
			

			
		}
		else{
			// INVALID CHECKSUM
			
			printf("{\"type\": \"WARNING\", \"data\": [\"Invalid Checksum\"]}\r\n");
			
			if (error_flag != 0){
				//usart_async_disable(&USART_EAST);
				//grid_sys_alert_set_alert(&grid_sys_state, 20, 0, 0, 1, 200); // SOFT RED
				//usart_async_enable(&USART_EAST);
			}
			else{
				
				//grid_sys_alert_set_alert(&grid_sys_state, 20, 0, 255, 1, 200); // BLUE BLINKY
				
				
			}
			
			
		}
		

	}
	else{
		// frame error
		
	}
	
	return;
	
}

void grid_port_receive_task(struct grid_port* por){
	
	//parity error
	
	if (por->usart_error_flag == 1){
		
		por->usart_error_flag = 0;
		
		grid_port_reset_receiver(por);
		
		grid_sys_alert_set_alert(&grid_sys_state, 255, 255, 255, 0, 500); // White triangle

		
	}
	

	///////////////////// PART 1 Old receive task

	if	(por->rx_double_buffer_status == 0){
		
		if (por->usart!=NULL){ // His is GRID usart port

			if (por->rx_double_buffer_timeout > 1000){
			
				if (por->partner_status == 1){
				
				
					GRID_DEBUG_WARNING(GRID_DEBUG_CONTEXT_PORT, "Timeout Disconnect & Reset Receiver");
				
					grid_port_reset_receiver(por);
				
					grid_sys_alert_set_alert(&grid_sys_state, 255, 255, 255, 0, 500);
				}
				else{
				
					if (por->rx_double_buffer_read_start_index == 0 && por->rx_double_buffer_seek_start_index == 0){
						// Ready to receive
					}
					else{
					
						GRID_DEBUG_WARNING(GRID_DEBUG_CONTEXT_PORT, "Timeout & Reset Receiver");
						grid_port_reset_receiver(por);
					}
				
				}
			
			}
			else{
			
				por->rx_double_buffer_timeout++;
			}			
					
		}
		
		for(uint16_t i = 0; i<490; i++){
				
			if (por->rx_double_buffer[por->rx_double_buffer_seek_start_index] == 10){ // \n
					
				por->rx_double_buffer_status = 1;
				por->rx_double_buffer_timeout = 0;
					
				break;
			}
			else if (por->rx_double_buffer[por->rx_double_buffer_seek_start_index] == 0){
				
				break;
			}
				
				
			// Buffer overrun error 1, 2, 3
			if (por->rx_double_buffer_seek_start_index == por->rx_double_buffer_read_start_index-1)
			{
						
				grid_port_reset_receiver(por);	
				grid_sys_alert_set_alert(&grid_sys_state, 255, 0, 0, 2, 200); // RED
				return;
			}
			// Buffer overrun error 1, 2, 3
			if (por->rx_double_buffer_seek_start_index == GRID_DOUBLE_BUFFER_RX_SIZE-1 && por->rx_double_buffer_read_start_index == 0)
			{
				
				grid_port_reset_receiver(por);
				grid_sys_alert_set_alert(&grid_sys_state, 0, 255, 0, 2, 200); // RED
				return;
			}
			// Buffer overrun error 1, 2, 3
			if (por->rx_double_buffer[(por->rx_double_buffer_read_start_index + GRID_DOUBLE_BUFFER_RX_SIZE -1)%GRID_DOUBLE_BUFFER_RX_SIZE] !=0)
			{
				
				grid_port_reset_receiver(por);
				grid_sys_alert_set_alert(&grid_sys_state, 0, 0, 255, 2, 200); // RED
				return;
			}
										
				
			if (por->rx_double_buffer_seek_start_index < GRID_DOUBLE_BUFFER_RX_SIZE-1){
					
				por->rx_double_buffer_timeout = 0;
				por->rx_double_buffer_seek_start_index++;
			}
			else{
					
				por->rx_double_buffer_timeout = 0;
				por->rx_double_buffer_seek_start_index=0;
			}
				
		}
	}
	
	////////////////// PART 2
	
	// No complete message in buffer
	if (por->rx_double_buffer_status == 0){
		return;
	}


	uint32_t length = 0;
	
	if (por->rx_double_buffer_read_start_index < por->rx_double_buffer_seek_start_index){
		length = por->rx_double_buffer_seek_start_index - por->rx_double_buffer_read_start_index + 1;
	}
	else{
		length = GRID_DOUBLE_BUFFER_RX_SIZE + por->rx_double_buffer_seek_start_index - por->rx_double_buffer_read_start_index + 1;
	}
	
	
	grid_port_receive_decode(por, por->rx_double_buffer_read_start_index, length);
	

	
	por->rx_double_buffer_status = 0;
	
	
	
}






uint8_t grid_buffer_init(struct grid_buffer* buf, uint16_t length){
	
	grid_buffer_error_count = 0;
	
	buf->buffer_length = length;
	
	buf->read_length   = 0;
	
	buf->read_start    = 0;
	buf->read_stop     = 0;
	buf->read_active   = 0;
	
	buf->write_start    = 0;
	buf->write_stop     = 0;
	buf->write_active   = 0;
	

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

uint16_t grid_buffer_get_space(struct grid_buffer* buf){
	
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
	
	
	
	uint16_t space = grid_buffer_get_space(buf);

	
	if (space>length){
		
		buf->write_stop = (buf->write_start+length)%buf->buffer_length;
		
		return length;
	}
	else{
		
		grid_buffer_error_count++;
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
		
		grid_buffer_error_count++;
		return 0;
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

void grid_port_init(volatile struct grid_port* por, struct usart_async_descriptor*  usart, uint8_t type, uint8_t dir, uint8_t dma){
	
	grid_buffer_init(&por->tx_buffer, GRID_BUFFER_SIZE);
	grid_buffer_init(&por->rx_buffer, GRID_BUFFER_SIZE);
	
	
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
	
	por->ping_local_token = 255;
	por->ping_partner_token = 255;
	
	por->ping_flag = 0;
	
	if (type == GRID_PORT_TYPE_USART){	
		
		por->partner_status = 0;
		por->partner_fi = 0;
		
		
		sprintf(por->ping_packet, "%c%c%c%c%02x%02x%02x%c00\n", GRID_CONST_SOH, GRID_CONST_DCT, GRID_CONST_BELL, por->direction, grid_sys_get_hwcfg(), 255, 255, GRID_CONST_EOT);
		
		por->ping_packet_length = strlen(por->ping_packet);	
			
		grid_msg_checksum_write(por->ping_packet, por->ping_packet_length, grid_msg_calculate_checksum_of_packet_string(por->ping_packet, por->ping_packet_length));
		

		
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
	
	grid_port_init(&GRID_PORT_N, &USART_NORTH, GRID_PORT_TYPE_USART, GRID_CONST_NORTH ,0);
	grid_port_init(&GRID_PORT_E, &USART_EAST,  GRID_PORT_TYPE_USART, GRID_CONST_EAST  ,1);
	grid_port_init(&GRID_PORT_S, &USART_SOUTH, GRID_PORT_TYPE_USART, GRID_CONST_SOUTH ,2);
	grid_port_init(&GRID_PORT_W, &USART_WEST,  GRID_PORT_TYPE_USART, GRID_CONST_WEST  ,3);
	
	grid_port_init(&GRID_PORT_U, NULL, GRID_PORT_TYPE_UI, 0, -1);
	grid_port_init(&GRID_PORT_H, NULL, GRID_PORT_TYPE_USB, 0, -1);	
	
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
	
	
	// Clear the tx double buffer	
	for(uint16_t i=0; i<GRID_DOUBLE_BUFFER_TX_SIZE; i++){
		por->tx_double_buffer[i] = 0;
	}
		
	struct grid_msg message;
	grid_msg_init(&message);
		

	// Let's transfer the packet to local memory
	grid_buffer_read_init(&por->tx_buffer);
		
	for (uint16_t i = 0; i<length; i++){
			
		uint8_t nextchar = grid_buffer_read_character(&por->tx_buffer);
		
		grid_msg_packet_receive_char(&message, nextchar);
		por->tx_double_buffer[i] = nextchar;	
			
	}
				
	// Let's acknowledge the transactions	(should wait for partner to send ack)
	grid_buffer_read_acknowledge(&por->tx_buffer);
		

	// GRID-2-HOST TRANSLATOR
		
	uint8_t error=0;
			
	int8_t dx = grid_msg_header_get_dx(&message) - GRID_SYS_DEFAULT_POSITION;
	int8_t dy = grid_msg_header_get_dy(&message) - GRID_SYS_DEFAULT_POSITION;	
		
				
	uint8_t current_start		= 0;
	uint8_t current_stop		= 0;
		
		
	uint8_t error_flag = 0;
							
	for (uint16_t i=0; i<message.body_length; i++){
			
		if (message.body[i] == GRID_CONST_STX){
			current_start = i;
		}
		else if (message.body[i] == GRID_CONST_ETX && current_start!=0){
			current_stop = i;
			uint8_t msg_class = grid_msg_text_get_parameter(&message, current_start, GRID_CLASS_offset, GRID_CLASS_length);
			uint8_t msg_instr = grid_msg_text_get_parameter(&message, current_start, GRID_INSTR_offset, GRID_INSTR_length);
											
			if (msg_class == GRID_CLASS_MIDIRELATIVE_code && msg_instr == GRID_INSTR_EXECUTE_code){
					
										
				uint8_t midi_cablecommand = grid_msg_text_get_parameter(&message, current_start, GRID_CLASS_MIDIRELATIVE_CABLECOMMAND_offset,		GRID_CLASS_MIDIRELATIVE_CABLECOMMAND_length);
				uint8_t midi_commandchannel = grid_msg_text_get_parameter(&message, current_start, GRID_CLASS_MIDIRELATIVE_COMMANDCHANNEL_offset ,		GRID_CLASS_MIDIRELATIVE_COMMANDCHANNEL_length);
				uint8_t midi_param1  = grid_msg_text_get_parameter(&message, current_start, GRID_CLASS_MIDIRELATIVE_PARAM1_offset  ,			GRID_CLASS_MIDIRELATIVE_PARAM1_length);
				uint8_t midi_param2  = grid_msg_text_get_parameter(&message, current_start, GRID_CLASS_MIDIRELATIVE_PARAM2_offset  ,			GRID_CLASS_MIDIRELATIVE_PARAM2_length);
						
				// Relative midi translation magic
			//	midi_channel = ((256-dy*2)%8+grid_sys_state.bank_active*8)%16;		  2bank			
						
						
				uint8_t midi_command = 	(midi_commandchannel&0xF0)>>4;
				uint8_t midi_channel = ((256-dy*1)%4+grid_sys_state.bank_activebank_number*4)%16;
					
					
				midi_param1  = (256-32+midi_param1 + 16*dx)%96; // 96-128 reserved
												
// 				uint8_t debug[30] = {0};
// 				sprintf(debug, "MIDI: %02x %02x %02x %02x", 0<<4|midi_command, midi_command<<4|midi_channel, midi_param1, midi_param2);
// 				grid_debug_print_text(debug);	
				
				struct grid_keyboard_key_desc key;
				
				key.ismodifier = 0;
				
				key.keycode = midi_param1 +  0x20;
				
				if (midi_param2){
					key.ispressed = 1;
				}
				else{
					key.ispressed = 0;
				}
				
				//grid_keyboard_keychange(&grid_keyboard_state, &key);
				
				struct grid_midi_event_desc midievent;
								
				midievent.byte0 = 0<<4|midi_command;
				midievent.byte1 = midi_command<<4|midi_channel;
				midievent.byte2 = midi_param1;
				midievent.byte3 = midi_param2;
				
				grid_midi_tx_push(midievent);
				grid_midi_tx_pop(midievent);				
// 				while(audiodf_midi_write_status() == USB_BUSY){
// 					delay_us(20);
// 					grid_sys_alert_set_alert(&grid_sys_state, 255, 255, 0, 2, 250);
// 				}
// 				
// 				audiodf_midi_write(0<<4|midi_command, midi_command<<4|midi_channel, midi_param1, midi_param2);	
				
													
			}
			else if (msg_class == GRID_CLASS_MIDIABSOLUTE_code && msg_instr == GRID_INSTR_EXECUTE_code){
					
					
				uint8_t midi_cablecommand =		grid_msg_text_get_parameter(&message, current_start, GRID_CLASS_MIDIABSOLUTE_CABLECOMMAND_offset,		GRID_CLASS_MIDIABSOLUTE_CABLECOMMAND_length);
				uint8_t midi_commandchannel =	grid_msg_text_get_parameter(&message, current_start, GRID_CLASS_MIDIABSOLUTE_COMMANDCHANNEL_offset,		GRID_CLASS_MIDIABSOLUTE_COMMANDCHANNEL_length);
				uint8_t midi_param1  =			grid_msg_text_get_parameter(&message, current_start, GRID_CLASS_MIDIABSOLUTE_PARAM1_offset  ,			GRID_CLASS_MIDIABSOLUTE_PARAM1_length);
				uint8_t midi_param2  =			grid_msg_text_get_parameter(&message, current_start, GRID_CLASS_MIDIABSOLUTE_PARAM2_offset  ,			GRID_CLASS_MIDIABSOLUTE_PARAM2_length);
					
				struct grid_midi_event_desc midievent;
				
				midievent.byte0 = midi_cablecommand;
				midievent.byte1 = midi_commandchannel;
				midievent.byte2 = midi_param1;
				midievent.byte3 = midi_param2;
					
				grid_midi_tx_push(midievent);
				grid_midi_tx_pop(midievent);	
					
// 				while(audiodf_midi_write_status() == USB_BUSY){
// 					delay_us(20);
// 					grid_sys_alert_set_alert(&grid_sys_state, 255, 255, 0, 2, 250);
// 				}
// 						
// 				audiodf_midi_write(midi_cablecommand, midi_commandchannel, midi_param1, midi_param2);
					
					
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
		
		
	uint32_t packet_length = grid_msg_packet_get_length(&message);
	
	for (uint32_t i=0; i<packet_length; i++){
		
		por->tx_double_buffer[i] = grid_msg_packet_send_char(&message, i);

	}
			
	// Let's send the packet through USB
	cdcdf_acm_write(por->tx_double_buffer, packet_length);

	
}

uint8_t grid_port_process_outbound_ui(struct grid_port* por){
	
	
	uint16_t length = grid_buffer_read_size(&por->tx_buffer);
	
	if (!length){
				
		// NO PACKET IN RX BUFFER
		return 0;
	}
	else{
		
		uint8_t message[GRID_PARAMETER_PACKET_maxlength] = {0};
		
		// Let's transfer the packet to local memory
		grid_buffer_read_init(&por->tx_buffer);
		
		for (uint16_t i = 0; i<length; i++){
					
			message[i] = grid_buffer_read_character(&por->tx_buffer);
			//usb_tx_double_buffer[i] = character;
					
		}

		grid_buffer_read_acknowledge(&por->tx_buffer);
		
		// GRID-2-UI TRANSLATOR
		
		uint8_t error=0;
		
		uint8_t dx = grid_msg_get_parameter(message, GRID_BRC_DX_offset, GRID_BRC_DX_length, &error);
		uint8_t dy = grid_msg_get_parameter(message, GRID_BRC_DY_offset, GRID_BRC_DY_length, &error);
			
		uint8_t position_is_me = 0;
		uint8_t position_is_global = 0;
		uint8_t position_is_local = 0;
			
		if (dx == GRID_SYS_DEFAULT_POSITION && dy == GRID_SYS_DEFAULT_POSITION){
			position_is_me = 1;
		}
		else if (dx == GRID_SYS_GLOBAL_POSITION && dy==GRID_SYS_GLOBAL_POSITION){
			position_is_global = 1;
		}
		else if (dx == GRID_SYS_LOCAL_POSITION && dy==GRID_SYS_LOCAL_POSITION){
			position_is_local = 1;
		}
		
		
			
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
									
					if (msg_instr == GRID_INSTR_EXECUTE_code){ //SET BANK
									
						if (grid_sys_get_bank_valid(&grid_sys_state) == 0){
							
							
							grid_ui_smart_trigger(&grid_core_state, 0, 0, GRID_UI_EVENT_HEARTBEAT);

						}
							
						
																		
						grid_sys_set_bank(&grid_sys_state, banknumber);
						
													
					}
					else if (msg_instr == GRID_INSTR_FETCH_code){ //GET BANK
						
						if (grid_sys_get_bank_valid(&grid_sys_state) != 0){
							
							grid_ui_smart_trigger(&grid_core_state, 0, 0, GRID_UI_EVENT_CFG_RESPONSE);
							
						}						
						
					}
					
					
				}
				else if (msg_class == GRID_CLASS_BANKENABLED_code && msg_instr == GRID_INSTR_EXECUTE_code && (position_is_global || position_is_me || position_is_local)){
					
					//grid_sys_alert_set_alert(&grid_sys_state, 255, 0, 0, 0, 500); // RED
										
					uint8_t banknumber = grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_BANKENABLED_BANKNUMBER_offset], GRID_CLASS_BANKENABLED_BANKNUMBER_length, &error_flag);
					uint8_t isenabled  = grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_BANKENABLED_ISENABLED_offset], GRID_CLASS_BANKENABLED_ISENABLED_length, &error_flag);
					
					if (isenabled == 1){
						
						
						grid_sys_bank_enable(&grid_sys_state, banknumber);
						
						if (grid_sys_get_bank_num(&grid_sys_state) == banknumber){
							
							if (grid_sys_state.bank_activebank_valid == 1){
														
								grid_sys_set_bank(&grid_sys_state, banknumber);
														
							}
						}
						
						
					}else if (isenabled == 0){	
						
						if (grid_sys_get_bank_num(&grid_sys_state) == banknumber){
			
							if (grid_sys_state.bank_activebank_valid == 1){
								
								grid_sys_set_bank(&grid_sys_state, 255);
								
							}
						}	
						
						grid_sys_bank_disable(&grid_sys_state, banknumber);
					}
					else{
						//Sorry
					}
					
					
				}	
				else if (msg_class == GRID_CLASS_BANKCOLOR_code && msg_instr == GRID_INSTR_EXECUTE_code && (position_is_global || position_is_me || position_is_local)){
					
					uint8_t banknumber = grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_BANKCOLOR_NUM_offset], GRID_CLASS_BANKCOLOR_NUM_length, &error_flag);
					uint8_t red		   = grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_BANKCOLOR_RED_offset], GRID_CLASS_BANKCOLOR_RED_length, &error_flag);
					uint8_t green	   = grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_BANKCOLOR_GRE_offset], GRID_CLASS_BANKCOLOR_GRE_length, &error_flag);
					uint8_t blue	   = grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_BANKCOLOR_BLU_offset], GRID_CLASS_BANKCOLOR_BLU_length, &error_flag);
					
					grid_sys_bank_set_color(&grid_sys_state, banknumber, (red<<16) + (green<<8) + (blue<<0) );
					
					// If the currently active bank was changed, then we must reinitialize the bank so the color can be updated properly!
					if (grid_sys_get_bank_num(&grid_sys_state) == banknumber){
						
						if (grid_sys_state.bank_activebank_valid == 1){
							
							grid_sys_set_bank(&grid_sys_state, banknumber);
							
						}
					}
									
				}
				else if (msg_class == GRID_CLASS_LEDPHASE_code && msg_instr == GRID_INSTR_EXECUTE_code && (position_is_local || position_is_me)){
					
					uint8_t led_num  = grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_LEDPHASE_NUM_offset], GRID_CLASS_LEDPHASE_NUM_length, &error_flag);
					uint8_t led_lay = grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_LEDPHASE_LAY_offset], GRID_CLASS_LEDPHASE_LAY_length, &error_flag);
					uint16_t led_pha  = grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_LEDPHASE_PHA_offset], GRID_CLASS_LEDPHASE_PHA_length, &error_flag);
					
					if (led_pha*2 > 255){
						grid_led_set_phase(&grid_led_state, led_num, led_lay, 255);
					}
					else{
						grid_led_set_phase(&grid_led_state, led_num, led_lay, led_pha*2);
					}
					
					
					
							
				}
				else if (msg_class == GRID_CLASS_LEDCOLOR_code && msg_instr == GRID_INSTR_EXECUTE_code && (position_is_local || position_is_me)){
					
						
					uint8_t led_num = grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_LEDCOLOR_NUM_offset], GRID_CLASS_LEDCOLOR_NUM_length, &error_flag);
					uint8_t led_lay = grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_LEDCOLOR_LAY_offset], GRID_CLASS_LEDCOLOR_LAY_length, &error_flag);
					uint8_t led_red	= grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_LEDCOLOR_RED_offset], GRID_CLASS_LEDCOLOR_RED_length, &error_flag);
					uint8_t led_gre	= grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_LEDCOLOR_GRE_offset], GRID_CLASS_LEDCOLOR_GRE_length, &error_flag);
					uint8_t led_blu	= grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_LEDCOLOR_BLU_offset], GRID_CLASS_LEDCOLOR_BLU_length, &error_flag);
																							
					grid_led_set_color(&grid_led_state, led_num, led_lay, led_red, led_gre, led_blu);
				
				}
				else if(msg_class == GRID_CLASS_SERIALNUMBER_code && msg_instr == GRID_INSTR_FETCH_code && (position_is_me || position_is_global)){
					
					uint32_t uniqueid[4] = {0};
					grid_sys_get_id(uniqueid);					
					// Generate RESPONSE
					struct grid_msg response;
											
					grid_msg_init(&response);
					grid_msg_init_header(&response, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_ROTATION, GRID_SYS_DEFAULT_AGE);

					uint8_t response_payload[50] = {0};
					snprintf(response_payload, 49, GRID_CLASS_SERIALNUMBER_frame);

					grid_msg_body_append_text(&response, response_payload, strlen(response_payload));
						
					grid_msg_text_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);					
											
					grid_msg_text_set_parameter(&response, 0, GRID_CLASS_SERIALNUMBER_WORD0_offset, GRID_CLASS_SERIALNUMBER_WORD0_length, uniqueid[0]);
					grid_msg_text_set_parameter(&response, 0, GRID_CLASS_SERIALNUMBER_WORD1_offset, GRID_CLASS_SERIALNUMBER_WORD1_length, uniqueid[1]);
					grid_msg_text_set_parameter(&response, 0, GRID_CLASS_SERIALNUMBER_WORD2_offset, GRID_CLASS_SERIALNUMBER_WORD2_length, uniqueid[2]);
					grid_msg_text_set_parameter(&response, 0, GRID_CLASS_SERIALNUMBER_WORD3_offset, GRID_CLASS_SERIALNUMBER_WORD3_length, uniqueid[3]);


											
					grid_msg_packet_close(&response);
					grid_msg_packet_send_everywhere(&response);
						
	
				}
				else if(msg_class == GRID_CLASS_UPTIME_code && msg_instr == GRID_INSTR_FETCH_code && (position_is_me || position_is_global)){

					// Generate RESPONSE
					struct grid_msg response;
				
					grid_msg_init(&response);
					grid_msg_init_header(&response, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_ROTATION, GRID_SYS_DEFAULT_AGE);

					uint8_t response_payload[50] = {0};
					snprintf(response_payload, 49, GRID_CLASS_UPTIME_frame);

					grid_msg_body_append_text(&response, response_payload, strlen(response_payload));
				
					grid_msg_text_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);
				
					grid_msg_text_set_parameter(&response, 0, GRID_CLASS_UPTIME_UPTIME_offset, GRID_CLASS_UPTIME_UPTIME_length, grid_sys_state.uptime);
					
					
					uint32_t milliseconds = grid_sys_state.uptime/RTC1MS%1000;
					uint32_t seconds =		grid_sys_state.uptime/RTC1MS/1000%60;
					uint32_t minutes =		grid_sys_state.uptime/RTC1MS/1000/60%60;
					uint32_t hours =		grid_sys_state.uptime/RTC1MS/1000/60/60%60;
					
				
					grid_msg_packet_close(&response);
					grid_msg_packet_send_everywhere(&response);
				
				
				}
				else if(msg_class == GRID_CLASS_RESETCAUSE_code && msg_instr == GRID_INSTR_FETCH_code && (position_is_me || position_is_global)){

					// Generate RESPONSE
					struct grid_msg response;
					
					grid_msg_init(&response);
					grid_msg_init_header(&response, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_ROTATION, GRID_SYS_DEFAULT_AGE);

					uint8_t response_payload[50] = {0};
					snprintf(response_payload, 49, GRID_CLASS_RESETCAUSE_frame);

					grid_msg_body_append_text(&response, response_payload, strlen(response_payload));
					
					grid_msg_text_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);
					
					grid_msg_text_set_parameter(&response, 0, GRID_CLASS_RESETCAUSE_CAUSE_offset, GRID_CLASS_RESETCAUSE_CAUSE_length,grid_sys_state.reset_cause);
			
					grid_msg_packet_close(&response);
					grid_msg_packet_send_everywhere(&response);
					
					
				}
				else if(msg_class == GRID_CLASS_RESET_code && msg_instr == GRID_INSTR_EXECUTE_code && (position_is_me)){

					NVIC_SystemReset();
				
				}				
				else if (msg_class == GRID_CLASS_GLOBALLOAD_code && msg_instr == GRID_INSTR_EXECUTE_code && (position_is_me || position_is_global)){
				
					grid_sys_nvm_load_configuration(&grid_sys_state, &grid_nvm_state);
				}
				else if (msg_class == GRID_CLASS_GLOBALSTORE_code && msg_instr == GRID_INSTR_EXECUTE_code && (position_is_me || position_is_global)){
			
					grid_sys_nvm_store_configuration(&grid_sys_state, &grid_nvm_state);
				}
				else if (msg_class == GRID_CLASS_GLOBALCLEAR_code && msg_instr == GRID_INSTR_EXECUTE_code && (position_is_me || position_is_global)){
				
					grid_sys_nvm_clear_configuration(&grid_ui_state, &grid_nvm_state);
				}
				else if (msg_class == GRID_CLASS_GLOBALRECALL_code && msg_instr == GRID_INSTR_EXECUTE_code && (position_is_me || position_is_global)){
					
					uint8_t banknumber		= grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_GLOBALRECALL_BANKNUMBER_offset], GRID_CLASS_GLOBALRECALL_BANKNUMBER_length	, &error_flag);
					grid_sys_recall_configuration(&grid_sys_state, banknumber);
				}
			
				
				else if (msg_class == GRID_CLASS_LOCALLOAD_code && msg_instr == GRID_INSTR_EXECUTE_code && (position_is_me || position_is_global)){
				
					grid_ui_nvm_load_all_configuration(&grid_ui_state, &grid_nvm_state);						
						
				}
				else if (msg_class == GRID_CLASS_LOCALSTORE_code && msg_instr == GRID_INSTR_EXECUTE_code && (position_is_me || position_is_global)){
				
					grid_ui_nvm_store_all_configuration(&grid_ui_state, &grid_nvm_state);
				}

				else if (msg_class == GRID_CLASS_LOCALCLEAR_code && msg_instr == GRID_INSTR_EXECUTE_code && (position_is_me || position_is_global)){
				
					grid_ui_nvm_clear_all_configuration(&grid_ui_state, &grid_nvm_state);
				
				}
				else if (msg_class == GRID_CLASS_CONFIGURATION_code && msg_instr == GRID_INSTR_FETCH_code && (position_is_me || position_is_global)){
					
					uint8_t banknumber		= grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_CONFIGURATION_BANKNUMBER_offset]		, GRID_CLASS_CONFIGURATION_BANKNUMBER_length	, &error_flag);
					uint8_t elementnumber	= grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_CONFIGURATION_ELEMENTNUMBER_offset]	, GRID_CLASS_CONFIGURATION_ELEMENTNUMBER_length	, &error_flag);
					uint8_t eventtype		= grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_CONFIGURATION_EVENTTYPE_offset]		, GRID_CLASS_CONFIGURATION_EVENTTYPE_length		, &error_flag);
					
					
					grid_ui_recall_event_configuration(&grid_ui_state, banknumber, elementnumber, eventtype);
					
				}
				else if (msg_class == GRID_CLASS_CONFIGURATION_code && msg_instr == GRID_INSTR_EXECUTE_code && (position_is_me || position_is_local)){

					uint8_t banknumber		= grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_CONFIGURATION_BANKNUMBER_offset]		, GRID_CLASS_CONFIGURATION_BANKNUMBER_length	, &error_flag);
					uint8_t elementnumber	= grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_CONFIGURATION_ELEMENTNUMBER_offset]	, GRID_CLASS_CONFIGURATION_ELEMENTNUMBER_length	, &error_flag);
					uint8_t eventtype		= grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_CONFIGURATION_EVENTTYPE_offset]		, GRID_CLASS_CONFIGURATION_EVENTTYPE_length		, &error_flag);
					
					uint8_t actionstring[GRID_UI_ACTION_STRING_maxlength]	= {0};
					uint32_t actionstring_length = current_stop-current_start-GRID_CLASS_CONFIGURATION_ACTIONSTRING_offset;
													
				
					
					for(uint32_t j = 0; j<actionstring_length; j++){
					
						actionstring[j] = message[current_start+GRID_CLASS_CONFIGURATION_ACTIONSTRING_offset + j];
						
					}
					
					uint8_t acknowledge = 0;
					
					
					//grid_debug_print_text("Cfg: Received");
					grid_ui_event_register_actionstring(&grid_ui_state.bank_list[banknumber].element_list[elementnumber], eventtype, actionstring, actionstring_length);

					if (banknumber == grid_sys_state.bank_activebank_number){
							
						grid_ui_smart_trigger(&grid_ui_state, banknumber, elementnumber, eventtype);
							
					}

					acknowledge = 1;
							
					uint8_t event_index = grid_ui_event_find(&grid_ui_state.bank_list[banknumber].element_list[elementnumber], eventtype);
					if (event_index != 255){
						if (position_is_local){
							// Clear changed flag because confguration came from nvm
							grid_ui_state.bank_list[banknumber].element_list[elementnumber].event_list[event_index].cfg_flashempty_flag=0;
								
							grid_ui_state.bank_list[banknumber].element_list[elementnumber].event_list[event_index].cfg_changed_flag = 0;
							grid_ui_state.bank_list[banknumber].element_list[elementnumber].event_list[event_index].cfg_default_flag = 0;
						}
						if (position_is_me){
							// Clear changed flag because confguration came from nvm
							grid_ui_state.bank_list[banknumber].element_list[elementnumber].event_list[event_index].cfg_changed_flag = 1;
							grid_ui_state.bank_list[banknumber].element_list[elementnumber].event_list[event_index].cfg_default_flag = 0;
						}
					}
					
					
					
					

					// Generate ACKNOWLEDGE RESPONSE
					struct grid_msg response;
								
					grid_msg_init(&response);
					grid_msg_init_header(&response, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_ROTATION, GRID_SYS_DEFAULT_AGE);

					uint8_t response_payload[10] = {0};
					sprintf(response_payload, GRID_CLASS_CONFIGURATION_frame);
					
					

					grid_msg_body_append_text(&response, response_payload, strlen(response_payload));
					
					grid_msg_text_set_parameter(&response, 0, GRID_CLASS_CONFIGURATION_BANKNUMBER_offset, GRID_CLASS_CONFIGURATION_BANKNUMBER_length, banknumber);
					grid_msg_text_set_parameter(&response, 0, GRID_CLASS_CONFIGURATION_ELEMENTNUMBER_offset, GRID_CLASS_CONFIGURATION_ELEMENTNUMBER_length, elementnumber);
					grid_msg_text_set_parameter(&response, 0, GRID_CLASS_CONFIGURATION_EVENTTYPE_offset, GRID_CLASS_CONFIGURATION_EVENTTYPE_length, eventtype);
					
								
					if (acknowledge == 1){
						grid_msg_text_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);
					}
					else{
						grid_msg_text_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_NACKNOWLEDGE_code);
					}

								
					grid_msg_packet_close(&response);
					grid_msg_packet_send_everywhere(&response);	

					

				}
				else if (msg_class == GRID_CLASS_CONFIGDEFAULT_code && msg_instr == GRID_INSTR_EXECUTE_code && (position_is_me || position_is_local)){

					uint8_t banknumber		= grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_CONFIGDEFAULT_BANKNUMBER_offset]		, GRID_CLASS_CONFIGURATION_BANKNUMBER_length	, &error_flag);
					uint8_t elementnumber	= grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_CONFIGDEFAULT_ELEMENTNUMBER_offset]	, GRID_CLASS_CONFIGURATION_ELEMENTNUMBER_length	, &error_flag);
					uint8_t eventtype		= grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_CONFIGDEFAULT_EVENTTYPE_offset]		, GRID_CLASS_CONFIGURATION_EVENTTYPE_length		, &error_flag);
					

								
					uint8_t acknowledge = 1;
					
						
					//grid_debug_print_text("Cfg: Default");
					grid_ui_event_generate_actionstring(&grid_ui_state.bank_list[banknumber].element_list[elementnumber], eventtype);
						
					if (banknumber == grid_sys_state.bank_activebank_number){
							
						grid_ui_smart_trigger(&grid_ui_state, banknumber, elementnumber, eventtype);
							
					}
						
					uint8_t event_index = grid_ui_event_find(&grid_ui_state.bank_list[banknumber].element_list[elementnumber], eventtype);
					if (event_index != 255){
							
						// Clear changed flag because confguration came from nvm
						grid_ui_state.bank_list[banknumber].element_list[elementnumber].event_list[event_index].cfg_changed_flag = 1;
						grid_ui_state.bank_list[banknumber].element_list[elementnumber].event_list[event_index].cfg_default_flag = 1;
					}

					
					

					// Generate ACKNOWLEDGE RESPONSE
					struct grid_msg response;
					
					grid_msg_init(&response);
					grid_msg_init_header(&response, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_ROTATION, GRID_SYS_DEFAULT_AGE);

					uint8_t response_payload[10] = {0};
					sprintf(response_payload, GRID_CLASS_CONFIGURATION_frame);
					
					

					grid_msg_body_append_text(&response, response_payload, strlen(response_payload));
					
					grid_msg_text_set_parameter(&response, 0, GRID_CLASS_CONFIGURATION_BANKNUMBER_offset, GRID_CLASS_CONFIGURATION_BANKNUMBER_length, banknumber);
					grid_msg_text_set_parameter(&response, 0, GRID_CLASS_CONFIGURATION_ELEMENTNUMBER_offset, GRID_CLASS_CONFIGURATION_ELEMENTNUMBER_length, elementnumber);
					grid_msg_text_set_parameter(&response, 0, GRID_CLASS_CONFIGURATION_EVENTTYPE_offset, GRID_CLASS_CONFIGURATION_EVENTTYPE_length, eventtype);
					
					
					if (acknowledge == 1){
						grid_msg_text_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);
					}
					else{
						grid_msg_text_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_NACKNOWLEDGE_code);
					}

					
					grid_msg_packet_close(&response);
					grid_msg_packet_send_everywhere(&response);

					

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
		
		uint16_t packet_size = grid_buffer_read_size(&por->tx_buffer);
		
		if (!packet_size){
			
			// NO PACKET IN RX BUFFER
			return 0;
		}else{
			
			// Let's transfer the packet to local memory
			grid_buffer_read_init(&por->tx_buffer);
			
			por->tx_double_buffer_status = packet_size;
			
			for (uint16_t i = 0; i<packet_size; i++){
				
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
