#include "grid_decode.h"

static enum GRID_DESTINATION{

	GRID_DESTINATION_IS_ME = 1,
	GRID_DESTINATION_IS_GLOBAL = 2,
	GRID_DESTINATION_IS_LOCAL = 4,
};

static uint8_t grid_check_destination(char* header, uint8_t target_destination_bm){

	uint8_t error = 0;

	uint8_t position_is_me = 0;
	uint8_t position_is_global = 0;
	uint8_t position_is_local = 0;


	uint8_t dx = grid_msg_string_get_parameter(header, GRID_BRC_DX_offset, GRID_BRC_DX_length, &error);
	uint8_t dy = grid_msg_string_get_parameter(header, GRID_BRC_DY_offset, GRID_BRC_DY_length, &error);
		
	uint8_t header_destination_bm = 0;

	if (dx == GRID_PARAMETER_DEFAULT_POSITION && dy == GRID_PARAMETER_DEFAULT_POSITION){
		//position_is_me = 1;
		header_destination_bm |= GRID_DESTINATION_IS_ME;
	}
	else if (dx == GRID_PARAMETER_GLOBAL_POSITION && dy==GRID_PARAMETER_GLOBAL_POSITION){
		//position_is_global = 1;
		header_destination_bm |= GRID_DESTINATION_IS_GLOBAL;
	}
	else if (dx == GRID_PARAMETER_LOCAL_POSITION && dy==GRID_PARAMETER_LOCAL_POSITION){
		//position_is_local = 1;
		header_destination_bm |= GRID_DESTINATION_IS_LOCAL;
	}


	if ((header_destination_bm&target_destination_bm) != 0){
		//at least one of the match pattersn fit
		return true;

	}
	else{
		return false;

	}

}



// =================== USB OUTBOUND ================= //

uint8_t	grid_decode_midi_to_usb(char* header, char* chunk){

	uint8_t error = 0;

	uint8_t msg_instr = grid_msg_string_get_parameter(chunk, GRID_INSTR_offset, GRID_INSTR_length, &error);
	if (msg_instr != GRID_INSTR_EXECUTE_code){
		return 1;
	}


	uint8_t midi_channel = grid_msg_string_get_parameter(chunk, GRID_CLASS_MIDI_CHANNEL_offset,		GRID_CLASS_MIDI_CHANNEL_length, &error);
	uint8_t midi_command = grid_msg_string_get_parameter(chunk, GRID_CLASS_MIDI_COMMAND_offset ,	GRID_CLASS_MIDI_COMMAND_length, &error);
	uint8_t midi_param1  = grid_msg_string_get_parameter(chunk, GRID_CLASS_MIDI_PARAM1_offset  ,	GRID_CLASS_MIDI_PARAM1_length, &error);
	uint8_t midi_param2  = grid_msg_string_get_parameter(chunk, GRID_CLASS_MIDI_PARAM2_offset  ,	GRID_CLASS_MIDI_PARAM2_length, &error);
	
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

	return 0; //OK	
}

uint8_t	grid_decode_sysex_to_usb(char* header, char* chunk){

	uint8_t error = 0;
			
	uint8_t msg_instr = grid_msg_string_get_parameter(chunk, GRID_INSTR_offset, GRID_INSTR_length, &error);
	if (msg_instr != GRID_INSTR_EXECUTE_code){
		return 1;
	}

	uint16_t length = grid_msg_string_get_parameter(chunk, GRID_CLASS_MIDISYSEX_LENGTH_offset,		GRID_CLASS_MIDISYSEX_LENGTH_length, &error);

	//grid_platform_printf("midi: %d %d %d %d \r\n", midi_channel, midi_command, midi_param1, midi_param2);

					
	//grid_port_debug_printf("Midi Sysex received: %d", length);

	// https://www.usb.org/sites/default/files/midi10.pdf page 17 Table 4-2: Examples of Parsed MIDI Events in 32 -bit USB-MIDI Event Packets
	

	uint8_t first = grid_msg_string_get_parameter(chunk, GRID_CLASS_MIDISYSEX_PAYLOAD_offset, GRID_CLASS_MIDISYSEX_PAYLOAD_length, &error);
	uint8_t last = grid_msg_string_get_parameter(chunk, GRID_CLASS_MIDISYSEX_PAYLOAD_offset + (length-1)*2, GRID_CLASS_MIDISYSEX_PAYLOAD_length, &error);

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

		midievent.byte1 = grid_msg_string_get_parameter(chunk, GRID_CLASS_MIDISYSEX_PAYLOAD_offset+i*2, GRID_CLASS_MIDISYSEX_PAYLOAD_length, &error);
		i++;
		if (i<length){
			midievent.byte2 = grid_msg_string_get_parameter(chunk, GRID_CLASS_MIDISYSEX_PAYLOAD_offset+i*2, GRID_CLASS_MIDISYSEX_PAYLOAD_length, &error);
			i++;
		}
		if (i<length){
			midievent.byte3 =  grid_msg_string_get_parameter(chunk, GRID_CLASS_MIDISYSEX_PAYLOAD_offset+i*2, GRID_CLASS_MIDISYSEX_PAYLOAD_length, &error);
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
	
	}

	// try to pop
	grid_midi_tx_pop(midievent);	
	
	if (number_of_packets_dropped){
		grid_port_debug_printf("MIDI TX: %d Packet(s) Dropped!", number_of_packets_dropped);
	};
							
	
	return 0; //OK
}

