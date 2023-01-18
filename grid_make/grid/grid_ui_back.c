#include "grid_ui_back.h"






void grid_port_process_ui_local(struct grid_ui_model* ui){


		
	// Prepare packet header LOCAL
	struct grid_msg_packet message_local;
	grid_msg_packet_init(&grid_msg_state, &message_local, GRID_PARAMETER_DEFAULT_POSITION, GRID_PARAMETER_DEFAULT_POSITION);
	uint8_t payload_local[GRID_PARAMETER_PACKET_maxlength] = {0};				
	uint32_t offset_local=0;		

	// Prepare packet header GLOBAL
	struct grid_msg_packet message_global;
	grid_msg_packet_init(&grid_msg_state, &message_global, GRID_PARAMETER_DEFAULT_POSITION, GRID_PARAMETER_DEFAULT_POSITION);
	uint8_t payload_global[GRID_PARAMETER_PACKET_maxlength] = {0};				
	uint32_t offset_global=0;
	
	
	
	// UI STATE

	for (uint8_t j=0; j<ui->element_list_length; j++){
		
		for (uint8_t k=0; k<ui->element_list[j].event_list_length; k++){
		
			if (offset_local>GRID_PARAMETER_PACKET_marign || offset_global>GRID_PARAMETER_PACKET_marign){
				continue;
			}		
			else{
				
				CRITICAL_SECTION_ENTER()
				if (grid_ui_event_istriggered_local(&ui->element_list[j].event_list[k])){
					
					offset_local += grid_ui_event_render_action(&ui->element_list[j].event_list[k], &payload_local[offset_local]);
					grid_ui_event_reset(&ui->element_list[j].event_list[k]);


					// automatically report elementname after config
					if (j<ui->element_list_length-1){
						

							uint8_t number = j;
							uint8_t command[20] = {0};

							sprintf(command, "gens(%d,ele[%d]:gen())", j, j);

							// lua get element name
							grid_lua_clear_stdo(&grid_lua_state);
							grid_lua_dostring(&grid_lua_state, command);
							strcat(payload_global, grid_lua_get_output_string(&grid_lua_state));
							grid_lua_clear_stdo(&grid_lua_state);

					}
					

				
				}
				CRITICAL_SECTION_LEAVE()
				
			}
		
		}
		
		

		
	}

	if (strlen(payload_global)>0){

		grid_msg_packet_body_append_text(&message_global, payload_global);
		grid_msg_packet_close(&grid_msg_state, &message_global);
		grid_port_packet_send_everywhere(&message_global);
	}

	
	grid_msg_packet_body_append_text(&message_local, payload_local);


	grid_msg_packet_close(&grid_msg_state, &message_local);
		
	uint32_t message_length = grid_msg_packet_get_length(&message_local);
		
	// Put the packet into the UI_TX buffer
	if (grid_buffer_write_init(&ui->port->tx_buffer, message_length)){
			
		for(uint32_t i = 0; i<message_length; i++){
				
			grid_buffer_write_character(&ui->port->tx_buffer, grid_msg_packet_send_char_by_char(&message_local, i));
		}
			
		grid_buffer_write_acknowledge(&ui->port->tx_buffer);
		
// 			uint8_t debug_string[200] = {0};
// 			sprintf(debug_string, "Space: RX: %d/%d  TX: %d/%d", grid_buffer_get_space(&ui->port->rx_buffer), GRID_BUFFER_SIZE, grid_buffer_get_space(&ui->port->tx_buffer), GRID_BUFFER_SIZE);
// 			grid_port_debug_print_text(debug_string);


	}
	else{
		// LOG UNABLE TO WRITE EVENT
	}
		





}


void grid_port_process_ui(struct grid_ui_model* ui){
	


	struct grid_msg_packet message;
	grid_msg_packet_init(&grid_msg_state, &message, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);
	

	for (uint8_t j=0; j<ui->element_list_length; j++){
	
		for (uint8_t k=0; k<ui->element_list[j].event_list_length; k++){ //j=1 because init is local
		
			if (grid_msg_packet_get_length(&message)>GRID_PARAMETER_PACKET_marign){
				continue;
			}		
			else{
							
				if (grid_ui_event_istriggered(&ui->element_list[j].event_list[k])){

					uint32_t offset = grid_msg_packet_body_get_length(&message); 

					message.body_length += grid_ui_event_render_event(&ui->element_list[j].event_list[k], &message.body[offset]);
				
					offset = grid_msg_packet_body_get_length(&message); 

					CRITICAL_SECTION_ENTER()
					message.body_length += grid_ui_event_render_action(&ui->element_list[j].event_list[k], &message.body[offset]);
					grid_ui_event_reset(&ui->element_list[j].event_list[k]);
					CRITICAL_SECTION_LEAVE()
					

				}
				
			}
		
		}
	}
	

	

	grid_msg_packet_close(&grid_msg_state, &message);
	uint32_t length = grid_msg_packet_get_length(&message);
	

	// Put the packet into the UI_RX buffer
	if (grid_buffer_write_init(&ui->port->rx_buffer, length)){

		for(uint16_t i = 0; i<length; i++){
			
			grid_buffer_write_character(&ui->port->rx_buffer, grid_msg_packet_send_char_by_char(&message, i));
		}
		
		grid_buffer_write_acknowledge(&ui->port->rx_buffer);

		
	}
	else{
		// LOG UNABLE TO WRITE EVENT
	}

	

	// LEDREPORT
	if (grid_protocol_led_change_report_length(&grid_led_state) && grid_sys_get_editor_connected_state(&grid_sys_state)){

		struct grid_msg_packet response;
								
		grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

		uint8_t response_payload[300] = {0};
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







	
}








// ==================== BULK OPERATIONS ======================== //

