#include "grid_ui.h"



void grid_port_process_ui(struct grid_port* por){
	
	// Priorities: Always process local, try to process direct, broadcast messages are last. 
	
	
	uint8_t message_broadcast_action_available = 0;


	
	// UI STATE
	for (uint8_t i=0; i<grid_ui_state.element_list_length; i++){
		
		for (uint8_t j=0; j<grid_ui_state.element[i].event_list_length; j++){
			
			if (grid_ui_event_istriggered(&grid_ui_state.element[i].event_list[j])){
				
				message_broadcast_action_available++;
			}
			
		}
		
	}		
	
	// CORE SYSTEM
	for (uint8_t i=0; i<grid_core_state.element_list_length; i++){
		
		for (uint8_t j=0; j<grid_core_state.element[i].event_list_length; j++){
			
			if (grid_ui_event_istriggered(&grid_core_state.element[i].event_list[j])){
				
				message_broadcast_action_available++;
			}
			
		}
		
	}	
	
	
	
	//NEW
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

	
	
		
	// Bandwidth Limiter for Broadcast messages
	if (por->cooldown > 15){
		
		por->cooldown--;
		return;
	}
	else if (por->cooldown>0){
		
		por->cooldown--;
	}	
	
	
	//BROADCAST MESSAGES		
	if (message_broadcast_action_available){
			
		// Prepare packet header
		uint8_t message[GRID_PARAMETER_PACKET_maxlength] = {0};
		uint16_t length=0;	
		uint8_t packetvalid = 0;
	
		sprintf(&message[length], GRID_BRC_frame);
		
		uint8_t error = 0;

		grid_msg_set_parameter(&message[length], GRID_BRC_LEN_offset, GRID_BRC_LEN_length, 0, &error);
		grid_msg_set_parameter(&message[length], GRID_BRC_ID_offset , GRID_BRC_ID_length , grid_sys_state.next_broadcast_message_id,  &error);
		grid_msg_set_parameter(&message[length], GRID_BRC_DX_offset , GRID_BRC_DX_length , GRID_SYS_DEFAULT_POSITION,  &error);
		grid_msg_set_parameter(&message[length], GRID_BRC_DY_offset , GRID_BRC_DY_length , GRID_SYS_DEFAULT_POSITION,  &error);
		grid_msg_set_parameter(&message[length], GRID_BRC_AGE_offset, GRID_BRC_AGE_length, grid_sys_state.age, &error);
		grid_msg_set_parameter(&message[length], GRID_BRC_ROT_offset, GRID_BRC_ROT_length, GRID_SYS_DEFAULT_ROTATION, &error);

		length += strlen(&message[length]);
	
		// CORE SYSTEM
		for (uint8_t i=0; i<grid_core_state.element_list_length; i++){
			
			for (uint8_t j=0; j<grid_core_state.element[i].event_list_length; j++){
				
				if (length>GRID_PARAMETER_PACKET_marign){
					continue;
				}						
				
				CRITICAL_SECTION_ENTER()
				if (grid_ui_event_istriggered(&grid_core_state.element[i].event_list[j])){
					
					packetvalid++;
					grid_ui_event_render_action(&grid_core_state.element[i].event_list[j], &message[length]);
					length += strlen(&message[length]);
					grid_ui_event_reset(&grid_core_state.element[i].event_list[j]);
					
				}
				CRITICAL_SECTION_LEAVE()
				
			}
			
		}
		
		// UI STATE
		for (uint8_t i=0; i<grid_ui_state.element_list_length; i++){
			
			for (uint8_t j=0; j<grid_ui_state.element[i].event_list_length; j++){
				
				if (length>GRID_PARAMETER_PACKET_marign){
					continue;
				}		
						
				CRITICAL_SECTION_ENTER()
				if (grid_ui_event_istriggered(&grid_ui_state.element[i].event_list[j])){
					
					packetvalid++;				
					grid_ui_event_render_action(&grid_ui_state.element[i].event_list[j], &message[length]);
					length += strlen(&message[length]);
					grid_ui_event_reset(&grid_ui_state.element[i].event_list[j]);
					
				}
				CRITICAL_SECTION_LEAVE()
				
			}
			
		}
			
		// Got messages
		if (packetvalid){
		
			//por->cooldown += (2+por->cooldown/2);
			por->cooldown += (10+por->cooldown);
		
			grid_sys_state.next_broadcast_message_id++;
		

			// Calculate packet length and insert it into the header! +1 is the EOT character
			uint8_t error = 0;
			grid_msg_set_parameter(message, GRID_BRC_LEN_offset, GRID_BRC_LEN_length, length+1, &error);

			// Close the packet
			sprintf(&message[length], "%c..\n", GRID_CONST_EOT);
			length += strlen(&message[length]);

			// Calculate checksum!
			uint8_t checksum = grid_msg_checksum_calculate(message, length);
			grid_msg_checksum_write(message, length, checksum);

		

			// Put the packet into the UI_RX buffer
			if (grid_buffer_write_init(&GRID_PORT_U.rx_buffer, length)){
	
				for(uint16_t i = 0; i<length; i++){
				
					grid_buffer_write_character(&GRID_PORT_U.rx_buffer, message[i]);
				}
			
				grid_buffer_write_acknowledge(&GRID_PORT_U.rx_buffer);

			}
			else{
				// LOG UNABLE TO WRITE EVENT
			}
		
		
		}
	
	}
	
}


