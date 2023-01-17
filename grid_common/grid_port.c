/*
 * grid_port.c
 *
 * Created: 4/12/2019 5:27:13 PM
 * Author : SUKU WC
*/

#include "grid_port.h"



volatile struct grid_port GRID_PORT_N;
volatile struct grid_port GRID_PORT_E;
volatile struct grid_port GRID_PORT_S;
volatile struct grid_port GRID_PORT_W;

volatile struct grid_port GRID_PORT_U;
volatile struct grid_port GRID_PORT_H;





void grid_port_init(struct grid_port* por, uint8_t type, uint8_t dir){
	
	grid_buffer_init(&por->tx_buffer, GRID_BUFFER_SIZE);
	grid_buffer_init(&por->rx_buffer, GRID_BUFFER_SIZE);
	
	
	por->cooldown = 0;
	
	
	por->direction = dir;
	
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
		
		
		sprintf((char*) por->ping_packet, "%c%c%c%c%02lx%02x%02x%c00\n", GRID_CONST_SOH, GRID_CONST_DCT, GRID_CONST_BELL, por->direction, grid_sys_get_hwcfg(&grid_sys_state), 255, 255, GRID_CONST_EOT);
		
		por->ping_packet_length = strlen((char*) por->ping_packet);	
			
		grid_msg_string_checksum_write(por->ping_packet, por->ping_packet_length, grid_msg_string_calculate_checksum_of_packet_string(por->ping_packet, por->ping_packet_length));
		

		
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
	
	grid_port_init((struct grid_port*) &GRID_PORT_N, GRID_PORT_TYPE_USART, GRID_CONST_NORTH);
	grid_port_init((struct grid_port*) &GRID_PORT_E,  GRID_PORT_TYPE_USART, GRID_CONST_EAST);
	grid_port_init((struct grid_port*) &GRID_PORT_S, GRID_PORT_TYPE_USART, GRID_CONST_SOUTH);
	grid_port_init((struct grid_port*) &GRID_PORT_W,  GRID_PORT_TYPE_USART, GRID_CONST_WEST);
	
	grid_port_init((struct grid_port*) &GRID_PORT_U, GRID_PORT_TYPE_UI, 0);
	grid_port_init((struct grid_port*) &GRID_PORT_H, GRID_PORT_TYPE_USB, 0);	
	
	GRID_PORT_U.partner_status = 1; // UI IS ALWAYS CONNECTED
	GRID_PORT_H.partner_status = 1; // HOST IS ALWAYS CONNECTED (Not really!)
	
	
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
			
			
			grid_platform_send_grid_message(por->direction, por->tx_double_buffer, por->tx_double_buffer_status);
			
			
			return 1;
		}
		
	}
	
	return 0;
}

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
		
	struct grid_msg_packet message;

	//grid_msg_packet_init(&grid_msg_state, &message, 0, 0);
	message.header_length = 0;
	message.body_length = 0;
	message.last_appended_length = 0;
	message.footer_length = 0;
	
	// Let's transfer the packet to local memory
	grid_buffer_read_init(&por->tx_buffer);
		
	for (uint16_t i = 0; i<length; i++){
			
		uint8_t nextchar = grid_buffer_read_character(&por->tx_buffer);
		
		grid_msg_packet_receive_char_by_char(&message, nextchar);
		por->tx_double_buffer[i] = nextchar;	
			
	}
				
	// Let's acknowledge the transfer	(should wait for partner to send ack)
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
			uint8_t msg_class = grid_msg_packet_body_get_parameter(&message, current_start, GRID_PARAMETER_CLASSCODE_offset, GRID_PARAMETER_CLASSCODE_length);
			uint8_t msg_instr = grid_msg_packet_body_get_parameter(&message, current_start, GRID_INSTR_offset, GRID_INSTR_length);
											
			if (msg_class == GRID_CLASS_MIDI_code && msg_instr == GRID_INSTR_EXECUTE_code){
					
										
				uint8_t midi_channel = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_MIDI_CHANNEL_offset,		GRID_CLASS_MIDI_CHANNEL_length);
				uint8_t midi_command = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_MIDI_COMMAND_offset ,	GRID_CLASS_MIDI_COMMAND_length);
				uint8_t midi_param1  = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_MIDI_PARAM1_offset  ,	GRID_CLASS_MIDI_PARAM1_length);
				uint8_t midi_param2  = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_MIDI_PARAM2_offset  ,	GRID_CLASS_MIDI_PARAM2_length);
				
				//grid_platform_printf("midi: %d %d %d %d \r\n", midi_channel, midi_command, midi_param1, midi_param2);
				struct grid_midi_event_desc midievent;
								
				midievent.byte0 = 0<<4|midi_command>>4;
				midievent.byte1 = midi_command|midi_channel;
				midievent.byte2 = midi_param1;
				midievent.byte3 = midi_param2;
				
				if (grid_midi_tx_push(midievent)){
					grid_port_debug_print_text("MIDI TX: Packet Dropped!");
				};
				grid_midi_tx_pop(midievent);				
				
													
			}
			if (msg_class == GRID_CLASS_MIDISYSEX_code && msg_instr == GRID_INSTR_EXECUTE_code){
					

				uint16_t length = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_MIDISYSEX_LENGTH_offset,		GRID_CLASS_MIDISYSEX_LENGTH_length);

				//grid_platform_printf("midi: %d %d %d %d \r\n", midi_channel, midi_command, midi_param1, midi_param2);

								
				//grid_port_debug_printf("Midi Sysex received: %d", length);

				// https://www.usb.org/sites/default/files/midi10.pdf page 17 Table 4-2: Examples of Parsed MIDI Events in 32 -bit USB-MIDI Event Packets
				

				uint8_t first = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_MIDISYSEX_PAYLOAD_offset, GRID_CLASS_MIDISYSEX_PAYLOAD_length);
				uint8_t last = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_MIDISYSEX_PAYLOAD_offset + (length-1)*2, GRID_CLASS_MIDISYSEX_PAYLOAD_length);

				if (first != 0xF0 || last != 0xF7){
					grid_port_debug_printf("Sysex invalid: %d %d", first, last);
				}

				uint32_t number_of_packets_dropped = 0;

				struct grid_midi_event_desc midievent;
				for (uint16_t i=0; i<length;){

					midievent.byte0 = 0;
					midievent.byte1 = 0;
					midievent.byte2 = 0;
					midievent.byte3 = 0;

					midievent.byte1 = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_MIDISYSEX_PAYLOAD_offset+i*2, GRID_CLASS_MIDISYSEX_PAYLOAD_length);
					i++;
					if (i<length){
						midievent.byte2 = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_MIDISYSEX_PAYLOAD_offset+i*2, GRID_CLASS_MIDISYSEX_PAYLOAD_length);
						i++;
					}
					if (i<length){
						midievent.byte3 =  grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_MIDISYSEX_PAYLOAD_offset+i*2, GRID_CLASS_MIDISYSEX_PAYLOAD_length);
						i++;
					}

					if (length<4){  //shortsysex
						if (length == 2){
							midievent.byte0 = 0<<4 | 6;
						}
						if (length == 3){
							midievent.byte0 = 0<<4 | 7;
						}
					}
					else if (i<4){ //first eventpacket of longsysex
						midievent.byte0 = 0<<4 | 4;
					}
					else{ // how many useful bytes are in this eventpacket
						if (i%3 == 0){ // 3
							midievent.byte0 = 0<<4 | 7;
						}
						else if (i%3 == 1){ // 1
							midievent.byte0 = 0<<4 | 5;
						}
						else if (i%3 == 2){ // 2
							midievent.byte0 = 0<<4 | 6;
						}
					}

					//grid_port_debug_printf("Packet: %d %d %d %d", midievent.byte0, midievent.byte1, midievent.byte2, midievent.byte3);
					number_of_packets_dropped += grid_midi_tx_push(midievent);
					// try to pop
					grid_midi_tx_pop(midievent);	
					
				}

				if (number_of_packets_dropped){
					grid_port_debug_printf("MIDI TX: %d Packet(s) Dropped!", number_of_packets_dropped);
				};
							
				
													
			}
			// else if (msg_class == GRID_CLASS_HIDMOUSEBUTTONIMMEDIATE_code && msg_instr == GRID_INSTR_EXECUTE_code){
					
										
			// 	uint8_t state = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_HIDMOUSEBUTTON_STATE_offset, GRID_CLASS_HIDMOUSEBUTTON_STATE_length);
			// 	uint8_t button = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_HIDMOUSEBUTTON_BUTTON_offset ,	GRID_CLASS_HIDMOUSEBUTTON_BUTTON_length);
			
			// 	//grid_port_debug_printf("MouseButton: %d %d", state, button);	
				
			// 	hiddf_mouse_button_change(state, button);
													
			// }
			// else if (msg_class == GRID_CLASS_HIDMOUSEMOVEIMMEDIATE_code && msg_instr == GRID_INSTR_EXECUTE_code){
					
										
			// 	uint8_t position_raw = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_HIDMOUSEMOVE_POSITION_offset, GRID_CLASS_HIDMOUSEMOVE_POSITION_length);
			// 	uint8_t axis = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_HIDMOUSEMOVE_AXIS_offset ,	GRID_CLASS_HIDMOUSEMOVE_AXIS_length);
			
			// 	int8_t position = position_raw - 128;

			// 	//grid_port_debug_printf("MouseMove: %d %d", position, axis);	
				
			// 	hiddf_mouse_move(position, axis);
													
			// }
			else if (msg_class == GRID_CLASS_HIDMOUSEBUTTON_code && msg_instr == GRID_INSTR_EXECUTE_code){
					
										
				uint8_t state = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_HIDMOUSEBUTTON_STATE_offset, GRID_CLASS_HIDMOUSEBUTTON_STATE_length);
				uint8_t button = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_HIDMOUSEBUTTON_BUTTON_offset ,	GRID_CLASS_HIDMOUSEBUTTON_BUTTON_length);
			
				//grid_port_debug_printf("MouseButton: %d %d", state, button);	

				struct grid_keyboard_event_desc key;
			
				key.ismodifier 	= 3; // 0: no, 1: yes, 2: mousemove, 3: mousebutton, f: delay
				key.ispressed 	= state;
				key.keycode 	= button;
				key.delay 		= 1;

				if (0 != grid_keyboard_tx_push(key)){
					grid_port_debug_printf("MOUSE: Packet Dropped!");
				};
													
			}
			else if (msg_class == GRID_CLASS_HIDMOUSEMOVE_code && msg_instr == GRID_INSTR_EXECUTE_code){
											
				uint8_t position_raw = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_HIDMOUSEMOVE_POSITION_offset, GRID_CLASS_HIDMOUSEMOVE_POSITION_length);
				uint8_t axis = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_HIDMOUSEMOVE_AXIS_offset ,	GRID_CLASS_HIDMOUSEMOVE_AXIS_length);
			
				int8_t position = position_raw - 128;

				struct grid_keyboard_event_desc key;
			
				key.ismodifier 	= 2; // 0: no, 1: yes, 2: mousemove, 3: mousebutton, f: delay
				key.ispressed 	= position_raw;
				key.keycode 	= axis;
				key.delay 		= 1;

				if (0 != grid_keyboard_tx_push(key)){
					grid_port_debug_printf("MOUSE: Packet Dropped!");
				};
													
			}
			else if (msg_class == GRID_CLASS_HIDKEYBOARD_code && msg_instr == GRID_INSTR_EXECUTE_code){
				
				uint16_t length =	grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_HIDKEYBOARD_LENGTH_offset,		GRID_CLASS_HIDKEYBOARD_LENGTH_length);
				
				uint8_t default_delay =	grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_HIDKEYBOARD_DEFAULTDELAY_offset,		GRID_CLASS_HIDKEYBOARD_DEFAULTDELAY_length);
				

				uint32_t number_of_packets_dropped = 0;

				for(uint16_t j=0; j<length*4; j+=4){
					
					uint8_t key_ismodifier =	grid_msg_packet_body_get_parameter(&message, current_start+j, GRID_CLASS_HIDKEYBOARD_KEYISMODIFIER_offset,	GRID_CLASS_HIDKEYBOARD_KEYISMODIFIER_length);
					uint8_t key_state  =		grid_msg_packet_body_get_parameter(&message, current_start+j, GRID_CLASS_HIDKEYBOARD_KEYSTATE_offset,		GRID_CLASS_HIDKEYBOARD_KEYSTATE_length);
					uint8_t key_code =			grid_msg_packet_body_get_parameter(&message, current_start+j, GRID_CLASS_HIDKEYBOARD_KEYCODE_offset,		GRID_CLASS_HIDKEYBOARD_KEYCODE_length);


					struct grid_keyboard_event_desc key;

					
					if (key_ismodifier == 0 || key_ismodifier == 1){

						key.ismodifier 	= key_ismodifier;
						key.ispressed 	= key_state;
						key.keycode 	= key_code;
						key.delay 		= default_delay;

						if (key_state == 2){ // combined press and release

							key.ispressed 	= 1;
							number_of_packets_dropped += grid_keyboard_tx_push(key);
							key.ispressed 	= 0;
							number_of_packets_dropped += grid_keyboard_tx_push(key);

						}
						else{ // single press or release

							number_of_packets_dropped += grid_keyboard_tx_push(key);

						}

					}
					else if (key_ismodifier == 0xf){
						// Special delay event

						uint16_t delay = grid_msg_packet_body_get_parameter(&message, current_start+j, GRID_CLASS_HIDKEYBOARD_DELAY_offset, GRID_CLASS_HIDKEYBOARD_DELAY_length);

						key.ismodifier 	= key_ismodifier;
						key.ispressed 	= 0;
						key.keycode 	= 0;
						key.delay 		= delay;

						number_of_packets_dropped += grid_keyboard_tx_push(key);

					}
					else{
						grid_platform_printf("invalid key_ismodifier parameter %d\r\n", key_ismodifier);
					}
					
					// key change fifo buffer

				}
				
				if (number_of_packets_dropped){
					grid_port_debug_printf("KEYBOARD: %d Packet(s) Dropped!", number_of_packets_dropped);
				};

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
		
		por->tx_double_buffer[i] = grid_msg_packet_send_char_by_char(&message, i);

	}
			
	// Let's send the packet through USB
	grid_platform_usb_serial_write(por->tx_double_buffer, packet_length);


	return 0;
}