uint8_t	grid_decode_mousebutton_to_usb(char* header, char* chunk){

	uint8_t error = 0;
		
	uint8_t msg_instr = grid_msg_string_get_parameter(chunk, GRID_INSTR_offset, GRID_INSTR_length, &error);
	if (msg_instr != GRID_INSTR_EXECUTE_code){
		return 1;
	}					
							
	uint8_t state = grid_msg_string_get_parameter(chunk, GRID_CLASS_HIDMOUSEBUTTON_STATE_offset, GRID_CLASS_HIDMOUSEBUTTON_STATE_length, &error);
	uint8_t button = grid_msg_string_get_parameter(chunk, GRID_CLASS_HIDMOUSEBUTTON_BUTTON_offset ,	GRID_CLASS_HIDMOUSEBUTTON_BUTTON_length, &error);

	//grid_port_debug_printf("MouseButton: %d %d", state, button);	

	struct grid_keyboard_event_desc key;

	key.ismodifier 	= 3; // 0: no, 1: yes, 2: mousemove, 3: mousebutton, f: delay
	key.ispressed 	= state;
	key.keycode 	= button;
	key.delay 		= 1;

	if (0 != grid_keyboard_tx_push(key)){
		grid_port_debug_printf("MOUSE: Packet Dropped!");
	};			
	
	return 0; //OK
}

uint8_t	grid_decode_mousemove_to_usb(char* header, char* chunk){

	uint8_t error = 0;
	
	uint8_t msg_instr = grid_msg_string_get_parameter(chunk, GRID_INSTR_offset, GRID_INSTR_length, &error);
	if (msg_instr != GRID_INSTR_EXECUTE_code){
		return 1;
	}

	uint8_t position_raw = grid_msg_string_get_parameter(chunk, GRID_CLASS_HIDMOUSEMOVE_POSITION_offset, GRID_CLASS_HIDMOUSEMOVE_POSITION_length, &error);
	uint8_t axis = grid_msg_string_get_parameter(chunk, GRID_CLASS_HIDMOUSEMOVE_AXIS_offset ,	GRID_CLASS_HIDMOUSEMOVE_AXIS_length, &error);

	int8_t position = position_raw - 128;

	struct grid_keyboard_event_desc key;

	key.ismodifier 	= 2; // 0: no, 1: yes, 2: mousemove, 3: mousebutton, f: delay
	key.ispressed 	= position_raw;
	key.keycode 	= axis;
	key.delay 		= 1;

	if (0 != grid_keyboard_tx_push(key)){
		grid_port_debug_printf("MOUSE: Packet Dropped!");
	};

	return 0; //OK
}

uint8_t	grid_decode_keyboard_to_usb(char* header, char* chunk){

	uint8_t error = 0;
							
	uint8_t msg_instr = grid_msg_string_get_parameter(chunk, GRID_INSTR_offset, GRID_INSTR_length, &error);
	if (msg_instr != GRID_INSTR_EXECUTE_code){
		return 1;
	}

	uint16_t length =	grid_msg_string_get_parameter(chunk, GRID_CLASS_HIDKEYBOARD_LENGTH_offset,		GRID_CLASS_HIDKEYBOARD_LENGTH_length, &error);
	
	uint8_t default_delay =	grid_msg_string_get_parameter(chunk, GRID_CLASS_HIDKEYBOARD_DEFAULTDELAY_offset,		GRID_CLASS_HIDKEYBOARD_DEFAULTDELAY_length, &error);
	

	uint32_t number_of_packets_dropped = 0;

	for(uint16_t j=0; j<length*4; j+=4){
		
		uint8_t key_ismodifier =	grid_msg_string_get_parameter(chunk, GRID_CLASS_HIDKEYBOARD_KEYISMODIFIER_offset+j,	GRID_CLASS_HIDKEYBOARD_KEYISMODIFIER_length, &error);
		uint8_t key_state  =		grid_msg_string_get_parameter(chunk, GRID_CLASS_HIDKEYBOARD_KEYSTATE_offset+j,		GRID_CLASS_HIDKEYBOARD_KEYSTATE_length, &error);
		uint8_t key_code =			grid_msg_string_get_parameter(chunk, GRID_CLASS_HIDKEYBOARD_KEYCODE_offset+j,		GRID_CLASS_HIDKEYBOARD_KEYCODE_length, &error);


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

			uint16_t delay = grid_msg_string_get_parameter(chunk, GRID_CLASS_HIDKEYBOARD_DELAY_offset+j, GRID_CLASS_HIDKEYBOARD_DELAY_length, &error);

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


	return 0; //OK
}



// =================== UI OUTBOUND ================= //

uint8_t	grid_decode_pageactive_to_ui(char* header, char* chunk){


	uint8_t error = 0;

	uint8_t msg_instr = grid_msg_string_get_parameter(chunk, GRID_INSTR_offset, GRID_INSTR_length, &error);							
	uint8_t page = grid_msg_string_get_parameter(chunk, GRID_CLASS_PAGEACTIVE_PAGENUMBER_offset, GRID_CLASS_PAGEACTIVE_PAGENUMBER_length, &error);

	if (msg_instr == GRID_INSTR_EXECUTE_code){ //SET BANK

		
		if (grid_ui_state.page_change_enabled == false){
			return 0;
		}

		// The set page request was valid, change page now!
		grid_ui_page_load(&grid_ui_state, page);
		grid_sys_set_bank(&grid_sys_state, page);
									
	}
	else if (msg_instr == GRID_INSTR_REPORT_code){ //REPORT BANK

		if (grid_ui_state.page_negotiated == true){
			// page was already negotiated, disable this feature from now on!
			return 0;
		}


		uint8_t sx = grid_msg_string_get_parameter(header, GRID_BRC_SX_offset, GRID_BRC_SX_length, &error);
		uint8_t sy = grid_msg_string_get_parameter(header, GRID_BRC_SY_offset, GRID_BRC_SY_length, &error);

		if (sx==GRID_PARAMETER_DEFAULT_POSITION && sy==GRID_PARAMETER_DEFAULT_POSITION){
			// the report originates from this module, no action is needed!
			return 0;
		}


		grid_ui_state.page_negotiated = true;
		grid_ui_page_load(&grid_ui_state, page);
		grid_sys_set_bank(&grid_sys_state, page);
				
									
	}


	return 0; //OK	
}

uint8_t	grid_decode_pagecount_to_ui(char* header, char* chunk){

	if(grid_check_destination(header, GRID_DESTINATION_IS_ME|GRID_DESTINATION_IS_GLOBAL) == false){
		return 1;
	}

	uint8_t error = 0;

	uint8_t msg_instr = grid_msg_string_get_parameter(chunk, GRID_INSTR_offset, GRID_INSTR_length, &error);							
			
	if (msg_instr == GRID_INSTR_FETCH_code){

		struct grid_msg_packet response;
								
		grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

		grid_msg_packet_body_append_printf(&response, GRID_CLASS_PAGECOUNT_frame);
		grid_msg_packet_body_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);									
		grid_msg_packet_body_set_parameter(&response, 0, GRID_CLASS_PAGECOUNT_PAGENUMBER_offset, GRID_CLASS_PAGECOUNT_PAGENUMBER_length, grid_ui_state.page_count);

		grid_msg_packet_close(&grid_msg_state, &response);
		grid_port_packet_send_everywhere(&response);

	}

	return 0; //OK	
}




