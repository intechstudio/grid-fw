/*
 * grid_buf.c
 *
 * Created: 4/12/2019 5:27:13 PM
 * Author : SUKU WC
*/

#include "grid_buf.h"

// PORTS


void grid_port_reset_receiver(struct grid_port* por){

	if (por == &GRID_PORT_E){

		printf("*");
	}

	usart_async_disable(por->usart);
	
	por->partner_status = 0;
	
	
	por->ping_partner_token = 255;
	por->ping_local_token = 255;
	
	grid_sys_write_hex_string_value(&por->ping_packet[8], 2, por->ping_partner_token);
	grid_sys_write_hex_string_value(&por->ping_packet[6], 2, por->ping_local_token);
	grid_msg_checksum_write(por->ping_packet, por->ping_packet_length, grid_msg_calculate_checksum_of_packet_string(por->ping_packet, por->ping_packet_length));


	
	por->rx_double_buffer_timeout = 0;
	grid_sys_port_reset_dma(por);
	


	por->rx_double_buffer_seek_start_index = 0;
	por->rx_double_buffer_read_start_index = 0;

	for(uint16_t i=0; i<GRID_DOUBLE_BUFFER_RX_SIZE; i++){
		por->rx_double_buffer[i] = 0;
	}
	
	for(uint16_t i=0; i<GRID_DOUBLE_BUFFER_TX_SIZE; i++){
		por->tx_double_buffer[i] = 0;
	}
	
	usart_async_enable(por->usart);
	
}

void grid_port_reset_receiver2(struct grid_port* por){


	
	por->partner_status = 0;

	
	por->rx_double_buffer_timeout = 0;
	

	por->rx_double_buffer_seek_start_index = 0;
	por->rx_double_buffer_read_start_index = 0;

	grid_sys_port_reset_dma(por);

	for(uint16_t i=0; i<GRID_DOUBLE_BUFFER_RX_SIZE; i++){
		por->rx_double_buffer[i] = 0;
	}
	
	for(uint16_t i=0; i<GRID_DOUBLE_BUFFER_TX_SIZE; i++){
		por->tx_double_buffer[i] = 0;
	}

	
	
}



