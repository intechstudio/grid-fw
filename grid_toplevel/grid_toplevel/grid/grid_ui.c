#include "grid_ui.h"



void grid_port_process_ui(struct grid_port* por){
	
	//por->cooldown = 0;
	if (por->cooldown > 15){
		por->cooldown--;
		return;
	}
	else if (por->cooldown>0){
		
		por->cooldown--;
	}	

																
	uint8_t message[256];
	uint32_t length=0;
	
	
	uint8_t len = 0;
	uint8_t id = grid_sys_state.next_broadcast_message_id;
	uint8_t dx = GRID_SYS_DEFAULT_POSITION;
	uint8_t dy = GRID_SYS_DEFAULT_POSITION;
	uint8_t age = grid_sys_state.age;
	
	uint8_t packetvalid = 0;
	
	sprintf(&message[length],
	"%c%c%02x%02x%02x%02x%02x%c",
	GRID_MSG_START_OF_HEADING,
	GRID_MSG_BROADCAST,
	len, id, dx, dy, age,
	GRID_MSG_END_OF_BLOCK
	);
	
	length += strlen(&message[length]);
	
	// POTENTIOMETER/BUTTON READINGS FOR THE UI
	for (uint8_t i = 0; i<grid_ui_state.report_length; i++)
	{
		if (length>200){
			continue;
		}
		
		
		CRITICAL_SECTION_ENTER()
		if (grid_report_sys_get_changed_flag(&grid_ui_state, i)){
			
			packetvalid++;
			grid_report_render(&grid_ui_state, i, &message[length]);
			grid_report_sys_clear_changed_flag(&grid_ui_state, i);
			length += strlen(&message[length]);
					
		}
		CRITICAL_SECTION_LEAVE()
	}
	
	if (packetvalid){
		
		por->cooldown += (10+por->cooldown);
		
		grid_sys_state.next_broadcast_message_id++;
		
		// Close the packet
		sprintf(&message[length], "%c", GRID_MSG_END_OF_TRANSMISSION); // CALCULATE AND ADD CRC HERE
		length += strlen(&message[length]);
		
		// Calculate packet length and insert it into the header
		char length_string[8];
		sprintf(length_string, "%02x", length);
		
		message[2] = length_string[0];
		message[3] = length_string[1];
		
		
		// Add placeholder checksum and linebreak
		sprintf(&message[length], "00\n");
		length += strlen(&message[length]);
		
		uint8_t checksum = grid_msg_get_checksum(message, length);
		grid_msg_set_checksum(message, length, checksum);
		
		// Put the packet into the UI_RX buffer
		if (grid_buffer_write_init(&GRID_PORT_U.rx_buffer, length)){
			
			for(uint32_t i = 0; i<length; i++){
				
				grid_buffer_write_character(&GRID_PORT_U.rx_buffer, message[i]);
			}
			
			grid_buffer_write_acknowledge(&GRID_PORT_U.rx_buffer);
		}
		
		
	}
	
	
}

uint8_t grid_ui_model_init(struct grid_ui_model* mod, uint8_t len){
	
	
	mod->report_offset = 2; // System Reserved Report Elements
	
	mod->report_length = len + mod->report_offset;
	
	mod->report_array = malloc(mod->report_length*sizeof(struct grid_ui_report));
	
	mod->report_ui_array = &mod->report_array[mod->report_offset];
		
}






uint8_t grid_report_init(struct grid_ui_model* mod, uint8_t index, uint8_t* p, uint32_t p_len, uint8_t* h, uint32_t h_len){

	mod->report_array[index].changed = 0;
	mod->report_array[index].payload_length = p_len;
	mod->report_array[index].helper_length = h_len;
	
	mod->report_array[index].payload = malloc(p_len*sizeof(uint8_t));
	mod->report_array[index].helper = malloc(h_len*sizeof(uint8_t));
	
	
	if (mod->report_array[index].payload == NULL || mod->report_array[index].helper == NULL){
		return -1;
	}
	
	for (uint8_t i=0; i<mod->report_array[index].payload_length; i++){
		mod->report_array[index].payload[i] = p[i];
	}
	for (uint8_t i=0; i<mod->report_array[index].helper_length; i++){
		mod->report_array[index].helper[i] = h[i];
	}
	
	return 0;
	
}

uint8_t grid_report_ui_init(struct grid_ui_model* mod, uint8_t index, uint8_t* p, uint32_t p_len, uint8_t* h, uint32_t h_len){
	
	grid_report_init(mod, index+mod->report_offset, p, p_len, h, h_len);
}

uint8_t grid_report_sys_init(struct grid_ui_model* mod){
	
	for(uint8_t i=0; i<mod->report_offset; i++){
			
		uint8_t payload_template[30];
			
		if (i == 0){ // MAPMODE

			sprintf(payload_template, "%c%02x%02x%02x%02x%c",

			GRID_MSG_START_OF_TEXT,
			GRID_MSG_PROTOCOL_SYS,
			GRID_MSG_COMMAND_SYS_BANK,
			GRID_MSG_COMMAND_SYS_BANK_SELECT,
			0,
			GRID_MSG_END_OF_TEXT

			);

		}
		else{ // HEARTBEAT 	
			
			sprintf(payload_template, "%c%02x%02x%02x%02x%c",

			GRID_MSG_START_OF_TEXT,
			GRID_MSG_PROTOCOL_SYS,
			GRID_MSG_COMMAND_SYS_HEARTBEAT,
			GRID_MSG_COMMAND_SYS_HEARTBEAT_ALIVE,
			0,
			GRID_MSG_END_OF_TEXT

			);
			
		}
		
				
		uint8_t payload_length = strlen(payload_template);

		uint8_t helper_template[2];
		
		helper_template[0] = 0;
		helper_template[1] = 0;
		
		uint8_t helper_length = 2;
		
		uint8_t error = grid_report_init(mod, i, payload_template, payload_length, helper_template, helper_length);
		
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