uint8_t	grid_decode_midi_to_ui(char* header, char* chunk){



	uint8_t error = 0;

	uint8_t msg_instr = grid_msg_string_get_parameter(chunk, GRID_INSTR_offset, GRID_INSTR_length, &error);							
			
	if (msg_instr == GRID_INSTR_REPORT_code){
							
		uint8_t midi_channel = grid_msg_string_get_parameter(chunk, GRID_CLASS_MIDI_CHANNEL_offset, GRID_CLASS_MIDI_CHANNEL_length, &error);	
		uint8_t midi_command = grid_msg_string_get_parameter(chunk, GRID_CLASS_MIDI_COMMAND_offset, GRID_CLASS_MIDI_COMMAND_length, &error);	
		uint8_t midi_param1 = grid_msg_string_get_parameter(chunk, GRID_CLASS_MIDI_PARAM1_offset, GRID_CLASS_MIDI_PARAM1_length, &error);	
		uint8_t midi_param2 = grid_msg_string_get_parameter(chunk, GRID_CLASS_MIDI_PARAM2_offset, GRID_CLASS_MIDI_PARAM2_length, &error);		
		
		//printf("M: %d %d %d %d \r\n", midi_channel, midi_command, midi_param1, midi_param2);

		char temp[130] = {0};


		grid_lua_clear_stdo(&grid_lua_state);

		// add the received midi message to the dynamic fifo and set the high water mark if necessary
		sprintf(temp, "table.insert(midi_fifo, {%d, %d, %d, %d}) if #midi_fifo > midi_fifo_highwater then midi_fifo_highwater = #midi_fifo end", midi_channel, midi_command, midi_param1, midi_param2);
		grid_lua_dostring(&grid_lua_state, temp);

		grid_lua_clear_stdo(&grid_lua_state);

		struct grid_ui_element* ele = &grid_ui_state.element_list[grid_ui_state.element_list_length-1];
		struct grid_ui_event* eve = NULL;

		eve = grid_ui_event_find(ele, GRID_UI_EVENT_MIDIRX);
		if (eve != NULL){

			grid_ui_event_trigger(eve);

		}

	}	

	return 0; //OK

}

uint8_t	grid_decode_imediate_to_ui(char* header, char* chunk){

	if(grid_check_destination(header, GRID_DESTINATION_IS_ME|GRID_DESTINATION_IS_GLOBAL|GRID_DESTINATION_IS_LOCAL) == false){
		return 1;
	}

	uint8_t error = 0;
							
	uint8_t msg_instr = grid_msg_string_get_parameter(chunk, GRID_INSTR_offset, GRID_INSTR_length, &error);							
			
	if (msg_instr == GRID_INSTR_EXECUTE_code){


		uint16_t length = grid_msg_string_get_parameter(chunk, GRID_CLASS_IMEDIATE_ACTIONLENGTH_offset, GRID_CLASS_IMEDIATE_ACTIONLENGTH_length, &error);
			
		char* lua_script = &chunk[GRID_CLASS_IMEDIATE_ACTIONSTRING_offset];


		if (0 != strncmp(lua_script, "<?lua ", 6) ){
			// incorrect opening tag
			printf("IMEDIATE NOT OK %d: %s\r\n", length, lua_script);
			return 1; //NOT OK
		}

		if (0 != strncmp(&lua_script[length-3], " ?>", 3)){		
			// incorrect closing tag
			printf("IMEDIATE NOT OK %d: %s\r\n", length, lua_script);
			return 1; //NOT OK

		}

		
		lua_script[length-3] = '\0'; // add terminating zero

		printf("IMEDIATE %d: %s\r\n", length, lua_script);

		grid_lua_dostring(&grid_lua_state, &lua_script[6]);

		lua_script[length-3] = ' '; // restore packet!

	}

	return 0; //OK
}