void grid_port_receive_decode(struct grid_port* por, uint16_t startcommand, uint16_t len){
	

	uint8_t error_flag = 0;
	uint8_t checksum_calculated = 0;
	uint8_t checksum_received = 0;
	
	// Copy data from cyrcular buffer to temporary linear array;
	uint8_t* message;
	
	uint16_t length = len;
	uint8_t buffer[length+1];

	
	// Store message in temporary buffer (MAXMSGLEN = 250 character)
	for (uint16_t i = 0; i<length; i++){
		buffer[i] = por->rx_double_buffer[(por->rx_double_buffer_read_start_index + i)%GRID_DOUBLE_BUFFER_RX_SIZE];
		por->rx_double_buffer[(por->rx_double_buffer_read_start_index + i)%GRID_DOUBLE_BUFFER_RX_SIZE]=0;
	}
	buffer[length] = 0;
	
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
			
			
			grid_debug_printf("Frame Start Offset");
			
			
		}

		if (buffer[i] == '\n' && i<length-1){

			grid_debug_printf("Frame End Offset");
			length = i;
			break;
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
				uint8_t received_session = grid_msg_get_parameter(message, GRID_BRC_SESSION_offset, GRID_BRC_SESSION_length, &error);
				uint8_t received_msgage = grid_msg_get_parameter(message, GRID_BRC_MSGAGE_offset, GRID_BRC_MSGAGE_length, &error);
				
				// Read the received destination X Y values (SIGNED INT)
				int8_t received_dx  = grid_msg_get_parameter(message, GRID_BRC_DX_offset, GRID_BRC_DX_length, &error) - GRID_SYS_DEFAULT_POSITION;
				int8_t received_dy  = grid_msg_get_parameter(message, GRID_BRC_DY_offset, GRID_BRC_DY_length, &error) - GRID_SYS_DEFAULT_POSITION;
				
				// Read the received source X Y values (SIGNED INT)
				int8_t received_sx  = grid_msg_get_parameter(message, GRID_BRC_SX_offset, GRID_BRC_SX_length, &error) - GRID_SYS_DEFAULT_POSITION;
				int8_t received_sy  = grid_msg_get_parameter(message, GRID_BRC_SY_offset, GRID_BRC_SY_length, &error) - GRID_SYS_DEFAULT_POSITION;
				
				uint8_t received_rot = grid_msg_get_parameter(message, GRID_BRC_ROT_offset, GRID_BRC_ROT_length, &error);
				

				// DO THE DX DY AGE calculations
				
				
				int8_t rotated_dx = 0;
				int8_t rotated_dy = 0;

				int8_t rotated_sx = 0;
				int8_t rotated_sy = 0;
				
				uint8_t updated_rot = (received_rot + por->partner_fi)%4;

				// APPLY THE 2D ROTATION MATRIX
				
				if (por->partner_fi == 0){ // 0 deg
					rotated_dx  += received_dx;
					rotated_dy  += received_dy;

					rotated_sx  += received_sx;
					rotated_sy  += received_sy;
				}
				else if(por->partner_fi == 1){ // 90 deg
					rotated_dx  -= received_dy;
					rotated_dy  += received_dx;

					rotated_sx  -= received_sy;
					rotated_sy  += received_sx;
				}
				else if(por->partner_fi == 2){ // 180 deg
					rotated_dx  -= received_dx;
					rotated_dy  -= received_dy;

					rotated_sx  -= received_sx;
					rotated_sy  -= received_sy;
				}
				else if(por->partner_fi == 3){ // 270 deg
					rotated_dx  += received_dy;
					rotated_dy  -= received_dx;

					rotated_sx  += received_sy;
					rotated_sy  -= received_sx;
				}
				else{
					// TRAP INVALID MESSAGE
				}
				
				uint8_t updated_dx = rotated_dx + GRID_SYS_DEFAULT_POSITION + por->dx;
				uint8_t updated_dy = rotated_dy + GRID_SYS_DEFAULT_POSITION + por->dy;

				uint8_t updated_sx = rotated_sx + GRID_SYS_DEFAULT_POSITION + por->dx;
				uint8_t updated_sy = rotated_sy + GRID_SYS_DEFAULT_POSITION + por->dy;
				
				
				
				uint8_t updated_msgage = received_msgage+1;
				
				if (received_dx + GRID_SYS_DEFAULT_POSITION == 0 && received_dy + GRID_SYS_DEFAULT_POSITION == 0)
				{
					// EDITOR GENERATED GLOBAL MESSAGE
					
				}
				else if (received_dx + GRID_SYS_DEFAULT_POSITION == 255 && received_dy + GRID_SYS_DEFAULT_POSITION == 255){
					
					// GRID GENERATED GLOBAL MESSAGE
					
				}
				else{
					
					// Update message with the new values
					grid_msg_set_parameter(message, GRID_BRC_DX_offset, GRID_BRC_DX_length, updated_dx, &error);
					grid_msg_set_parameter(message, GRID_BRC_DY_offset, GRID_BRC_DY_length, updated_dy, &error);

				}
				
								
				if (received_sx + GRID_SYS_DEFAULT_POSITION == 0 && received_sy + GRID_SYS_DEFAULT_POSITION == 0)
				{
					// EDITOR GENERATED GLOBAL MESSAGE
					
				}
				else if (received_sx + GRID_SYS_DEFAULT_POSITION == 255 && received_sy + GRID_SYS_DEFAULT_POSITION == 255){
					
					// GRID GENERATED GLOBAL MESSAGE
					
				}
				else{
					
					// Update message with the new values
					grid_msg_set_parameter(message, GRID_BRC_SX_offset, GRID_BRC_SX_length, updated_sx, &error);
					grid_msg_set_parameter(message, GRID_BRC_SY_offset, GRID_BRC_SY_length, updated_sy, &error);
				}
				
				grid_msg_set_parameter(message, GRID_BRC_MSGAGE_offset, GRID_BRC_MSGAGE_length, updated_msgage, &error);
				grid_msg_set_parameter(message, GRID_BRC_ROT_offset, GRID_BRC_ROT_length, updated_rot, &error);
				grid_msg_set_parameter(message, GRID_BRC_PORTROT_offset, GRID_BRC_PORTROT_length, por->partner_fi, &error);

				
				uint32_t fingerprint = received_id*256*256*256 + updated_sx*256*256 + updated_sy*256 + received_session;
				
				
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

				}
				else if (message[2] == GRID_CONST_NAK){
					// RESEND PREVIOUS
				}
				else if (message[2] == GRID_CONST_CAN){
					// RESEND PREVIOUS
				}
				else if (message[2] == GRID_CONST_BELL){

					if (por->partner_status == 0){

						// CONNECT
						por->partner_fi = (message[3] - por->direction + 6)%4;
						por->partner_hwcfg = grid_sys_read_hex_string_value(&message[length-10], 2, error_flag);
						por->partner_status = 1;
						
						por->rx_double_buffer_timeout = 0;
						
						grid_debug_printf("Connect");			
						grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_GREEN, 50);	
						grid_led_set_alert_frequency(&grid_led_state, -2);	
						grid_led_set_alert_phase(&grid_led_state, 100);	


					}
					else{

						por->rx_double_buffer_timeout = 0;
					}
				}
				else if (message[2] == GRID_CONST_BELL + 1){ // OLD IMPLEMENTATION
					
					
					// Handshake logic
					
					uint8_t local_token_received = grid_sys_read_hex_string_value(&message[8], 2, error_flag);
					uint8_t partner_token_received = grid_sys_read_hex_string_value(&message[6], 2, error_flag);
							
					if (por->partner_status == 0){
												
						if (por->ping_local_token == 255){ // I have no clue
							
							printf("BELL 0\r\n");
							// Generate new local
							
							por->ping_local_token  = grid_sys_rtc_get_time(&grid_sys_state)%128;
							
							//NEW
							grid_sys_write_hex_string_value(&por->ping_packet[6], 2, por->ping_local_token);
							grid_msg_checksum_write(por->ping_packet, por->ping_packet_length, grid_msg_calculate_checksum_of_packet_string(por->ping_packet, por->ping_packet_length));
								
							// No chance to connect now
			
						}
						else if (partner_token_received == 255){
							

							printf("BELL 1\r\n");
							// Remote is clueless
							// No chance to connect now
							
						}
						if (partner_token_received != por->ping_partner_token){
							
							por->ping_partner_token = partner_token_received;							
							
							//NEW
							grid_sys_write_hex_string_value(&por->ping_packet[8], 2, partner_token_received);
							grid_msg_checksum_write(por->ping_packet, por->ping_packet_length, grid_msg_calculate_checksum_of_packet_string(por->ping_packet, por->ping_packet_length));
							
							printf("BELL 2\r\n");	

							
							// Store remote
							// No chance to connect now
						}
						if (por->ping_local_token != local_token_received){
							
							printf("BELL 3\r\n");
							// Remote is clueless
							// No chance to connect now
							
						}
						else{
							
							// CONNECT
							por->partner_fi = (message[3] - por->direction + 6)%4;
							por->partner_hwcfg = grid_sys_read_hex_string_value(&message[length-10], 2, error_flag);
							por->partner_status = 1;
							
							
							
							grid_debug_printf("Connect");			
							grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_GREEN, 50);	
							grid_led_set_alert_frequency(&grid_led_state, -2);	
							grid_led_set_alert_phase(&grid_led_state, 100);	

							
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
		
							
						}
					}
					
					
				}
				
			}
			else{ // Unknown Message Type
				
				grid_debug_printf("Unknown message type\r\n");
				
			}
			

			
		}
		else{
			// INVALID CHECKSUM
			

			if (error_flag != 0){
				//usart_async_disable(&USART_EAST);
				//usart_async_enable(&USART_EAST);
				grid_debug_printf("Invalid Checksum + flag");
			}
			else{
				printf("##  %s", message);
				grid_debug_printf("Invalid Checksum %02x %02x", checksum_calculated, checksum_received);
			}
			
			
		}
		

	}
	else{
		// frame error
		
		grid_debug_printf("Frame Error %d", length);
		printf("FRAME %s\r\n", message);
	}
	
	return;
	
}

