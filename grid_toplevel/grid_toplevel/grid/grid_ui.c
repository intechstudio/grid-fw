#include "grid_ui.h"



void grid_port_process_ui(struct grid_port* por){
	
	//por->cooldown = 0;
	if (por->cooldown > 0){
		por->cooldown--;
		return;
	}
	
	uint8_t message[256];
	uint32_t length=0;
	
	
	uint8_t len = 0;
	uint8_t id = grid_sys_state.next_broadcast_message_id;
	uint8_t dx = GRID_SYS_DEFAULT_POSITION;
	uint8_t dy = GRID_SYS_DEFAULT_POSITION;
	uint8_t age = 0;
	
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
		if (grid_ui_report_get_changed_flag(&grid_ui_state, i)){
			
			packetvalid++;
			grid_ui_report_render(&grid_ui_state, i, &message[length]);
			grid_ui_report_clear_changed_flag(&grid_ui_state, i);
			length += strlen(&message[length]);
					
		}
		CRITICAL_SECTION_LEAVE()
	}
	
	if (packetvalid){
		
		por->cooldown = packetvalid;
		
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
	
	mod->report_array = malloc(len*sizeof(struct grid_ui_report));
	mod->report_length = len;
	
}

uint8_t grid_ui_report_init(struct grid_ui_model* mod, uint8_t index, uint8_t* p, uint8_t p_len, uint8_t* h, uint8_t h_len){

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

uint8_t grid_ui_report_render(struct grid_ui_model* mod, uint8_t index, uint8_t* target){
	
	struct grid_ui_report* rep = &mod->report_array[index];
	
	for(uint8_t i=0; i<rep->payload_length; i++){
		target[i] = rep->payload[i];
	}
	
	return rep->payload_length;
}

uint8_t grid_ui_report_get_changed_flag(struct grid_ui_model* mod, uint8_t index){
	
	return mod->report_array[index].changed;
}

void grid_ui_report_set_changed_flag(struct grid_ui_model* mod, uint8_t index){
	
	mod->report_array[index].changed = 1;
}

void grid_ui_report_clear_changed_flag(struct grid_ui_model* mod, uint8_t index){
	
	mod->report_array[index].changed = 0;
}