void grid_ui_model_init(struct grid_ui_model* mod, uint8_t element_list_length){
	
	mod->status = GRID_UI_STATUS_INITIALIZED;
	
	mod->element_list_length = element_list_length;	
	mod->element = malloc(mod->element_list_length*sizeof(struct grid_ui_element));
	
	for(uint8_t i=0; i<element_list_length; i++){
		
		mod->element[i].status = GRID_UI_STATUS_UNDEFINED;		
		mod->element[i].event_list_length = 0;
		
	}
	
}

void grid_ui_event_init(struct grid_ui_event* eve, enum grid_ui_event_t event_type){
	
	eve->status = GRID_UI_STATUS_INITIALIZED;
	
	eve->type   = event_type;	
	eve->status = GRID_UI_EVENT_STATUS_READY;

	eve->action_string = malloc(GRID_UI_ACTION_STRING_LENGTH*sizeof(uint8_t));
	
	for (uint32_t i=0; i<GRID_UI_ACTION_STRING_LENGTH; i++){
		eve->action_string[i] = 0;
	}	
	
	eve->action_length = 0;
	
	eve->action_parameter_count = 0;
	
	eve->action_parameter_list = malloc(GRID_UI_ACTION_PARAMETER_COUNT*sizeof(struct grid_ui_action_parameter));

	for (uint32_t i=0; i<GRID_UI_ACTION_PARAMETER_COUNT; i++){
		eve->action_parameter_list[i].status = GRID_UI_STATUS_UNDEFINED;
		eve->action_parameter_list[i].address = 0;
		eve->action_parameter_list[i].offset = 0;
		eve->action_parameter_list[i].length = 0;
	}	
			
}