void grid_port_receive_task(struct grid_port* por){

	//parity error
	
	if (por->usart_error_flag == 1){
		
		por->usart_error_flag = 0;
		
		grid_port_reset_receiver(por);
		grid_debug_printf("Parity error");

		grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_RED, 50);	
		grid_led_set_alert_frequency(&grid_led_state, -2);	
		grid_led_set_alert_phase(&grid_led_state, 100);	
		
	}
	

	///////////////////// PART 1 Old receive task

	if	(por->rx_double_buffer_status == 0){
		
		if (por->usart!=NULL){ // This is GRID usart port

			if (por->rx_double_buffer_timeout > 1000){
			
				if (por->partner_status == 1){
				
					grid_debug_printf("Timeout Disconnect 1");
				
						grid_port_reset_receiver2(por);	
						//grid_port_reset_receiver2(por);	
								

						grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_RED, 50);	
						grid_led_set_alert_frequency(&grid_led_state, -2);	
						grid_led_set_alert_phase(&grid_led_state, 100);	
				}
				else{
				
					if (por->rx_double_buffer_read_start_index == 0 && por->rx_double_buffer_seek_start_index == 0){
						// Ready to receive
						
						grid_port_reset_receiver2(por);
					}
					else{
					
						grid_debug_printf("Timeout Disconnect 2");
						grid_port_reset_receiver2(por);
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
				
				
			uint8_t overrun_condition_1 = (por->rx_double_buffer_seek_start_index == por->rx_double_buffer_read_start_index-1);
			uint8_t overrun_condition_2 = (por->rx_double_buffer_seek_start_index == GRID_DOUBLE_BUFFER_RX_SIZE-1 && por->rx_double_buffer_read_start_index == 0);
			uint8_t overrun_condition_3 = (por->rx_double_buffer[(por->rx_double_buffer_read_start_index + GRID_DOUBLE_BUFFER_RX_SIZE -1)%GRID_DOUBLE_BUFFER_RX_SIZE] !=0);
			
			// Buffer overrun error 1, 2, 3
			if (overrun_condition_1 || overrun_condition_2 || overrun_condition_3){

				grid_port_reset_receiver(por);	
				
				printf("Overrun\r\n"); // never use grid message to indicate overrun directly				

				grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_RED, 50);	
				grid_led_set_alert_frequency(&grid_led_state, -2);	
				grid_led_set_alert_phase(&grid_led_state, 100);	
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
		
		
		sprintf(por->ping_packet, "%c%c%c%c%02x%02x%02x%c00\n", GRID_CONST_SOH, GRID_CONST_DCT, GRID_CONST_BELL, por->direction, grid_sys_get_hwcfg(&grid_sys_state), 255, 255, GRID_CONST_EOT);
		
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
					
					printf("Buffer full\r\n");
					grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_BLUE, 255);
					
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

	//grid_msg_init_header(&message, 0, 0);
	message.header_length = 0;
	message.body_length = 0;
	message.last_appended_length = 0;
	message.footer_length = 0;
	
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
											
			if (msg_class == GRID_CLASS_MIDI_code && msg_instr == GRID_INSTR_EXECUTE_code){
					
										
				uint8_t midi_channel = grid_msg_text_get_parameter(&message, current_start, GRID_CLASS_MIDI_CHANNEL_offset,		GRID_CLASS_MIDI_CHANNEL_length);
				uint8_t midi_command = grid_msg_text_get_parameter(&message, current_start, GRID_CLASS_MIDI_COMMAND_offset ,	GRID_CLASS_MIDI_COMMAND_length);
				uint8_t midi_param1  = grid_msg_text_get_parameter(&message, current_start, GRID_CLASS_MIDI_PARAM1_offset  ,	GRID_CLASS_MIDI_PARAM1_length);
				uint8_t midi_param2  = grid_msg_text_get_parameter(&message, current_start, GRID_CLASS_MIDI_PARAM2_offset  ,	GRID_CLASS_MIDI_PARAM2_length);
				
				//printf("midi: %d %d %d %d \r\n", midi_channel, midi_command, midi_param1, midi_param2);
				struct grid_midi_event_desc midievent;
								
				midievent.byte0 = 0<<4|midi_command>>4;
				midievent.byte1 = midi_command|midi_channel;
				midievent.byte2 = midi_param1;
				midievent.byte3 = midi_param2;
				
				grid_midi_tx_push(midievent);
				grid_midi_tx_pop(midievent);				
				
													
			}
			else if (msg_class == GRID_CLASS_HIDMOUSEBUTTON_code && msg_instr == GRID_INSTR_EXECUTE_code){
					
										
				uint8_t state = grid_msg_text_get_parameter(&message, current_start, GRID_CLASS_HIDMOUSEBUTTON_STATE_offset, GRID_CLASS_HIDMOUSEBUTTON_STATE_length);
				uint8_t button = grid_msg_text_get_parameter(&message, current_start, GRID_CLASS_HIDMOUSEBUTTON_BUTTON_offset ,	GRID_CLASS_HIDMOUSEBUTTON_BUTTON_length);
			
				//grid_debug_printf("MouseButton: %d %d", state, button);	
				
				hiddf_mouse_button_change(state, button);
													
			}
			else if (msg_class == GRID_CLASS_HIDMOUSEMOVE_code && msg_instr == GRID_INSTR_EXECUTE_code){
					
										
				uint8_t position_raw = grid_msg_text_get_parameter(&message, current_start, GRID_CLASS_HIDMOUSEMOVE_POSITION_offset, GRID_CLASS_HIDMOUSEMOVE_POSITION_length);
				uint8_t axis = grid_msg_text_get_parameter(&message, current_start, GRID_CLASS_HIDMOUSEMOVE_AXIS_offset ,	GRID_CLASS_HIDMOUSEMOVE_AXIS_length);
			
				int8_t position = position_raw - 128;

				//grid_debug_printf("MouseMove: %d %d", position, axis);	
				
				hiddf_mouse_move(position, axis);
													
			}
			else if (msg_class == GRID_CLASS_HIDKEYBOARD_code && msg_instr == GRID_INSTR_EXECUTE_code){
				
				uint8_t length =	grid_msg_text_get_parameter(&message, current_start, GRID_CLASS_HIDKEYBOARD_LENGTH_offset,		GRID_CLASS_HIDKEYBOARD_LENGTH_length);
				
				uint8_t default_delay =	grid_msg_text_get_parameter(&message, current_start, GRID_CLASS_HIDKEYBOARD_DEFAULTDELAY_offset,		GRID_CLASS_HIDKEYBOARD_DEFAULTDELAY_length);
				


				for(uint8_t j=0; j<length; j+=4){
					
					uint8_t key_ismodifier =	grid_msg_text_get_parameter(&message, current_start+j, GRID_CLASS_HIDKEYBOARD_KEYISMODIFIER_offset,	GRID_CLASS_HIDKEYBOARD_KEYISMODIFIER_length);
					uint8_t key_state  =		grid_msg_text_get_parameter(&message, current_start+j, GRID_CLASS_HIDKEYBOARD_KEYSTATE_offset,		GRID_CLASS_HIDKEYBOARD_KEYSTATE_length);
					uint8_t key_code =			grid_msg_text_get_parameter(&message, current_start+j, GRID_CLASS_HIDKEYBOARD_KEYCODE_offset,		GRID_CLASS_HIDKEYBOARD_KEYCODE_length);



					struct grid_keyboard_event_desc key;
					
					if (key_ismodifier == 0 || key_ismodifier == 1){

						key.ismodifier 	= key_ismodifier;
						key.ispressed 	= key_state;
						key.keycode 	= key_code;
						key.delay 		= default_delay;

						if (key_state == 2){ // combined press and release

							key.ispressed 	= 1;
							grid_keyboard_tx_push(key);
							key.ispressed 	= 0;
							grid_keyboard_tx_push(key);

						}
						else{ // single press or release

							grid_keyboard_tx_push(key);

						}

					}
					else if (key_ismodifier == 0xf){
						// Special delay event

						uint16_t delay = grid_msg_text_get_parameter(&message, current_start+j, GRID_CLASS_HIDKEYBOARD_DELAY_offset, GRID_CLASS_HIDKEYBOARD_DELAY_length);

						key.ismodifier 	= key_ismodifier;
						key.ispressed 	= 0;
						key.keycode 	= 0;
						key.delay 		= delay;

						grid_keyboard_tx_push(key);

					}
					else{
						printf("invalid key_ismodifier parameter\r\n");
					}
					
					// key change fifo buffer

				}

			}
			else{
				
				// sprintf(&por->tx_double_buffer[output_cursor], "[UNKNOWN] -> Protocol: %d\n", msg_protocol);
				// output_cursor += strlen(&por->tx_double_buffer[output_cursor]);		
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

		uint8_t id = grid_msg_get_parameter(message, GRID_BRC_ID_offset, GRID_BRC_ID_length, &error);
			
		uint8_t dx = grid_msg_get_parameter(message, GRID_BRC_DX_offset, GRID_BRC_DX_length, &error);
		uint8_t dy = grid_msg_get_parameter(message, GRID_BRC_DY_offset, GRID_BRC_DY_length, &error);

		uint8_t sx = grid_msg_get_parameter(message, GRID_BRC_SX_offset, GRID_BRC_SX_length, &error);
		uint8_t sy = grid_msg_get_parameter(message, GRID_BRC_SY_offset, GRID_BRC_SY_length, &error);

		uint8_t rot = grid_msg_get_parameter(message, GRID_BRC_ROT_offset, GRID_BRC_ROT_length, &error);
		uint8_t portrot = grid_msg_get_parameter(message, GRID_BRC_PORTROT_offset, GRID_BRC_PORTROT_length, &error);
			
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

		uint8_t start_count		= 0;
		uint8_t stop_count		= 0;
			
			
		uint8_t error_flag = 0;	
		
		
		for (uint16_t i=0; i<length; i++){
	
			if (message[i] == GRID_CONST_STX){

				current_start = i;
				start_count++;
			}
			else if (message[i] == GRID_CONST_ETX && current_start!=0 && (start_count-stop_count) == 1){
				current_stop = i;
				stop_count++;
				uint8_t msg_class = grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_offset], GRID_CLASS_length, &error_flag);
				uint8_t msg_instr = grid_sys_read_hex_string_value(&message[current_start+GRID_INSTR_offset], GRID_INSTR_length, &error_flag);
		

		
				if (msg_class == GRID_CLASS_PAGEACTIVE_code){ // dont check address!
						
					uint8_t page = grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_PAGEACTIVE_PAGENUMBER_offset], GRID_CLASS_PAGEACTIVE_PAGENUMBER_length, &error_flag);
								
					
					if (msg_instr == GRID_INSTR_EXECUTE_code){ //SET BANK

						

						if (grid_ui_state.page_change_enabled == 1){

							//grid_debug_printf("TRY");
							grid_ui_page_load(&grid_ui_state, &grid_nvm_state, page);
							grid_sys_set_bank(&grid_sys_state, page);

						}
						else{

							//grid_debug_printf("DISABLE");
						}
													
					}

					if (msg_instr == GRID_INSTR_REPORT_code){ //SET BANK

				
						//printf("RX: %d %d\r\n", sx, sy);

						if (!(sx==GRID_SYS_DEFAULT_POSITION && sy==GRID_SYS_DEFAULT_POSITION)){

							//printf("RX: %s\r\n", &message[current_start]);
							if (grid_ui_state.page_negotiated == 0){

								grid_ui_state.page_negotiated = 1;
								grid_ui_page_load(&grid_ui_state, &grid_nvm_state, page);
								grid_sys_set_bank(&grid_sys_state, page);
								
							}
							



						}
													
					}

					
				}
				if (msg_class == GRID_CLASS_MIDI_code && msg_instr == GRID_INSTR_REPORT_code){
						
									
					uint8_t midi_channel = grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_MIDI_CHANNEL_offset], GRID_CLASS_MIDI_CHANNEL_length, &error_flag);	
					uint8_t midi_command = grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_MIDI_COMMAND_offset], GRID_CLASS_MIDI_COMMAND_length, &error_flag);	
					uint8_t midi_param1 = grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_MIDI_PARAM1_offset], GRID_CLASS_MIDI_PARAM1_length, &error_flag);	
					uint8_t midi_param2 = grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_MIDI_PARAM2_offset], GRID_CLASS_MIDI_PARAM2_length, &error_flag);		
					
					//printf("M: %d %d %d %d \r\n", midi_channel, midi_command, midi_param1, midi_param2);

					uint8_t temp[GRID_PARAMETER_ACTIONSTRING_maxlength + 100] = {0};
					sprintf(temp, "midi={} midi.ch,midi.cmd,midi.p1,midi.p2 = %d, %d, %d, %d ", midi_channel, midi_command, midi_param1, midi_param2);
					//grid_lua_dostring(&grid_lua_state, temp);

					struct grid_ui_element* ele = &grid_ui_state.element_list[grid_ui_state.element_list_length-1];
					struct grid_ui_event* eve = NULL;

					eve = grid_ui_event_find(ele, GRID_UI_EVENT_MIDIRX);
					if (eve != NULL){

						sprintf(&temp[strlen(temp)], " %s", &eve->action_string[6]);
						temp[strlen(temp)-3] = '\0'; 

						grid_lua_dostring(&grid_lua_state, temp);
						//printf("%s \r\n", temp);

					}



														
				}
				if (msg_class == GRID_CLASS_PAGECOUNT_code && (position_is_global || position_is_me)){
				
					if (msg_instr == GRID_INSTR_FETCH_code){ //get page count

						struct grid_msg response;
												
						grid_msg_init_header(&response, GRID_SYS_GLOBAL_POSITION, GRID_SYS_GLOBAL_POSITION);

						uint8_t response_payload[50] = {0};
						snprintf(response_payload, 49, GRID_CLASS_PAGECOUNT_frame);

						grid_msg_body_append_text(&response, response_payload);
							
						grid_msg_text_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);					
												
						grid_msg_text_set_parameter(&response, 0, GRID_CLASS_PAGECOUNT_PAGENUMBER_offset, GRID_CLASS_PAGECOUNT_PAGENUMBER_length, grid_ui_state.page_count);

						grid_msg_packet_close(&response);
						grid_msg_packet_send_everywhere(&response);
							

													
					}
					
				}
				else if (msg_class == GRID_CLASS_IMEDIATE_code && msg_instr == GRID_INSTR_EXECUTE_code && (position_is_global || position_is_me || position_is_local)){

					uint16_t length = grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_IMEDIATE_ACTIONLENGTH_offset], GRID_CLASS_IMEDIATE_ACTIONLENGTH_length, &error_flag);
					uint8_t actionstring[200] = {0};
					strncpy(actionstring, &message[current_start+GRID_CLASS_IMEDIATE_ACTIONSTRING_offset], length);


					if (0 == strncmp(actionstring, "<?lua ", 6) && actionstring[length-3] == ' ' && actionstring[length-2] == '?' && actionstring[length-1] == '>'){
					
					
						printf("IMEDIATE %d: %s\r\n", length, actionstring);
						
						actionstring[length-3] = '\0';
						grid_lua_dostring(&grid_lua_state, &actionstring[6]);

					
					}
					else{
						printf("IMEDIATE NOT OK %d: %s\r\n", length, actionstring);
					}

					



				}
				else if (msg_class == GRID_CLASS_HEARTBEAT_code){
					
					uint8_t type  = grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_HEARTBEAT_TYPE_offset], GRID_CLASS_HEARTBEAT_TYPE_length, &error_flag);
					
					if (type == 0){
						// from other grid module
					}
					else if (type == 1){

						// from usb connected module
						int8_t received_sx = sx-GRID_SYS_DEFAULT_POSITION; // convert to signed ind
						int8_t received_sy = sy-GRID_SYS_DEFAULT_POSITION; // convert to signed ind
						int8_t rotated_sx;
						int8_t rotated_sy;

						// APPLY THE 2D ROTATION MATRIX
						
						//printf("Protrot %d \r\n", portrot);

						if (portrot == 0){ // 0 deg

							rotated_sx  -= received_sx;
							rotated_sy  -= received_sy;
						}
						else if(portrot == 1){ // 90 deg

							rotated_sx  -= received_sy;
							rotated_sy  += received_sx;
						}
						else if(portrot == 2){ // 180 deg

							rotated_sx  += received_sx;
							rotated_sy  += received_sy;
						}
						else if(portrot == 3){ // 270 deg

							rotated_sx  += received_sy;
							rotated_sy  -= received_sx;
						}
						else{
							// TRAP INVALID MESSAGE
						}


						grid_sys_state.module_x = rotated_sx; // convert to signed ind
						grid_sys_state.module_y = rotated_sy; // convert to signed ind
						grid_sys_state.module_rot = rot;


					}
					else if (type >127){ // editor


						if (grid_sys_state.editor_connected == 0){
							grid_sys_state.editor_connected = 1;
							printf("EDITOR connect\r\n");
						}

						grid_sys_state.editor_heartbeat_lastrealtime = grid_sys_rtc_get_time(&grid_sys_state);

						if (type == 255){
							grid_ui_state.page_change_enabled = 1;
						}
						else{
							grid_ui_state.page_change_enabled = 0;
						}

						uint8_t led_report_valid = 0;
						uint8_t ui_report_valid = 0;

						if (1 && 0){

							for(uint8_t j=0; j<grid_led_state.led_number; j++){

								if (grid_led_state.led_lowlevel_changed[j]){

									uint8_t led_num = j;
									uint8_t led_red = grid_led_state.led_lowlevel_red[j];
									uint8_t led_gre = grid_led_state.led_lowlevel_gre[j];
									uint8_t led_blu = grid_led_state.led_lowlevel_blu[j];
									
									printf("Led %d: %d %d %d\r\n", led_num, led_red, led_gre, led_blu);
									
									//grid_led_state.led_lowlevel_changed[j] = 0;
									led_report_valid = 1;
								}

							}
						}

						if (1 && 0){

							uint16_t report_length = 0;
							struct grid_msg response;
													
							grid_msg_init_header(&response, GRID_SYS_GLOBAL_POSITION, GRID_SYS_GLOBAL_POSITION);

							uint8_t response_payload[300] = {0};
							uint16_t len = 0;
							snprintf(&response_payload[len], 299, GRID_CLASS_EVENTPREVIEW_frame_start);
							len += strlen(&response_payload[len]);



							for(uint8_t j=0; j<grid_ui_state.element_list_length; j++){


								struct grid_ui_element* ele = &grid_ui_state.element_list[j];

								uint8_t element_num = ele->index;
								uint8_t element_value = 0;

								if (ele->type == GRID_UI_ELEMENT_POTENTIOMETER){

									element_value = ele->template_parameter_list[GRID_LUA_FNC_P_POTMETER_VALUE_index];
								}
								else if (ele->type == GRID_UI_ELEMENT_ENCODER){

									element_value = ele->template_parameter_list[GRID_LUA_FNC_E_ENCODER_VALUE_index];
								}
								
								report_length += 4;

								printf("Element %d: %d\r\n", element_num, element_value);
								
								//grid_led_state.led_lowlevel_changed[j] = 0;

								ui_report_valid = 1;

								sprintf(&response_payload[len], "%02x%02x", element_num, element_value);
								len += strlen(&response_payload[len]);

							}


							sprintf(&response_payload[len], GRID_CLASS_EVENTPREVIEW_frame_end);
							len += strlen(&response_payload[len]);

							grid_msg_body_append_text(&response, response_payload);
								

							grid_msg_text_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);													
							grid_msg_text_set_parameter(&response, 0, GRID_CLASS_EVENTPREVIEW_LENGTH_offset, GRID_CLASS_EVENTPREVIEW_LENGTH_length, report_length);
							
							grid_msg_packet_close(&response);
							grid_msg_packet_send_everywhere(&response);

							printf(response.body);
							printf("\r\n");
						}

						if (led_report_valid && 0){

							struct grid_msg response;
													
							grid_msg_init_header(&response, GRID_SYS_GLOBAL_POSITION, GRID_SYS_GLOBAL_POSITION);

							uint8_t response_payload[300] = {0};
							uint16_t len = 0;
							snprintf(response_payload, 299, GRID_CLASS_LEDPREVIEW_frame_start);
							len += strlen(&response_payload[len]);

							uint16_t report_length = grid_led_lowlevel_change_report(&grid_led_state, -1, &response_payload[len]);

							len += strlen(&response_payload[len]);

							grid_msg_body_append_text(&response, response_payload);
								
							grid_msg_text_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);													
							grid_msg_text_set_parameter(&response, 0, GRID_CLASS_LEDPREVIEW_LENGTH_offset, GRID_CLASS_LEDPREVIEW_LENGTH_length, report_length);
							
							grid_msg_packet_close(&response);
							grid_msg_packet_send_everywhere(&response);

							printf(response.body);
							printf("\r\n");

						}


						// from editor

					}
					else{
						// unknown type
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
											
					grid_msg_init_header(&response, GRID_SYS_GLOBAL_POSITION, GRID_SYS_GLOBAL_POSITION);

					uint8_t response_payload[50] = {0};
					snprintf(response_payload, 49, GRID_CLASS_SERIALNUMBER_frame);

					grid_msg_body_append_text(&response, response_payload);
						
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
				
					grid_msg_init_header(&response, GRID_SYS_GLOBAL_POSITION, GRID_SYS_GLOBAL_POSITION);

					uint8_t response_payload[50] = {0};
					snprintf(response_payload, 49, GRID_CLASS_UPTIME_frame);

					grid_msg_body_append_text(&response, response_payload);
				
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
					
					grid_msg_init_header(&response, GRID_SYS_GLOBAL_POSITION, GRID_SYS_GLOBAL_POSITION);

					uint8_t response_payload[50] = {0};
					snprintf(response_payload, 49, GRID_CLASS_RESETCAUSE_frame);

					grid_msg_body_append_text(&response, response_payload);
					
					grid_msg_text_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);
					
					grid_msg_text_set_parameter(&response, 0, GRID_CLASS_RESETCAUSE_CAUSE_offset, GRID_CLASS_RESETCAUSE_CAUSE_length,grid_sys_state.reset_cause);
			
					grid_msg_packet_close(&response);
					grid_msg_packet_send_everywhere(&response);
					
					
				}
				else if(msg_class == GRID_CLASS_RESET_code && msg_instr == GRID_INSTR_EXECUTE_code && (position_is_me)){

					NVIC_SystemReset();
				
				}				
			
				else if (msg_class == GRID_CLASS_CONFIGDISCARD_code && msg_instr == GRID_INSTR_EXECUTE_code && (position_is_me || position_is_global)){
			
					grid_sys_state.lastheader_configdiscard.status = -1;
					grid_sys_state.lastheader_configdiscard.id = id;		
					grid_nvm_ui_bulk_read_init(&grid_nvm_state, &grid_ui_state);				

				}		
				else if (msg_class == GRID_CLASS_CONFIGDISCARD_code && msg_instr == GRID_INSTR_CHECK_code && (position_is_me || position_is_global)){
					
					struct grid_msg response;	
					grid_msg_init_header(&response, GRID_SYS_GLOBAL_POSITION, GRID_SYS_GLOBAL_POSITION);
					grid_msg_body_append_printf(&response, GRID_CLASS_CONFIGDISCARD_frame);
					grid_msg_body_append_parameter(&response, GRID_CLASS_CONFIGDISCARD_LASTHEADER_offset, GRID_CLASS_CONFIGDISCARD_LASTHEADER_length, grid_sys_state.lastheader_configdiscard.id);		
				
					if (grid_sys_state.lastheader_configdiscard.status != -1){ // ACK
						grid_msg_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);
					}		
					else{ // NACK
						grid_msg_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_NACKNOWLEDGE_code);
					}	

					grid_msg_packet_close(&response);
					grid_msg_packet_send_everywhere(&response);	

				}			
				else if (msg_class == GRID_CLASS_CONFIGSTORE_code && msg_instr == GRID_INSTR_EXECUTE_code && (position_is_me || position_is_global)){
				
									
					grid_sys_state.lastheader_configstore.status = -1;
					grid_sys_state.lastheader_configstore.id = id;
					grid_nvm_ui_bulk_store_init(&grid_nvm_state, &grid_ui_state);					

				}		
				else if (msg_class == GRID_CLASS_CONFIGSTORE_code && msg_instr == GRID_INSTR_CHECK_code && (position_is_me || position_is_global)){
					
					struct grid_msg response;	
					grid_msg_init_header(&response, GRID_SYS_GLOBAL_POSITION, GRID_SYS_GLOBAL_POSITION);
					grid_msg_body_append_printf(&response, GRID_CLASS_CONFIGSTORE_frame);
					grid_msg_body_append_parameter(&response, GRID_CLASS_CONFIGSTORE_LASTHEADER_offset, GRID_CLASS_CONFIGSTORE_LASTHEADER_length, grid_sys_state.lastheader_configstore.id);		
				
					if (grid_sys_state.lastheader_configstore.status != -1 && 0 == grid_nvm_ui_bulk_store_is_in_progress(&grid_nvm_state, &grid_ui_state)){ // ACK
						grid_msg_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);
					}		
					else{ // NACK
						grid_msg_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_NACKNOWLEDGE_code);
					}	

					grid_msg_packet_close(&response);
					grid_msg_packet_send_everywhere(&response);	

				}	
				else if (msg_class == GRID_CLASS_CONFIGERASE_code && msg_instr == GRID_INSTR_EXECUTE_code && (position_is_me || position_is_global)){
				
					grid_sys_state.lastheader_configerase.status = -1;
					grid_sys_state.lastheader_configerase.id = id;
					grid_nvm_ui_bulk_clear_init(&grid_nvm_state, &grid_ui_state);
					//grid_nvm_erase_all(&grid_nvm_state);

				}		
				else if (msg_class == GRID_CLASS_CONFIGERASE_code && msg_instr == GRID_INSTR_CHECK_code && (position_is_me || position_is_global)){
					
					struct grid_msg response;	
					grid_msg_init_header(&response, GRID_SYS_GLOBAL_POSITION, GRID_SYS_GLOBAL_POSITION);
					grid_msg_body_append_printf(&response, GRID_CLASS_CONFIGERASE_frame);
					grid_msg_body_append_parameter(&response, GRID_CLASS_CONFIGERASE_LASTHEADER_offset, GRID_CLASS_CONFIGERASE_LASTHEADER_length, grid_sys_state.lastheader_configerase.id);		
				
					if (grid_sys_state.lastheader_configerase.status != -1 && 0 == grid_nvm_ui_bulk_clear_is_in_progress(&grid_nvm_state, &grid_ui_state)){ // ACK
						grid_msg_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);
					}		
					else{ // NACK
						grid_msg_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_NACKNOWLEDGE_code);
					}	

					grid_msg_packet_close(&response);
					grid_msg_packet_send_everywhere(&response);	

				}	
				else if (msg_class == GRID_CLASS_CONFIG_code && msg_instr == GRID_INSTR_FETCH_code && (position_is_me || position_is_global)){
					
					
					uint8_t pagenumber = grid_msg_get_parameter(message, current_start+GRID_CLASS_CONFIG_PAGENUMBER_offset, GRID_CLASS_CONFIG_PAGENUMBER_length, NULL);
					uint8_t elementnumber = grid_msg_get_parameter(message, current_start+GRID_CLASS_CONFIG_ELEMENTNUMBER_offset, GRID_CLASS_CONFIG_ELEMENTNUMBER_length, NULL);
					uint8_t eventtype = grid_msg_get_parameter(message, current_start+GRID_CLASS_CONFIG_EVENTTYPE_offset, GRID_CLASS_CONFIG_EVENTTYPE_length, NULL);
					//uint16_t actionlength = grid_msg_get_parameter(message, current_start+GRID_CLASS_CONFIG_ACTIONLENGTH_offset, GRID_CLASS_CONFIG_ACTIONLENGTH_length, NULL);

					// Helper to map system element to 255
					if (elementnumber == 255){
						elementnumber = grid_ui_state.element_list_length - 1;
					}

					grid_ui_recall_event_configuration(&grid_ui_state, &grid_nvm_state, pagenumber, elementnumber, eventtype);
					
				}
				else if (msg_class == GRID_CLASS_CONFIG_code && msg_instr == GRID_INSTR_EXECUTE_code){

                    if (position_is_local || position_is_global || position_is_me){
                        // disable hid action automatically
                        grid_keyboard_state.isenabled = 0;             
                        //grid_debug_print_text("Disabling KB");

						uint8_t vmajor = grid_msg_get_parameter(message, current_start+GRID_CLASS_CONFIG_VERSIONMAJOR_offset, GRID_CLASS_CONFIG_VERSIONMAJOR_length, NULL);
						uint8_t vminor = grid_msg_get_parameter(message, current_start+GRID_CLASS_CONFIG_VERSIONMINOR_offset, GRID_CLASS_CONFIG_VERSIONMINOR_length, NULL);
						uint8_t vpatch = grid_msg_get_parameter(message, current_start+GRID_CLASS_CONFIG_VERSIONPATCH_offset, GRID_CLASS_CONFIG_VERSIONPATCH_length, NULL);
									
						if (vmajor == GRID_PROTOCOL_VERSION_MAJOR && vminor == GRID_PROTOCOL_VERSION_MINOR && vpatch == GRID_PROTOCOL_VERSION_PATCH){
							// version ok	
							printf("version ok\r\n");
						}
						else{
							printf("error.buf.config version mismatch\r\n");
						}

						uint8_t pagenumber = grid_msg_get_parameter(message, current_start+GRID_CLASS_CONFIG_PAGENUMBER_offset, GRID_CLASS_CONFIG_PAGENUMBER_length, NULL);
						uint8_t elementnumber = grid_msg_get_parameter(message, current_start+GRID_CLASS_CONFIG_ELEMENTNUMBER_offset, GRID_CLASS_CONFIG_ELEMENTNUMBER_length, NULL);
						uint8_t eventtype = grid_msg_get_parameter(message, current_start+GRID_CLASS_CONFIG_EVENTTYPE_offset, GRID_CLASS_CONFIG_EVENTTYPE_length, NULL);
						uint16_t actionlength = grid_msg_get_parameter(message, current_start+GRID_CLASS_CONFIG_ACTIONLENGTH_offset, GRID_CLASS_CONFIG_ACTIONLENGTH_length, NULL);


						if (elementnumber == 255){
							
							elementnumber = grid_ui_state.element_list_length - 1;
						}	

						char* action = &message[current_start + GRID_CLASS_CONFIG_ACTIONSTRING_offset];

						uint8_t ack = 0; // nacknowledge by default

						if (action[actionlength] == GRID_CONST_ETX){
							
							if (actionlength < GRID_PARAMETER_ACTIONSTRING_maxlength){

							action[actionlength] = 0;
							//printf("Config: %d %d %d %d -> %s\r\n", pagenumber, elementnumber, eventtype, actionlength, action);
							
								grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_WHITE, 64);
								if (pagenumber == grid_ui_state.page_activepage){

									//find event
									struct grid_ui_event* eve = grid_ui_event_find(&grid_ui_state.element_list[elementnumber], eventtype);
									
									if (eve != NULL){

										//register actionstring
										grid_ui_state.page_change_enabled = 0;
										grid_ui_event_register_actionstring(eve, action);
										//printf("Registered\r\n");
										//acknowledge
										ack = 1;

										//grid_debug_printf("autotrigger: %d", autotrigger);

										grid_ui_event_trigger_local(eve);	
										


									}

								}
								
								action[actionlength] = GRID_CONST_ETX;
							}
							else{
								grid_debug_printf("config too long");
							}

						}
						else{

							printf("Config frame invalid: %d %d %d %d end: %d %s\r\n", pagenumber, elementnumber, eventtype, actionlength, action[actionlength], action);

						}


						// Generate ACKNOWLEDGE RESPONSE
						struct grid_msg response;
						
						grid_msg_init_header(&response, GRID_SYS_GLOBAL_POSITION, GRID_SYS_GLOBAL_POSITION);
						
						grid_msg_body_append_printf(&response, GRID_CLASS_CONFIG_frame_check);
						
						grid_msg_body_append_parameter(&response, GRID_CLASS_CONFIG_LASTHEADER_offset, GRID_CLASS_CONFIG_LASTHEADER_length, id);
						
						if (ack == 1){
							grid_sys_state.lastheader_config.status = 0;
							grid_sys_state.lastheader_config.id = id;
							grid_msg_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);
							grid_debug_printf("Config %d", id);
						}
						else{
							grid_msg_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_NACKNOWLEDGE_code);
							grid_debug_printf("Config Error");
						}
						
						grid_msg_packet_close(&response);
						grid_msg_packet_send_everywhere(&response);

                    }
                }		
				else if (msg_class == GRID_CLASS_CONFIG_code && msg_instr == GRID_INSTR_CHECK_code && (position_is_me || position_is_global)){
					
					struct grid_msg response;	
					grid_msg_init_header(&response, GRID_SYS_GLOBAL_POSITION, GRID_SYS_GLOBAL_POSITION);
					grid_msg_body_append_printf(&response, GRID_CLASS_CONFIG_frame);
					grid_msg_body_append_parameter(&response, GRID_CLASS_CONFIG_LASTHEADER_offset, GRID_CLASS_CONFIG_LASTHEADER_length, grid_sys_state.lastheader_config.id);		
				
					if (grid_sys_state.lastheader_config.status != -1){ // ACK
						grid_msg_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);
					}		
					else{ // NACK
						grid_msg_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_NACKNOWLEDGE_code);
					}	

					grid_msg_packet_close(&response);
					grid_msg_packet_send_everywhere(&response);	

				}	
                else if (msg_class == GRID_CLASS_HIDKEYSTATUS_code && msg_instr == GRID_INSTR_EXECUTE_code && (position_is_me || position_is_global)){
				
                    uint8_t isenabled =	grid_sys_read_hex_string_value(&message[current_start+GRID_CLASS_HIDKEYSTATUS_ISENABLED_offset]		, GRID_CLASS_HIDKEYSTATUS_ISENABLED_length	, &error_flag);
					
                    
                    grid_keyboard_state.isenabled = isenabled;

                    if (isenabled){
                    
                        //grid_debug_print_text("Enabling KB");
                    
                    }
                    else{
                        //grid_debug_print_text("Disabling  KB");
                    }
                    
                    // Generate ACKNOWLEDGE RESPONSE
                    struct grid_msg response;

                    grid_msg_init_header(&response, GRID_SYS_GLOBAL_POSITION, GRID_SYS_GLOBAL_POSITION);

                    grid_msg_body_append_printf(&response, GRID_CLASS_HIDKEYSTATUS_frame);

                    grid_msg_body_append_parameter(&response, GRID_CLASS_HIDKEYSTATUS_ISENABLED_offset, GRID_CLASS_HIDKEYSTATUS_ISENABLED_length, grid_keyboard_state.isenabled);

                    grid_msg_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);


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