uint8_t	grid_decode_heartbeat_to_ui(char* header, char* chunk){


	uint8_t error = 0;

	uint8_t type  = grid_msg_string_get_parameter(chunk, GRID_CLASS_HEARTBEAT_TYPE_offset, GRID_CLASS_HEARTBEAT_TYPE_length, &error);
	
	uint8_t editor_connected_now = 0;



	if (type == 0){
		// from other grid module
	}
	else if (type == 1){

		// Heartbeat is from USB connected module, let's calculate absolute position based on that!

		uint8_t sx = grid_msg_string_get_parameter(header, GRID_BRC_SX_offset, GRID_BRC_SX_length, &error);
		uint8_t sy = grid_msg_string_get_parameter(header, GRID_BRC_SY_offset, GRID_BRC_SY_length, &error);
		uint8_t rot = grid_msg_string_get_parameter(header, GRID_BRC_ROT_offset, GRID_BRC_ROT_length, &error);
		uint8_t portrot = grid_msg_string_get_parameter(header, GRID_BRC_PORTROT_offset, GRID_BRC_PORTROT_length, &error);

		grid_sys_set_module_absolute_position(&grid_sys_state, sx, sy, rot, portrot);

	}
	else if (type >127){ // editor


		if (grid_sys_get_editor_connected_state(&grid_sys_state) == 0){

			grid_sys_set_editor_connected_state(&grid_sys_state, 1);
		
			editor_connected_now = 1;
			grid_port_debug_print_text("EDITOR connect");
		}

		grid_msg_set_editor_heartbeat_lastrealtime(&grid_msg_state, grid_platform_rtc_get_micros());

		if (type == 255){
			grid_ui_state.page_change_enabled = 1;
			//printf("255\r\n");
		}
		else{
			grid_ui_state.page_change_enabled = 0;
			//printf("254\r\n");
		}

		uint8_t ui_report_valid = 0;


		if (editor_connected_now){

			uint16_t report_length = 0;
			struct grid_msg_packet response;
									
			grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

			char response_payload[300] = {0};
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

				//printf("Element %d: %d\r\n", element_num, element_value);
				
				//grid_led_state.led_lowlevel_changed[j] = 0;

				ui_report_valid = 1;

				sprintf(&response_payload[len], "%02x%02x", element_num, element_value);
				len += strlen(&response_payload[len]);

			}


			sprintf(&response_payload[len], GRID_CLASS_EVENTPREVIEW_frame_end);
			len += strlen(&response_payload[len]);

			grid_msg_packet_body_append_text(&response, response_payload);
				

			grid_msg_packet_body_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);													
			grid_msg_packet_body_set_parameter(&response, 0, GRID_CLASS_EVENTPREVIEW_LENGTH_offset, GRID_CLASS_EVENTPREVIEW_LENGTH_length, report_length);
			
			grid_msg_packet_close(&grid_msg_state, &response);
			grid_port_packet_send_everywhere(&response);

			//printf(response.body);
			//printf("\r\n");
		}

		// report stringnames
		if (editor_connected_now){

			uint16_t report_length = 0;
			struct grid_msg_packet response;
									
			grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);


			// -1 to exclude system element
			for(uint8_t j=0; j<grid_ui_state.element_list_length-1; j++){

				struct grid_ui_element* ele = &grid_ui_state.element_list[j];
				
				uint8_t number = j;
				char command[26] = {0};

				sprintf(command, "gens(%d,ele[%d]:gen())", j, j);

				// lua get element name
				grid_lua_clear_stdo(&grid_lua_state);
				grid_lua_dostring(&grid_lua_state, command);

				grid_msg_packet_body_append_text(&response, grid_lua_state.stdo);

			}

			grid_msg_packet_close(&grid_msg_state, &response);
			grid_port_packet_send_everywhere(&response);

			//printf(response.body);
			//printf("\r\n");
		}


		// Report the state of the changed leds

		if (editor_connected_now){
			// reset the changed flags to force report all leds
			grid_led_change_flag_reset(&grid_led_state);
		}


		if (grid_protocol_led_change_report_length(&grid_led_state)){


			struct grid_msg_packet response;
									
			grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

			char response_payload[300] = {0};
			uint16_t len = 0;
			snprintf(response_payload, 299, GRID_CLASS_LEDPREVIEW_frame_start);
			len += strlen(&response_payload[len]);

			uint16_t report_length = grid_protocol_led_change_report_generate(&grid_led_state, -1, &response_payload[len]);

			len += strlen(&response_payload[len]);

			grid_msg_packet_body_append_text(&response, response_payload);


			grid_msg_packet_body_append_printf(&response, GRID_CLASS_LEDPREVIEW_frame_end);

			grid_msg_packet_body_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);													
			grid_msg_packet_body_set_parameter(&response, 0, GRID_CLASS_LEDPREVIEW_LENGTH_offset, GRID_CLASS_LEDPREVIEW_LENGTH_length, report_length);
			
			grid_msg_packet_close(&grid_msg_state, &response);
			grid_port_packet_send_everywhere(&response);

		}

		

		// from editor

	}
	else{
		// unknown type
	}

	return 0; //OK	
}

uint8_t	grid_decode_serialmuber_to_ui(char* header, char* chunk){

	if(grid_check_destination(header, GRID_DESTINATION_IS_ME|GRID_DESTINATION_IS_GLOBAL) == false){
		return 1;
	}

	uint8_t error = 0;

	uint8_t msg_instr = grid_msg_string_get_parameter(chunk, GRID_INSTR_offset, GRID_INSTR_length, &error);							
			
	if (msg_instr == GRID_INSTR_FETCH_code){

		uint32_t uniqueid[4] = {0};
		grid_sys_get_id(&grid_sys_state, uniqueid);					
		// Generate RESPONSE
		struct grid_msg_packet response;
								
		grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

		char response_payload[50] = {0};
		snprintf(response_payload, 49, GRID_CLASS_SERIALNUMBER_frame);

		grid_msg_packet_body_append_text(&response, response_payload);
			
		grid_msg_packet_body_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);					
								
		grid_msg_packet_body_set_parameter(&response, 0, GRID_CLASS_SERIALNUMBER_WORD0_offset, GRID_CLASS_SERIALNUMBER_WORD0_length, uniqueid[0]);
		grid_msg_packet_body_set_parameter(&response, 0, GRID_CLASS_SERIALNUMBER_WORD1_offset, GRID_CLASS_SERIALNUMBER_WORD1_length, uniqueid[1]);
		grid_msg_packet_body_set_parameter(&response, 0, GRID_CLASS_SERIALNUMBER_WORD2_offset, GRID_CLASS_SERIALNUMBER_WORD2_length, uniqueid[2]);
		grid_msg_packet_body_set_parameter(&response, 0, GRID_CLASS_SERIALNUMBER_WORD3_offset, GRID_CLASS_SERIALNUMBER_WORD3_length, uniqueid[3]);


								
		grid_msg_packet_close(&grid_msg_state, &response);
		grid_port_packet_send_everywhere(&response);

	}

	return 0; //OK	
}