void grid_port_receiver_softreset(struct grid_port* por){


	
	por->partner_status = 0;

	
	por->rx_double_buffer_timeout = 0;
	

	por->rx_double_buffer_seek_start_index = 0;
	por->rx_double_buffer_read_start_index = 0;

	grid_platform_reset_grid_transmitter(por->direction);

	for(uint16_t i=0; i<GRID_DOUBLE_BUFFER_RX_SIZE; i++){
		por->rx_double_buffer[i] = 0;
	}
	
	for(uint16_t i=0; i<GRID_DOUBLE_BUFFER_TX_SIZE; i++){
		por->tx_double_buffer[i] = 0;
	}

}


void grid_port_receiver_hardreset(struct grid_port* por){

	if (por == &GRID_PORT_E){

		grid_platform_printf("*");
	}


	grid_platform_disable_grid_transmitter(por->direction);


	por->partner_status = 0;
	
	
	por->ping_partner_token = 255;
	por->ping_local_token = 255;
	
	grid_msg_string_write_hex_string_value(&por->ping_packet[8], 2, por->ping_partner_token);
	grid_msg_string_write_hex_string_value(&por->ping_packet[6], 2, por->ping_local_token);
	grid_msg_string_checksum_write(por->ping_packet, por->ping_packet_length, grid_msg_string_calculate_checksum_of_packet_string(por->ping_packet, por->ping_packet_length));


	
	por->rx_double_buffer_timeout = 0;
	grid_platform_reset_grid_transmitter(por->direction);
	


	por->rx_double_buffer_seek_start_index = 0;
	por->rx_double_buffer_read_start_index = 0;

	for(uint16_t i=0; i<GRID_DOUBLE_BUFFER_RX_SIZE; i++){
		por->rx_double_buffer[i] = 0;
	}
	
	for(uint16_t i=0; i<GRID_DOUBLE_BUFFER_TX_SIZE; i++){
		por->tx_double_buffer[i] = 0;
	}
	
	grid_platform_enable_grid_transmitter(por->direction);
	
}