void grid_ui_element_init(struct grid_ui_element* ele, enum grid_ui_element_t element_type){

	ele->status = GRID_UI_STATUS_INITIALIZED;
	
	ele->type = element_type;
	
	
	ele->template_parameter_list = malloc(GRID_TEMPLATE_A_PARAMETER_LIST_LENGTH*sizeof(uint32_t));
	
	// initialize all of the template parameter values
	for(uint8_t i=0; i<GRID_TEMPLATE_A_PARAMETER_LIST_LENGTH; i++){
		ele->template_parameter_list[i] = 0;
	}

	
	if (element_type == GRID_UI_ELEMENT_SYSTEM){
		
		ele->event_list_length = 6;
		
		ele->event_list = malloc(ele->event_list_length*sizeof(struct grid_ui_event));	
		grid_ui_event_init(&ele->event_list[0], GRID_UI_EVENT_INIT); // Element Initialization Event
		grid_ui_event_init(&ele->event_list[1], GRID_UI_EVENT_HEARTBEAT); // Heartbeat
		grid_ui_event_init(&ele->event_list[2], GRID_UI_EVENT_MAPMODE_PRESS); // Mapmode press
		grid_ui_event_init(&ele->event_list[3], GRID_UI_EVENT_MAPMODE_RELEASE); // Mapmode release
		grid_ui_event_init(&ele->event_list[4], GRID_UI_EVENT_CFG_RESPONSE); // 
		grid_ui_event_init(&ele->event_list[5], GRID_UI_EVENT_CFG_REQUEST); //
		
	}
	else if (element_type == GRID_UI_ELEMENT_POTENTIOMETER){
		
		ele->event_list_length = 2;
		
		ele->event_list = malloc(ele->event_list_length*sizeof(struct grid_ui_event));
		
		grid_ui_event_init(&ele->event_list[0], GRID_UI_EVENT_INIT); // Element Initialization Event
		grid_ui_event_init(&ele->event_list[1], GRID_UI_EVENT_AVC7); // Absolute Value Change (7bit)
		
					
	}
	else if (element_type == GRID_UI_ELEMENT_BUTTON){
		
		ele->event_list_length = 3;
		
		ele->event_list = malloc(ele->event_list_length*sizeof(struct grid_ui_event));
		
		grid_ui_event_init(&ele->event_list[0], GRID_UI_EVENT_INIT); // Element Initialization Event
		grid_ui_event_init(&ele->event_list[1], GRID_UI_EVENT_DP);	// Press
		grid_ui_event_init(&ele->event_list[2], GRID_UI_EVENT_DR);	// Release
		
	}
	else if (element_type == GRID_UI_ELEMENT_ENCODER){
		
		ele->event_list_length = 4;
		
		ele->event_list = malloc(ele->event_list_length*sizeof(struct grid_ui_event));
		
		grid_ui_event_init(&ele->event_list[0], GRID_UI_EVENT_INIT); // Element Initialization Event
		grid_ui_event_init(&ele->event_list[1], GRID_UI_EVENT_DP);	// Press
		grid_ui_event_init(&ele->event_list[2], GRID_UI_EVENT_DR);	// Release
		grid_ui_event_init(&ele->event_list[3], GRID_UI_EVENT_AVC7); // Absolute Value Change (7bit)
		
	}
	else{
		//UNKNOWN ELEMENT TYPE
	}	
		
}



void grid_ui_event_register_action(struct grid_ui_element* ele, enum grid_ui_event_t event_type, uint8_t* event_string, uint32_t event_string_length){
		
	
	uint8_t event_index = 255;
	
	for(uint8_t i=0; i<ele->event_list_length; i++){
		if (ele->event_list[i].type == event_type){
			event_index = i;
		}
	}
	
	if (event_index == 255){
		return; // EVENT NOT FOUND
	}
	
	
	// TEMPLATE MAGIC COMING UP!
	
	uint8_t parameter_list_length = 0;
	struct grid_ui_action_parameter parameter_list[GRID_UI_ACTION_PARAMETER_COUNT];
	
	for (uint32_t i=0; i<event_string_length; i++){
		
		// if current character is A and the next character is a number from 0 to 9
		if (event_string[i] == 'A' && (event_string[i+1]-'0') < GRID_TEMPLATE_A_PARAMETER_LIST_LENGTH){
						
			parameter_list[parameter_list_length].status = GRID_UI_STATUS_INITIALIZED;
			
			parameter_list[parameter_list_length].group = event_string[i];
			
			parameter_list[parameter_list_length].address = (event_string[i+1]-'0');
			parameter_list[parameter_list_length].offset = i;
			parameter_list[parameter_list_length].length = 2;
			parameter_list_length++;
	
		}
		else if (event_string[i] == 'B' && (event_string[i+1]-'0') < GRID_TEMPLATE_B_PARAMETER_LIST_LENGTH){
			
			parameter_list[parameter_list_length].status = GRID_UI_STATUS_INITIALIZED;		
			
			parameter_list[parameter_list_length].group = event_string[i];
			
			parameter_list[parameter_list_length].address = (event_string[i+1]-'0');
			parameter_list[parameter_list_length].offset = i;
			parameter_list[parameter_list_length].length = 2;
			parameter_list_length++;
			
		}		
		
	}
	
	
	
	
	// COPY THE ACTION STRING
	for(uint32_t i=0; i<event_string_length; i++){
		ele->event_list[event_index].action_string[i] = event_string[i];
	}
	ele->event_list[event_index].action_length = event_string_length;
	
	

	// COPY THE PARAMETER DESCRIPTORS
	for(uint8_t i=0; i<parameter_list_length; i++){
		
		ele->event_list[event_index].action_parameter_list[i] = parameter_list[i];
	}
	ele->event_list[event_index].action_parameter_count = parameter_list_length;
	
	
	
}