uint8_t	grid_decode_uptime_to_ui(char* header, char* chunk){	
	
	if(grid_check_destination(header, GRID_DESTINATION_IS_ME|GRID_DESTINATION_IS_GLOBAL) == false){
		return 1;
	}
	uint8_t error = 0;

	uint8_t msg_instr = grid_msg_string_get_parameter(chunk, GRID_INSTR_offset, GRID_INSTR_length, &error);							
			
	if (msg_instr == GRID_INSTR_FETCH_code){

		// Generate RESPONSE
		struct grid_msg_packet response;

		grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

		grid_msg_packet_body_append_printf(&response, GRID_CLASS_UPTIME_frame);

		grid_msg_packet_body_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);


		uint64_t uptime = grid_platform_rtc_get_micros();

		grid_msg_packet_body_set_parameter(&response, 0, GRID_CLASS_UPTIME_UPTIME_offset, GRID_CLASS_UPTIME_UPTIME_length, (uint32_t)uptime);
		
		
		uint32_t milliseconds = uptime/MS_TO_US%1000;
		uint32_t seconds = 		uptime/MS_TO_US/1000%60;
		uint32_t minutes =		uptime/MS_TO_US/1000/60%60;
		uint32_t hours =		uptime/MS_TO_US/1000/60/60%60;
		

		grid_msg_packet_close(&grid_msg_state, &response);
		grid_port_packet_send_everywhere(&response);

	}

	return 0; //OK	

}

uint8_t	grid_decode_resetcause_to_ui(char* header, char* chunk){	

	if(grid_check_destination(header, GRID_DESTINATION_IS_ME|GRID_DESTINATION_IS_GLOBAL) == false){
		return 1;
	}
	
	uint8_t error = 0;

	uint8_t msg_instr = grid_msg_string_get_parameter(chunk, GRID_INSTR_offset, GRID_INSTR_length, &error);							
			
	if (msg_instr == GRID_INSTR_FETCH_code){

		struct grid_msg_packet response;

		grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

		grid_msg_packet_body_append_printf(&response, GRID_CLASS_RESETCAUSE_frame);

		grid_msg_packet_body_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);



		grid_msg_packet_body_set_parameter(&response, 0, GRID_CLASS_RESETCAUSE_CAUSE_offset, GRID_CLASS_RESETCAUSE_CAUSE_length, grid_platform_get_reset_cause());

		grid_msg_packet_close(&grid_msg_state, &response);
		grid_port_packet_send_everywhere(&response);

	}

	return 0; //OK	

}

uint8_t	grid_decode_reset_to_ui(char* header, char* chunk){	
	
	if(grid_check_destination(header, GRID_DESTINATION_IS_ME|GRID_DESTINATION_IS_GLOBAL) == false){
		return 1;
	}

	uint8_t error = 0;


	uint8_t msg_instr = grid_msg_string_get_parameter(chunk, GRID_INSTR_offset, GRID_INSTR_length, &error);							
			
	if (msg_instr == GRID_INSTR_EXECUTE_code){

		grid_platform_system_reset();

	}

	return 0; //OK	

}

uint8_t	grid_decode_pagediscard_to_ui(char* header, char* chunk){	
		
	if(grid_check_destination(header, GRID_DESTINATION_IS_ME|GRID_DESTINATION_IS_GLOBAL) == false){
		return 1;
	}
	
	uint8_t error = 0;

	uint8_t msg_instr = grid_msg_string_get_parameter(chunk, GRID_INSTR_offset, GRID_INSTR_length, &error);							


	if (msg_instr == GRID_INSTR_EXECUTE_code){
	
	
		uint8_t id = grid_msg_string_get_parameter(header, GRID_BRC_ID_offset, GRID_BRC_ID_length, &error);

		grid_msg_store_lastheader(&grid_msg_state, GRID_MSG_LASTHEADER_DISCARD_INDEX, id);

		if (grid_ui_bulk_pageread_is_in_progress(&grid_ui_state)){

		}else{
			grid_ui_bulk_pageread_init(&grid_ui_state, &grid_protocol_nvm_read_succcess_callback);
		}
	
	
	}
	else if (msg_instr == GRID_INSTR_CHECK_code){
	
			
		uint8_t state = grid_msg_get_lastheader_state(&grid_msg_state, GRID_MSG_LASTHEADER_DISCARD_INDEX);
		uint8_t id = grid_msg_get_lastheader_id(&grid_msg_state, GRID_MSG_LASTHEADER_DISCARD_INDEX);

		struct grid_msg_packet response;	
		grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);
		grid_msg_packet_body_append_printf(&response, GRID_CLASS_PAGEDISCARD_frame);
		grid_msg_packet_body_append_parameter(&response, GRID_CLASS_PAGEDISCARD_LASTHEADER_offset, GRID_CLASS_PAGEDISCARD_LASTHEADER_length, id);		


		uint8_t acknowlege = (state==0)?GRID_INSTR_ACKNOWLEDGE_code:GRID_INSTR_NACKNOWLEDGE_code;
		grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, acknowlege);

		grid_msg_packet_close(&grid_msg_state, &response);
		grid_port_packet_send_everywhere(&response);	

	
	}


			
	return 0; //OK	

}