void grid_port_debug_print_text(char* debug_string){
	
	
	struct grid_msg_packet message;
	
	grid_msg_packet_init(&grid_msg_state, &message, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

	grid_msg_packet_body_append_printf(&message, GRID_CLASS_DEBUGTEXT_frame_start);
	grid_msg_packet_body_append_parameter(&message, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);
	grid_msg_packet_body_append_printf(&message, debug_string);
	grid_msg_packet_body_append_printf(&message, GRID_CLASS_DEBUGTEXT_frame_end);

	grid_msg_packet_close(&grid_msg_state, &message);
	grid_port_packet_send_everywhere(&message);
		
}


void grid_port_websocket_print_text(char* debug_string){
	
	
	struct grid_msg_packet message;
	
	grid_msg_packet_init(&grid_msg_state, &message, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

	grid_msg_packet_body_append_printf(&message, GRID_CLASS_WEBSOCKET_frame_start);
	grid_msg_packet_body_append_parameter(&message, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);
	grid_msg_packet_body_append_printf(&message, debug_string);
	grid_msg_packet_body_append_printf(&message, GRID_CLASS_WEBSOCKET_frame_end);

	grid_msg_packet_close(&grid_msg_state, &message);
	grid_port_packet_send_everywhere(&message);
		
}

void grid_port_debug_printf(char const *fmt, ...){

	va_list ap;


	char temp[100] = {0};

	va_start(ap, fmt);

	vsnprintf(temp, 99, fmt, ap);

	va_end(ap);

	grid_port_debug_print_text(temp);

	return;
}




uint8_t	grid_port_packet_send_everywhere(struct grid_msg_packet* msg){
	
	uint32_t message_length = grid_msg_packet_get_length(msg);
	
	if (grid_buffer_write_init((struct grid_buffer*) &GRID_PORT_U.rx_buffer, message_length)){

		for(uint32_t i = 0; i<message_length; i++){

			grid_buffer_write_character((struct grid_buffer*) &GRID_PORT_U.rx_buffer, grid_msg_packet_send_char_by_char(msg, i));
		}

		grid_buffer_write_acknowledge((struct grid_buffer*) &GRID_PORT_U.rx_buffer);

		return 1;
	}
	else{
		
		return 0;
	}
	
	
}



void grid_port_ping_try_everywhere(void){

	//NEW PING
	struct grid_port* port[4] = {&GRID_PORT_N, &GRID_PORT_E, &GRID_PORT_S, &GRID_PORT_W};
	
	for (uint8_t k = 0; k<4; k++){
		
		if (port[k]->ping_flag == 1){
		
			if (grid_buffer_write_init(&port[k]->tx_buffer, port[k]->ping_packet_length)){
				//Success
				for(uint32_t i = 0; i<port[k]->ping_packet_length; i++){
					grid_buffer_write_character(&port[k]->tx_buffer, port[k]->ping_packet[i]);
				}
				grid_buffer_write_acknowledge(&port[k]->tx_buffer);
			}
			port[k]->ping_flag = 0;
		}		
			
	}			



}
