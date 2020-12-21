/*
 * grid_nvm.c
 *
 * Created: 9/15/2020 3:41:44 PM
 *  Author: suku
 */ 


#include "grid_nvm.h"


void grid_nvm_init(struct grid_nvm_model* nvm, struct flash_descriptor* flash_instance){
	
	nvm->bank_settings_page_address = GRID_NVM_GLOBAL_BASE_ADDRESS;
	
	nvm->flash = flash_instance;
	
	nvm->status = 1;
	nvm->read_buffer_status = GRID_NVM_BUFFER_STATUS_UNINITIALIZED;
	nvm->write_buffer_status = GRID_NVM_BUFFER_STATUS_UNINITIALIZED;
	
	
	nvm->read_bulk_page_index = 0;
	nvm->read_bulk_status = 0;
	
	nvm->clear_bulk_page_index = 0;
	nvm->clear_bulk_status = 0;	
	
	nvm->write_bulk_page_index = 0;
	nvm->write_bulk_status = 0;
	
	
	grid_nvm_clear_read_buffer(nvm);
	grid_nvm_clear_write_buffer(nvm);

}


void grid_nvm_ui_bulk_read_init(struct grid_nvm_model* nvm, struct grid_ui_model* ui){

	nvm->read_bulk_page_index = 0;
	nvm->read_bulk_status = 1;
			
}

uint8_t grid_nvm_ui_bulk_read_is_in_progress(struct grid_nvm_model* nvm, struct grid_ui_model* ui){

	return nvm->read_bulk_status;
	
}

void grid_nvm_ui_bulk_read_next(struct grid_nvm_model* nvm, struct grid_ui_model* ui){
	
	if (nvm->read_bulk_status == 1){
		
		
		uint8_t bank    = (nvm->read_bulk_page_index/GRID_NVM_STRATEGY_EVENT_maxcount/GRID_NVM_STRATEGY_ELEMENT_maxcount)%GRID_NVM_STRATEGY_BANK_maxcount;
		uint8_t element = (nvm->read_bulk_page_index/GRID_NVM_STRATEGY_EVENT_maxcount)%GRID_NVM_STRATEGY_ELEMENT_maxcount;
		uint8_t event   = nvm->read_bulk_page_index%GRID_NVM_STRATEGY_EVENT_maxcount;
		
		
		if (bank < ui->bank_list_length){
			
			if (element < ui->bank_list[bank].element_list_length){
				
				if (event < ui->bank_list[bank].element_list[element].event_list_length){
					// Valid memory location
					
					int status = grid_ui_nvm_load_event_configuration(ui, nvm, &ui->bank_list[bank].element_list[element].event_list[event]);
							
					
							
// 					uint8_t debugtext[200] = {0};
// 					sprintf(debugtext, "Bulk Read Valid:: Status: %d, Index: %d => Bank: %d, Ele: %d, Eve: %d", status, nvm->read_bulk_page_index, bank, element, event);
// 					grid_debug_print_text(debugtext);
// 						
						
						
					
					
				}
				
			}
	
		}
		
		
		if (nvm->read_bulk_page_index < GRID_NVM_STRATEGY_EVENT_maxcount*GRID_NVM_STRATEGY_ELEMENT_maxcount*GRID_NVM_STRATEGY_BANK_maxcount-1){ // multiply with bankcount
			
			nvm->read_bulk_page_index++;
			
		}
		else{
			
			nvm->read_bulk_page_index = 0;
			nvm->read_bulk_status = 0;
			
			
			uint8_t acknowledge = 1;

			// Generate ACKNOWLEDGE RESPONSE
			struct grid_msg response;
				
			grid_msg_init(&response);
			grid_msg_init_header(&response, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_ROTATION);

			uint8_t response_payload[10] = {0};
			sprintf(response_payload, GRID_CLASS_LOCALLOAD_frame);

			grid_msg_body_append_text(&response, response_payload, strlen(response_payload));
				
			if (acknowledge == 1){
				grid_msg_text_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);
			}
			else{
				grid_msg_text_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_NACKNOWLEDGE_code);
			}

				
			grid_msg_packet_close(&response);
			grid_msg_packet_send_everywhere(&response);
				
			
		}
		
		
		
	}
	
	
	
	
}