uint8_t	grid_decode_pagestore_to_ui(char* header, char* chunk){	

	if(grid_check_destination(header, GRID_DESTINATION_IS_ME|GRID_DESTINATION_IS_GLOBAL) == false){
		return 1;
	}
	
	uint8_t error = 0;

	uint8_t msg_instr = grid_msg_string_get_parameter(chunk, GRID_INSTR_offset, GRID_INSTR_length, &error);							
				

	if (msg_instr == GRID_INSTR_EXECUTE_code){

		uint8_t id = grid_msg_string_get_parameter(header, GRID_BRC_ID_offset, GRID_BRC_ID_length, &error);
						
		grid_msg_store_lastheader(&grid_msg_state, GRID_MSG_LASTHEADER_STORE_INDEX, id);
		
		// start animation (it will be stopped in the callback function)
		grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_YELLOW_DIM, -1);		
		grid_led_set_alert_frequency(&grid_led_state, -4);	

		grid_ui_bulk_pagestore_init(&grid_ui_state, &grid_protocol_nvm_store_succcess_callback);	
	
	
	}
	else if (msg_instr == GRID_INSTR_CHECK_code){
	
		uint8_t state = grid_msg_get_lastheader_state(&grid_msg_state, GRID_MSG_LASTHEADER_STORE_INDEX);
		uint8_t id = grid_msg_get_lastheader_id(&grid_msg_state, GRID_MSG_LASTHEADER_STORE_INDEX);

		struct grid_msg_packet response;	
		grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);
		grid_msg_packet_body_append_printf(&response, GRID_CLASS_PAGESTORE_frame);
		grid_msg_packet_body_append_parameter(&response, GRID_CLASS_PAGESTORE_LASTHEADER_offset, GRID_CLASS_PAGESTORE_LASTHEADER_length, id);		
	
		uint8_t acknowlege = (state==0)?GRID_INSTR_ACKNOWLEDGE_code:GRID_INSTR_NACKNOWLEDGE_code;
		grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, acknowlege);

		grid_msg_packet_close(&grid_msg_state, &response);
		grid_port_packet_send_everywhere(&response);	
	
	}


			
	return 0; //OK	

}

uint8_t	grid_decode_pageclear_to_ui(char* header, char* chunk){
	
	if(grid_check_destination(header, GRID_DESTINATION_IS_ME|GRID_DESTINATION_IS_GLOBAL) == false){
		return 1;
	}

	uint8_t error = 0;

	uint8_t msg_instr = grid_msg_string_get_parameter(chunk, GRID_INSTR_offset, GRID_INSTR_length, &error);							
				

	if (msg_instr == GRID_INSTR_EXECUTE_code){
		
		uint8_t id = grid_msg_string_get_parameter(header, GRID_BRC_ID_offset, GRID_BRC_ID_length, &error);
						
		grid_msg_store_lastheader(&grid_msg_state, GRID_MSG_LASTHEADER_CLEAR_INDEX, id);			
		grid_ui_bulk_pageclear_init(&grid_ui_state, &grid_protocol_nvm_clear_succcess_callback);					

	}
	else if (msg_instr == GRID_INSTR_CHECK_code){
	
	
		uint8_t state = grid_msg_get_lastheader_state(&grid_msg_state, GRID_MSG_LASTHEADER_CLEAR_INDEX);
		uint8_t id = grid_msg_get_lastheader_id(&grid_msg_state, GRID_MSG_LASTHEADER_CLEAR_INDEX);

		struct grid_msg_packet response;	
		grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);
		grid_msg_packet_body_append_printf(&response, GRID_CLASS_PAGECLEAR_frame);
		grid_msg_packet_body_append_parameter(&response, GRID_CLASS_PAGECLEAR_LASTHEADER_offset, GRID_CLASS_PAGECLEAR_LASTHEADER_length, id);		
	
		uint8_t acknowlege = (state==0)?GRID_INSTR_ACKNOWLEDGE_code:GRID_INSTR_NACKNOWLEDGE_code;
		grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, acknowlege);

		grid_msg_packet_close(&grid_msg_state, &response);
		grid_port_packet_send_everywhere(&response);	
	
	}


			
	return 0; //OK	

}

uint8_t	grid_decode_nvmerase_to_ui(char* header, char* chunk){

	if(grid_check_destination(header, GRID_DESTINATION_IS_ME|GRID_DESTINATION_IS_GLOBAL) == false){
		return 1;
	}
	
	uint8_t error = 0;

	uint8_t msg_instr = grid_msg_string_get_parameter(chunk, GRID_INSTR_offset, GRID_INSTR_length, &error);							
				

	if (msg_instr == GRID_INSTR_EXECUTE_code){
		
		uint8_t id = grid_msg_string_get_parameter(header, GRID_BRC_ID_offset, GRID_BRC_ID_length, &error);

		grid_msg_store_lastheader(&grid_msg_state, GRID_MSG_LASTHEADER_ERASE_INDEX, id);
		grid_ui_bulk_nvmerase_init(&grid_ui_state, &grid_protocol_nvm_erase_succcess_callback);


		// start animation (it will be stopped in the callback function)
		grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_YELLOW_DIM, -1);	
		grid_led_set_alert_frequency(&grid_led_state, -2);	

		for (uint8_t i = 0; i<grid_led_get_led_count(&grid_led_state); i++){
			grid_led_set_layer_min(&grid_led_state, i, GRID_LED_LAYER_ALERT, GRID_LED_COLOR_YELLOW_DIM);
		}

	}
	else if (msg_instr == GRID_INSTR_CHECK_code){
	
		uint8_t state = grid_msg_get_lastheader_state(&grid_msg_state, GRID_MSG_LASTHEADER_ERASE_INDEX);
		uint8_t id = grid_msg_get_lastheader_id(&grid_msg_state, GRID_MSG_LASTHEADER_ERASE_INDEX);


		struct grid_msg_packet response;	
		grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);
		grid_msg_packet_body_append_printf(&response, GRID_CLASS_NVMERASE_frame);
		grid_msg_packet_body_append_parameter(&response, GRID_CLASS_NVMERASE_LASTHEADER_offset, GRID_CLASS_NVMERASE_LASTHEADER_length, id);		
	
		uint8_t acknowlege = (state==0)?GRID_INSTR_ACKNOWLEDGE_code:GRID_INSTR_NACKNOWLEDGE_code;
		grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, acknowlege);

		grid_msg_packet_close(&grid_msg_state, &response);
		grid_port_packet_send_everywhere(&response);	
	
	}


			
	return 0; //OK	

}

