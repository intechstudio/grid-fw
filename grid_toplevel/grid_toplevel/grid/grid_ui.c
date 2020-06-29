#include "grid_ui.h"



void grid_port_process_ui(struct grid_port* por){
	
	
	struct grid_ui_model* mod = &grid_ui_state;
	
	
	// Priorities: Always process local, try to process direct, broadcast messages are last. 
	
	uint8_t message_local_available = 0;
	uint8_t message_direct_available = 0;
	uint8_t message_broadcast_available = 0;
	
	
	for (uint8_t i=0; i<grid_ui_state.report_length; i++){
		

		
		if (grid_report_sys_get_changed_flag(mod, i)){
			
			enum grid_report_type_t type = grid_report_get_type(mod, i);
			
			
			
			
			(type == GRID_REPORT_TYPE_BROADCAST)?message_broadcast_available++:1;	
			
			(type == GRID_REPORT_TYPE_DIRECT_ALL)?message_direct_available++:1;
			
			(type == GRID_REPORT_TYPE_DIRECT_NORTH)?message_direct_available++:1;
			(type == GRID_REPORT_TYPE_DIRECT_EAST)?message_direct_available++:1;
			(type == GRID_REPORT_TYPE_DIRECT_SOUTH)?message_direct_available++:1;
			(type == GRID_REPORT_TYPE_DIRECT_WEST)?message_direct_available++:1;
			
			(type == GRID_REPORT_TYPE_LOCAL)?message_local_available++:1;
				
			

						
		}
			
	}
	
	//DIRECT MESSAGES	
	if (message_direct_available){
		
		for (uint8_t i=0; i<grid_ui_state.report_length; i++){
			
			uint8_t changed = grid_report_sys_get_changed_flag(mod, i);
			enum grid_report_type_t type = grid_report_get_type(mod, i);
				
			if (changed && (type == GRID_REPORT_TYPE_DIRECT_ALL || type == GRID_REPORT_TYPE_DIRECT_NORTH || type == GRID_REPORT_TYPE_DIRECT_EAST || type == GRID_REPORT_TYPE_DIRECT_SOUTH || type == GRID_REPORT_TYPE_DIRECT_WEST)){
					
				uint8_t message[256] = {0};
				uint32_t length=0;
			
				CRITICAL_SECTION_ENTER()			
				
				grid_report_render(mod, i, &message[length]);
				length += strlen(&message[length]);
				
				CRITICAL_SECTION_LEAVE()			
			
				// FIND TARGET BUFFER
				struct grid_buffer* target_buffer;
				enum grid_report_type_t type = grid_report_get_type(mod, i);

				if (type == GRID_REPORT_TYPE_DIRECT_ALL){
					target_buffer = &GRID_PORT_U.rx_buffer;
				}
				else if (type == GRID_REPORT_TYPE_DIRECT_NORTH){
					target_buffer = &GRID_PORT_N.tx_buffer;
				}
				else if (type == GRID_REPORT_TYPE_DIRECT_EAST){
					target_buffer = &GRID_PORT_E.tx_buffer;
				}
				else if (type == GRID_REPORT_TYPE_DIRECT_SOUTH){
					target_buffer = &GRID_PORT_S.tx_buffer;
				}
				else if (type == GRID_REPORT_TYPE_DIRECT_WEST){
					target_buffer = &GRID_PORT_W.tx_buffer;
				}
			

				// TRY TO WRITE TO TARGET BUFFER
			
				if (grid_buffer_write_init(target_buffer, length)){
					//Success	
					grid_report_sys_clear_changed_flag(mod, i);
				
					for(uint32_t i = 0; i<length; i++){
					
						grid_buffer_write_character(target_buffer, message[i]);
					}
				
					grid_buffer_write_acknowledge(target_buffer);
				}
				else{
					//Fail
					
				}
			
			}
			
									
		}	
		

	}
	
	//LOCAL MESSAGES
	if (message_local_available && por->cooldown<20){

			

			
		// Prepare packet header
		uint8_t message[256] = {0};
		uint32_t length=0;
			
			
		uint8_t len = 0;
		uint8_t id = grid_sys_state.next_broadcast_message_id;
		uint8_t dx = GRID_SYS_DEFAULT_POSITION;
		uint8_t dy = GRID_SYS_DEFAULT_POSITION;
		uint8_t age = grid_sys_state.age;
		
		uint8_t rot = 0;
			
		uint8_t packetvalid = 0;
			
		sprintf(&message[length],
		"%c%c%02x%02x%02x%02x%02x%02x%02x%02x%02x%c",
		GRID_CONST_SOH,
		GRID_CONST_BROADCAST,
		len, id, dx, dy, age, rot,
		GRID_PROTOCOL_VERSION_MAJOR, GRID_PROTOCOL_VERSION_MINOR, GRID_PROTOCOL_VERSION_PATCH,
		GRID_CONST_EOB
		);
			
		length += strlen(&message[length]);
			

			
		// Append the UI change descriptors
		for (uint8_t i = 0; i<grid_ui_state.report_length; i++)
		{
				
			if (length>200){
				continue;
			}
				
				
			CRITICAL_SECTION_ENTER()
			if (grid_report_sys_get_changed_flag(mod, i) && grid_report_get_type(mod, i) == GRID_REPORT_TYPE_LOCAL){
					
				packetvalid++;
				grid_report_render(mod, i, &message[length]);
				grid_report_sys_clear_changed_flag(mod, i);
				length += strlen(&message[length]);
					
			}
			CRITICAL_SECTION_LEAVE()
		}
			
		// Got messages
		if (packetvalid){
				
			grid_sys_state.next_broadcast_message_id++;
				
			// Close the packet
			sprintf(&message[length], "%c", GRID_CONST_EOT); // CALCULATE AND ADD CRC HERE
			length += strlen(&message[length]);
				
			// Calculate packet length and insert it into the header
			char length_string[8];
			sprintf(length_string, "%02x", length);
				
			message[2] = length_string[0];
			message[3] = length_string[1];
				
				
			// Add placeholder checksum and linebreak
			sprintf(&message[length], "00\n");
			length += strlen(&message[length]);
				
			uint8_t checksum = grid_msg_checksum_calculate(message, length);
			grid_msg_checksum_write(message, length, checksum);
				
			// Put the packet into the UI_RX buffer
			if (grid_buffer_write_init(&GRID_PORT_U.tx_buffer, length)){
					
				for(uint32_t i = 0; i<length; i++){
						
					grid_buffer_write_character(&GRID_PORT_U.tx_buffer, message[i]);
				}
					
				grid_buffer_write_acknowledge(&GRID_PORT_U.tx_buffer);
			}
			else{
			}
				
				
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
	if (message_broadcast_available){

		

				
		// Prepare packet header
		uint8_t message[256] = {0};
		uint32_t length=0;
	
	
		uint8_t len = 0;
		uint8_t id = grid_sys_state.next_broadcast_message_id;
		uint8_t dx = GRID_SYS_DEFAULT_POSITION;
		uint8_t dy = GRID_SYS_DEFAULT_POSITION;
		uint8_t age = grid_sys_state.age;
		
		uint8_t rot = 0;
	
		uint8_t packetvalid = 0;
	
		sprintf(&message[length],
		"%c%c%02x%02x%02x%02x%02x%02x%02x%02x%02x%c",
		GRID_CONST_SOH,
		GRID_CONST_BROADCAST,
		len, id, dx, dy, age, rot,
		GRID_PROTOCOL_VERSION_MAJOR, GRID_PROTOCOL_VERSION_MINOR, GRID_PROTOCOL_VERSION_PATCH,
		GRID_CONST_EOB
		);
	
		length += strlen(&message[length]);
	

	
		// Append the UI change descriptors
		for (uint8_t i = 0; i<grid_ui_state.report_length; i++)
		{
				
			if (length>200){
				continue;
			}
		
		
			CRITICAL_SECTION_ENTER()
			if (grid_report_sys_get_changed_flag(mod, i) && grid_report_get_type(mod, i) == GRID_REPORT_TYPE_BROADCAST){
				
				packetvalid++;
				grid_report_render(mod, i, &message[length]);
				grid_report_sys_clear_changed_flag(mod, i);
				length += strlen(&message[length]);
					
			}
			CRITICAL_SECTION_LEAVE()
		}
	
		// Got messages
		if (packetvalid){
		
			//por->cooldown += (2+por->cooldown/2);
			por->cooldown += (10+por->cooldown);
		
			grid_sys_state.next_broadcast_message_id++;
		
			// Close the packet
			sprintf(&message[length], "%c", GRID_CONST_EOT); // CALCULATE AND ADD CRC HERE
			length += strlen(&message[length]);
		
			// Calculate packet length and insert it into the header
			char length_string[8];
			sprintf(length_string, "%02x", length);
		
			message[2] = length_string[0];
			message[3] = length_string[1];
		
		
			// Add placeholder checksum and linebreak
			sprintf(&message[length], "00\n");
			length += strlen(&message[length]);
		
			uint8_t checksum = grid_msg_checksum_calculate(message, length);
			grid_msg_checksum_write(message, length, checksum);
		
			// Put the packet into the UI_RX buffer
			if (grid_buffer_write_init(&GRID_PORT_U.rx_buffer, length)){
			
				for(uint32_t i = 0; i<length; i++){
				
					grid_buffer_write_character(&GRID_PORT_U.rx_buffer, message[i]);
				}
			
				grid_buffer_write_acknowledge(&GRID_PORT_U.rx_buffer);
			}
			else{
			}
		
		
		}
		
		
		

		
		
	}
	

	
	
}

uint8_t grid_ui_model_init(struct grid_ui_model* mod, uint8_t len){
	
	
	mod->report_offset = GRID_REPORT_OFFSET; // System Reserved Report Elements
	
	mod->report_length = len + mod->report_offset;
	
	mod->report_array = malloc(mod->report_length*sizeof(struct grid_ui_report));
	
	mod->report_ui_array = &mod->report_array[mod->report_offset];
		
}






uint8_t grid_report_init(struct grid_ui_model* mod, uint8_t index, enum grid_report_type_t type, uint8_t* p, uint32_t p_len, uint8_t* h, uint32_t h_len){

	mod->report_array[index].changed = 0;
	mod->report_array[index].type = type;
	
	mod->report_array[index].payload_length = p_len;
	mod->report_array[index].helper_length = h_len;
	
	mod->report_array[index].payload = malloc(p_len*sizeof(uint8_t));
	mod->report_array[index].helper = malloc(h_len*sizeof(uint8_t));
	
	
	if (mod->report_array[index].payload == NULL || mod->report_array[index].helper == NULL){
		return -1;
	}
	else{
	}
	
	for (uint8_t i=0; i<mod->report_array[index].payload_length; i++){
		mod->report_array[index].payload[i] = p[i];
	}
	for (uint8_t i=0; i<mod->report_array[index].helper_length; i++){
		mod->report_array[index].helper[i] = h[i];
	}
	
	return 0;
	
}

uint8_t grid_report_ui_init(struct grid_ui_model* mod, uint8_t index, enum grid_report_type_t type, uint8_t* p, uint32_t p_len, uint8_t* h, uint32_t h_len){
	
	grid_report_init(mod, index+mod->report_offset, type, p, p_len, h, h_len);
}

uint8_t grid_report_sys_init(struct grid_ui_model* mod){
		
	for(uint8_t i=0; i<mod->report_offset; i++){
			
		uint8_t payload_template[30] = {0};
		enum grid_report_type_t type = GRID_REPORT_TYPE_UNDEFINED;
			
		if (i == GRID_REPORT_INDEX_MAPMODE){ // MAPMODE
			
			type = GRID_REPORT_TYPE_BROADCAST;
			sprintf(payload_template, "%c%02x%02x%02x%02x%c", GRID_CONST_STX, GRID_CLASS_SYS, GRID_COMMAND_SYS_BANK,	GRID_PARAMETER_SYS_BANKSELECT, 0, GRID_CONST_ETX);

		}
		else if (i == GRID_REPORT_INDEX_CFG_REQUEST){ // CONFIGURATION REQUEST
			
			type = GRID_REPORT_TYPE_DIRECT_ALL;
			sprintf(payload_template, "%c%02x%02x%02x%c", GRID_CONST_STX, GRID_CLASS_SYS, GRID_COMMAND_SYS_CFG, GRID_PARAMETER_SYS_CFGREQUEST, GRID_CONST_ETX);
			
		}
		else if (i == GRID_REPORT_INDEX_HEARTBEAT){ // HEARTBEAT
			
			type = GRID_REPORT_TYPE_BROADCAST;
			sprintf(payload_template, "%c%02x%02x%02x%02x%c", GRID_CONST_STX, GRID_CLASS_SYS, GRID_COMMAND_SYS_HEARTBEAT, GRID_PARAMETER_SYS_HEARTBEATALIVE, grid_sys_get_hwcfg(), GRID_CONST_ETX);
			
		}
		else if (i == GRID_REPORT_INDEX_PING_NORTH){ // PING NORTH
		
			uint8_t direction = GRID_CONST_NORTH;
			
			type = GRID_REPORT_TYPE_DIRECT_NORTH;
			
			sprintf(payload_template, "%c%c%c%c%02x%02x%02x%c00\n", GRID_CONST_SOH, GRID_CONST_DIRECT, GRID_CONST_BELL, direction, grid_sys_get_hwcfg(), 255, 255, GRID_CONST_EOT);
			
			grid_msg_checksum_write(payload_template, strlen(payload_template), grid_msg_checksum_calculate(payload_template, strlen(payload_template)));
		
		}
		else if (i == GRID_REPORT_INDEX_PING_EAST){ // PING EAST 
			
			uint8_t direction = GRID_CONST_EAST;
			
			type = GRID_REPORT_TYPE_DIRECT_EAST;
			
			sprintf(payload_template, "%c%c%c%c%02x%02x%02x%c00\n", GRID_CONST_SOH, GRID_CONST_DIRECT, GRID_CONST_BELL, direction, grid_sys_get_hwcfg(), 255, 255, GRID_CONST_EOT);
			
			grid_msg_checksum_write(payload_template, strlen(payload_template), grid_msg_checksum_calculate(payload_template, strlen(payload_template)));
			
			
		}
		else if (i == GRID_REPORT_INDEX_PING_SOUTH){ // PING SOUTH
			
			uint8_t direction = GRID_CONST_SOUTH;
			
			type = GRID_REPORT_TYPE_DIRECT_SOUTH;
			
			sprintf(payload_template, "%c%c%c%c%02x%02x%02x%c00\n", GRID_CONST_SOH, GRID_CONST_DIRECT, GRID_CONST_BELL, direction, grid_sys_get_hwcfg(), 255, 255, GRID_CONST_EOT);
			
			grid_msg_checksum_write(payload_template, strlen(payload_template), grid_msg_checksum_calculate(payload_template, strlen(payload_template)));
			
			
		}
		else if (i == GRID_REPORT_INDEX_PING_WEST){ // PING WEST
			
			uint8_t direction = GRID_CONST_WEST;
			
			type = GRID_REPORT_TYPE_DIRECT_WEST;
			
			sprintf(payload_template, "%c%c%c%c%02x%02x%02x%c00\n", GRID_CONST_SOH, GRID_CONST_DIRECT, GRID_CONST_BELL, direction, grid_sys_get_hwcfg(), 255, 255, GRID_CONST_EOT);
			
			grid_msg_checksum_write(payload_template, strlen(payload_template), grid_msg_checksum_calculate(payload_template, strlen(payload_template)));
			
		}
		
		
		
				
		uint8_t payload_length = strlen(payload_template);

		uint8_t helper_template[2];
		
		helper_template[0] = 0;
		helper_template[1] = 0;
		
		uint8_t helper_length = 2;
		
	
		
		uint8_t error = grid_report_init(mod, i, type, payload_template, payload_length, helper_template, helper_length);
		
		if (error != 0){
			while(1){
				return;
			}
			
		}
	
	}
}




uint8_t grid_report_render(struct grid_ui_model* mod, uint8_t index, uint8_t* target){
	
	struct grid_ui_report* rep = &mod->report_array[index];
	
	for(uint8_t i=0; i<rep->payload_length; i++){
		target[i] = rep->payload[i];
	}
	
	return rep->payload_length;
}


enum grid_report_type_t grid_report_get_type(struct grid_ui_model* mod, uint8_t index){
		
	return mod->report_array[index].type;
		
};


// UI REPORT FLAGS

uint8_t grid_report_ui_get_changed_flag(struct grid_ui_model* mod, uint8_t index){
	
	return mod->report_array[index+mod->report_offset].changed;
}

void grid_report_ui_set_changed_flag(struct grid_ui_model* mod, uint8_t index){
	
	mod->report_array[index+mod->report_offset].changed = 1;
}

void grid_report_ui_clear_changed_flag(struct grid_ui_model* mod, uint8_t index){
	
	mod->report_array[index+mod->report_offset].changed = 0;
}

// SYS REPORT FLAGS

uint8_t grid_report_sys_get_changed_flag(struct grid_ui_model* mod, uint8_t index){
	
	return mod->report_array[index].changed;
}

void grid_report_sys_set_changed_flag(struct grid_ui_model* mod, uint8_t index){
	
	mod->report_array[index].changed = 1;
}

void grid_report_sys_clear_changed_flag(struct grid_ui_model* mod, uint8_t index){
	
	mod->report_array[index].changed = 0;
}