void grid_nvm_ui_bulk_clear_init(struct grid_nvm_model* nvm, struct grid_ui_model* ui){

	nvm->clear_bulk_page_index = 0;
	nvm->clear_bulk_status = 1;
	
}

uint8_t grid_nvm_ui_bulk_clear_is_in_progress(struct grid_nvm_model* nvm, struct grid_ui_model* ui){

	return nvm->clear_bulk_status;
	
}

void grid_nvm_ui_bulk_clear_next(struct grid_nvm_model* nvm, struct grid_ui_model* ui){
	
	if (nvm->clear_bulk_status == 1){
		
		
		uint8_t bank    = (nvm->clear_bulk_page_index/GRID_NVM_STRATEGY_EVENT_maxcount/GRID_NVM_STRATEGY_ELEMENT_maxcount)%GRID_NVM_STRATEGY_BANK_maxcount;
		uint8_t element = (nvm->clear_bulk_page_index/GRID_NVM_STRATEGY_EVENT_maxcount)%GRID_NVM_STRATEGY_ELEMENT_maxcount;
		uint8_t event   = nvm->clear_bulk_page_index%GRID_NVM_STRATEGY_EVENT_maxcount;
		
		
		if (bank < ui->bank_list_length){
			
			if (element < ui->bank_list[bank].element_list_length){
				
				if (event < ui->bank_list[bank].element_list[element].event_list_length){
					// Valid memory location
					
					grid_ui_nvm_clear_event_configuration(ui, nvm, &ui->bank_list[bank].element_list[element].event_list[event]);		
				
				}
				
			}
	
		}
		
		
		
		if (nvm->clear_bulk_page_index < GRID_NVM_STRATEGY_EVENT_maxcount*GRID_NVM_STRATEGY_ELEMENT_maxcount*GRID_NVM_STRATEGY_BANK_maxcount-1){ // multiply with bankcount
			
			nvm->clear_bulk_page_index++;
			
		}
		else{
			
			nvm->clear_bulk_page_index = 0;
			nvm->clear_bulk_status = 0;
			
			
			uint8_t acknowledge = 1;

			// Generate ACKNOWLEDGE RESPONSE
			struct grid_msg response;
				
			grid_msg_init(&response);
			grid_msg_init_header(&response, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_ROTATION);

			uint8_t response_payload[10] = {0};
			sprintf(response_payload, GRID_CLASS_LOCALCLEAR_frame);

			grid_msg_body_append_text(&response, response_payload, strlen(response_payload));
				
			if (acknowledge == 1){
				grid_msg_text_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);
			}
			else{
				grid_msg_text_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_NACKNOWLEDGE_code);
			}

				
			grid_msg_packet_close(&response);
			grid_msg_packet_send_everywhere(&response);
						
		}
		
		
		
	}
	
	
	
	
}





void grid_nvm_clear_read_buffer(struct grid_nvm_model* mod){
	
	for (uint32_t i=0; i<GRID_NVM_PAGE_SIZE; i++){
		
		mod->read_buffer[i] = 255;
		
	}

	mod->read_buffer_status = GRID_NVM_BUFFER_STATUS_EMPTY;
	mod->read_buffer_length = 0;
	
}

void grid_nvm_clear_write_buffer(struct grid_nvm_model* mod){
	
	for (uint32_t i=0; i<GRID_NVM_PAGE_SIZE; i++){
		
		mod->write_buffer[i] = 255;
		
	}
	
	mod->write_buffer_status = GRID_NVM_BUFFER_STATUS_EMPTY;
	mod->write_buffer_length = 0;
	mod->write_target_address = -1;
}


uint32_t grid_nvm_calculate_event_page_offset(struct grid_nvm_model* nvm, struct grid_ui_event* eve){
	
	
	
	uint8_t bank_number		= eve->parent->parent->index;
	uint8_t element_number	= eve->parent->index;
	uint8_t event_number	= eve->index;

	return GRID_NVM_STRATEGY_BANK_size * bank_number + GRID_NVM_STRATEGY_ELEMENT_size * element_number + GRID_NVM_STRATEGY_EVENT_size * event_number;
	
}