uint8_t grid_ui_event_find(struct grid_ui_element* ele, enum grid_ui_event_t event_type){

	uint8_t event_index = 255;
		
	for(uint8_t i=0; i<ele->event_list_length; i++){
		if (ele->event_list[i].type == event_type){
			event_index = i;
		}
	}

		
		
	return event_index;
	
}

void grid_ui_event_trigger(struct grid_ui_event* eve){
		
	if (eve->action_status == GRID_UI_STATUS_UNDEFINED){
		return;
	}	
		
	eve->trigger = GRID_UI_EVENT_STATUS_TRIGGERED;
	
	//grid_sys_alert_set_alert(&grid_sys_state, 50,50,50,2,350);

}


void grid_ui_event_reset(struct grid_ui_event* eve){
	
	eve->trigger = GRID_UI_EVENT_STATUS_READY;
}

uint8_t grid_ui_event_istriggered(struct grid_ui_event* eve){
		
		
	if (eve->trigger == GRID_UI_EVENT_STATUS_TRIGGERED){
		
					
		return 1;
				
	}
	else{
		
		return 0;
	}
			
}

uint8_t grid_ui_event_render_action(struct grid_ui_event* eve, uint8_t* target_string){
	
	for(uint8_t i=0; i<eve->action_length; i++){
		target_string[i] = eve->action_string[i];
	}
	
	return eve->action_length;
		
}

uint8_t grid_ui_event_template_action(struct grid_ui_element* ele, uint8_t event_index){
	
	if (event_index == 255){
				
		return;
	}
	
	for (uint8_t i=0; i<ele->event_list[event_index].action_parameter_count; i++){
		
		uint8_t* message = ele->event_list[event_index].action_string;
		
		if (ele->event_list[event_index].action_parameter_list[i].group == 'A'){
			
			uint32_t parameter_value =  ele->template_parameter_list[ele->event_list[event_index].action_parameter_list[i].address];
			uint8_t parameter_offset = ele->event_list[event_index].action_parameter_list[i].offset;
			uint8_t parameter_length = ele->event_list[event_index].action_parameter_list[i].length;
					
			uint8_t error = 0;
			grid_msg_set_parameter(message, parameter_offset, parameter_length, parameter_value, &error);
					
			//ele->event[event_index].action_string		
		}
		else if (ele->event_list[event_index].action_parameter_list[i].group == 'B'){

			uint32_t parameter_value = 0;
			uint8_t parameter_offset = ele->event_list[event_index].action_parameter_list[i].offset;
			uint8_t parameter_length = ele->event_list[event_index].action_parameter_list[i].length;

			if (ele->event_list[event_index].action_parameter_list[i].address == GRID_TEMPLATE_B_PARAMETER_BANK_NUMBER_ACTIVE){
				parameter_value = grid_sys_get_bank_num(&grid_sys_state);
			}
			else if (ele->event_list[event_index].action_parameter_list[i].address == GRID_TEMPLATE_B_PARAMETER_BANK_COLOR_RED){
				parameter_value = grid_sys_get_bank_red(&grid_sys_state);
			}
			else if (ele->event_list[event_index].action_parameter_list[i].address == GRID_TEMPLATE_B_PARAMETER_BANK_COLOR_GRE){
				parameter_value = grid_sys_get_bank_gre(&grid_sys_state);
			}
			else if (ele->event_list[event_index].action_parameter_list[i].address == GRID_TEMPLATE_B_PARAMETER_BANK_COLOR_BLU){
				parameter_value = grid_sys_get_bank_blu(&grid_sys_state);
			}
			else if (ele->event_list[event_index].action_parameter_list[i].address == GRID_TEMPLATE_B_PARAMETER_MAPMODE_STATE){
				parameter_value = grid_sys_state.mapmodestate;
			}
			else if (ele->event_list[event_index].action_parameter_list[i].address == GRID_TEMPLATE_B_PARAMETER_BANK_NEXT){
				parameter_value = grid_sys_get_bank_next(&grid_sys_state);
			}
			
			uint8_t error = 0;
			grid_msg_set_parameter(message, parameter_offset, parameter_length, parameter_value, &error);

		}
		

		
	}
	
	
	
	
	
}