uint8_t	grid_decode_nvmdefrag_to_ui(char* header, char* chunk){

	if(grid_check_destination(header, GRID_DESTINATION_IS_ME|GRID_DESTINATION_IS_GLOBAL) == false){
		return 1;
	}

	uint8_t error = 0;

	uint8_t msg_instr = grid_msg_string_get_parameter(chunk, GRID_INSTR_offset, GRID_INSTR_length, &error);							
				

	if (msg_instr == GRID_INSTR_EXECUTE_code){

		
		grid_platform_nvm_defrag();

	}
	else if (msg_instr == GRID_INSTR_CHECK_code){
	

	
	}


			
	return 0; //OK	

}

uint8_t	grid_decode_config_to_ui(char* header, char* chunk){

	if(grid_check_destination(header, GRID_DESTINATION_IS_ME|GRID_DESTINATION_IS_LOCAL) == false){
		return 1;
	}

	uint8_t error = 0;


	uint8_t msg_instr = grid_msg_string_get_parameter(chunk, GRID_INSTR_offset, GRID_INSTR_length, &error);	


	if (msg_instr == GRID_INSTR_EXECUTE_code){

		uint8_t id = grid_msg_string_get_parameter(header, GRID_BRC_ID_offset, GRID_BRC_ID_length, &error);
		
		grid_msg_store_lastheader(&grid_msg_state, GRID_MSG_LASTHEADER_CONFIG_INDEX, id);
		
		// disable hid automatically
		grid_keyboard_disable(&grid_keyboard_state);          
		//grid_port_debug_print_text("Disabling KB");

		uint8_t vmajor = grid_msg_string_get_parameter(chunk, GRID_CLASS_CONFIG_VERSIONMAJOR_offset, GRID_CLASS_CONFIG_VERSIONMAJOR_length, NULL);
		uint8_t vminor = grid_msg_string_get_parameter(chunk, GRID_CLASS_CONFIG_VERSIONMINOR_offset, GRID_CLASS_CONFIG_VERSIONMINOR_length, NULL);
		uint8_t vpatch = grid_msg_string_get_parameter(chunk, GRID_CLASS_CONFIG_VERSIONPATCH_offset, GRID_CLASS_CONFIG_VERSIONPATCH_length, NULL);
					
		uint8_t pagenumber = grid_msg_string_get_parameter(chunk, GRID_CLASS_CONFIG_PAGENUMBER_offset, GRID_CLASS_CONFIG_PAGENUMBER_length, NULL);
		uint8_t elementnumber = grid_msg_string_get_parameter(chunk, GRID_CLASS_CONFIG_ELEMENTNUMBER_offset, GRID_CLASS_CONFIG_ELEMENTNUMBER_length, NULL);
		uint8_t eventtype = grid_msg_string_get_parameter(chunk, GRID_CLASS_CONFIG_EVENTTYPE_offset, GRID_CLASS_CONFIG_EVENTTYPE_length, NULL);
		uint16_t actionlength = grid_msg_string_get_parameter(chunk, GRID_CLASS_CONFIG_ACTIONLENGTH_offset, GRID_CLASS_CONFIG_ACTIONLENGTH_length, NULL);


		if (elementnumber == 255){
			
			elementnumber = grid_ui_state.element_list_length - 1;
		}	


		char* action = &chunk[GRID_CLASS_CONFIG_ACTIONSTRING_offset];

		uint8_t ack = GRID_INSTR_NACKNOWLEDGE_code; // nacknowledge by default

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
						ack = GRID_INSTR_ACKNOWLEDGE_code;

						//grid_port_debug_printf("autotrigger: %d", autotrigger);

						grid_ui_event_trigger_local(eve);	
						


					}

				}
				
				action[actionlength] = GRID_CONST_ETX;
			}
			else{
				grid_port_debug_printf("config too long");
			}

		}
		else{

			printf("Config frame invalid: %d %d %d %d end: %d %s\r\n", pagenumber, elementnumber, eventtype, actionlength, action[actionlength], chunk);

		}


		// Generate ACKNOWLEDGE RESPONSE
		struct grid_msg_packet response;

		grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);
		
		grid_msg_packet_body_append_printf(&response, GRID_CLASS_CONFIG_frame_check);
		
		grid_msg_packet_body_append_parameter(&response, GRID_CLASS_CONFIG_LASTHEADER_offset, GRID_CLASS_CONFIG_LASTHEADER_length, id);
		
		
		grid_msg_clear_lastheader(&grid_msg_state, GRID_MSG_LASTHEADER_CONFIG_INDEX);
		


		grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, ack);
		grid_port_debug_printf("Config %01x %d", ack, id);



		grid_msg_packet_close(&grid_msg_state, &response);
		grid_port_packet_send_everywhere(&response);

	}
	else if (msg_instr == GRID_INSTR_FETCH_code){
	
		//grid_platform_printf("CONFIG FETCH\r\n\r\n");
				
		uint8_t pagenumber = grid_msg_string_get_parameter(chunk, GRID_CLASS_CONFIG_PAGENUMBER_offset, GRID_CLASS_CONFIG_PAGENUMBER_length, NULL);
		uint8_t elementnumber = grid_msg_string_get_parameter(chunk, GRID_CLASS_CONFIG_ELEMENTNUMBER_offset, GRID_CLASS_CONFIG_ELEMENTNUMBER_length, NULL);
		uint8_t eventtype = grid_msg_string_get_parameter(chunk, GRID_CLASS_CONFIG_EVENTTYPE_offset, GRID_CLASS_CONFIG_EVENTTYPE_length, NULL);
		//uint16_t actionlength = grid_msg_string_get_parameter(chunk, GRID_CLASS_CONFIG_ACTIONLENGTH_offset, GRID_CLASS_CONFIG_ACTIONLENGTH_length, NULL);

		// Helper to map system element to 255
		if (elementnumber == 255){
			elementnumber = grid_ui_state.element_list_length - 1;
		}


		char temp[GRID_PARAMETER_ACTIONSTRING_maxlength]  = {0};

		grid_ui_event_recall_configuration(&grid_ui_state, pagenumber, elementnumber, eventtype, temp);
		
		//grid_platform_printf("CONFIG: %s\r\n\r\n", temp);


		if (strlen(temp) != 0){



			struct grid_msg_packet message;

			grid_msg_packet_init(&grid_msg_state, &message, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

			grid_msg_packet_body_append_printf(&message, GRID_CLASS_CONFIG_frame_start);

			grid_msg_packet_body_append_parameter(&message, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);
			
			grid_msg_packet_body_append_parameter(&message, GRID_CLASS_CONFIG_VERSIONMAJOR_offset, GRID_CLASS_CONFIG_VERSIONMAJOR_length, GRID_PROTOCOL_VERSION_MAJOR);
			grid_msg_packet_body_append_parameter(&message, GRID_CLASS_CONFIG_VERSIONMINOR_offset, GRID_CLASS_CONFIG_VERSIONMINOR_length, GRID_PROTOCOL_VERSION_MINOR);
			grid_msg_packet_body_append_parameter(&message, GRID_CLASS_CONFIG_VERSIONPATCH_offset, GRID_CLASS_CONFIG_VERSIONPATCH_length, GRID_PROTOCOL_VERSION_PATCH);

			// Helper to map system element to 255
			uint8_t element_helper = elementnumber;
			if (elementnumber == grid_ui_state.element_list_length - 1){
				element_helper = 255;
			}

			grid_msg_packet_body_append_parameter(&message, GRID_CLASS_CONFIG_PAGENUMBER_offset, GRID_CLASS_CONFIG_PAGENUMBER_length, pagenumber);
			grid_msg_packet_body_append_parameter(&message, GRID_CLASS_CONFIG_ELEMENTNUMBER_offset, GRID_CLASS_CONFIG_EVENTTYPE_length, element_helper);
			grid_msg_packet_body_append_parameter(&message, GRID_CLASS_CONFIG_EVENTTYPE_offset, GRID_CLASS_CONFIG_EVENTTYPE_length, eventtype);
			grid_msg_packet_body_append_parameter(&message, GRID_CLASS_CONFIG_ACTIONLENGTH_offset, GRID_CLASS_CONFIG_ACTIONLENGTH_length, 0);



			grid_msg_packet_body_append_parameter(&message, GRID_CLASS_CONFIG_ACTIONLENGTH_offset, GRID_CLASS_CONFIG_ACTIONLENGTH_length, strlen(temp));		
			grid_msg_packet_body_append_text(&message, temp);

			grid_msg_packet_body_append_printf(&message, GRID_CLASS_CONFIG_frame_end);


			//printf("CFG: %s\r\n", message.body);
			grid_msg_packet_close(&grid_msg_state, &message);
			grid_port_packet_send_everywhere(&message);
			

		}

	
	}

	return 0; //OK

}


uint8_t	grid_decode_hidkeystatus_to_ui(char* header, char* chunk){

	if(grid_check_destination(header, GRID_DESTINATION_IS_ME|GRID_DESTINATION_IS_GLOBAL) == false){
		return 1;
	}

	uint8_t error = 0;

	uint8_t isenabled =	grid_msg_string_get_parameter(chunk, GRID_CLASS_HIDKEYSTATUS_ISENABLED_offset, GRID_CLASS_HIDKEYSTATUS_ISENABLED_length	, &error);
	

	uint8_t msg_instr = grid_msg_string_get_parameter(chunk, GRID_INSTR_offset, GRID_INSTR_length, &error);							
			
	if (msg_instr == GRID_INSTR_EXECUTE_code){

		if (isenabled){
			grid_keyboard_enable(&grid_keyboard_state);
		}
		else{
			grid_keyboard_disable(&grid_keyboard_state);
		}

		
		// Generate ACKNOWLEDGE RESPONSE
		struct grid_msg_packet response;

		grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

		grid_msg_packet_body_append_printf(&response, GRID_CLASS_HIDKEYSTATUS_frame);

		grid_msg_packet_body_append_parameter(&response, GRID_CLASS_HIDKEYSTATUS_ISENABLED_offset, GRID_CLASS_HIDKEYSTATUS_ISENABLED_length, grid_keyboard_isenabled(&grid_keyboard_state));

		grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);


		grid_msg_packet_close(&grid_msg_state, &response);
		grid_port_packet_send_everywhere(&response);

	}

	return 0; //OK
}