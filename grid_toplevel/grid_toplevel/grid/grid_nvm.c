/*
 * grid_nvm.c
 *
 * Created: 9/15/2020 3:41:44 PM
 *  Author: suku
 */ 


#include "grid_nvm.h"


void grid_nvm_init(struct grid_nvm_model* mod, struct flash_descriptor* flash_instance){
	
	mod->bank_settings_page_address = GRID_NVM_GLOBAL_BASE_ADDRESS;
	
	mod->flash = flash_instance;
	
	mod->status = 1;
	mod->read_buffer_status = GRID_NVM_BUFFER_STATUS_UNINITIALIZED;
	mod->write_buffer_status = GRID_NVM_BUFFER_STATUS_UNINITIALIZED;
	
	
	mod->read_bulk_bank_index = 0;
	mod->read_bulk_element_index = 0;
	mod->read_bulk_event_index = 0;
	mod->read_bulk_status = 0;
	
	
	grid_nvm_clear_read_buffer(mod);
	grid_nvm_clear_write_buffer(mod);

}

void grid_nvm_read_ui_bulk_next(struct grid_nvm_model* nvm, struct grid_ui_model* ui){
	
	if (nvm->read_bulk_status == 1){
	
		//if(ui->bank_list[nvm->read_bulk_bank_index].element_list[nvm->read_bulk_element_index].event_list_length<)	
		
		
	}
	
	
	
	
}


void grid_nvm_clear_read_buffer(struct grid_nvm_model* mod){
	
	for (uint32_t i=0; i<GRID_NVM_PAGE_SIZE; i++){
		
		mod->read_buffer[i] = 255;
		
	}

	mod->read_buffer_status = GRID_NVM_BUFFER_STATUS_EMPTY;
	mod->read_buffer_length = 0;
	mod->read_source_address = -1;
	
}

void grid_nvm_clear_write_buffer(struct grid_nvm_model* mod){
	
	for (uint32_t i=0; i<GRID_NVM_PAGE_SIZE; i++){
		
		mod->write_buffer[i] = 255;
		
	}
	
	mod->write_buffer_status = GRID_NVM_BUFFER_STATUS_EMPTY;
	mod->write_buffer_length = 0;
	mod->write_target_address = -1;
}


uint32_t grid_nvm_calculate_event_page_offset(struct grid_nvm_model* nvm, uint8_t bank_number, uint8_t element_number, uint8_t event_number){
	
	
	
	
	return GRID_NVM_STRATEGY_BANK_size * bank_number + GRID_NVM_STRATEGY_ELEMENT_size * element_number + GRID_NVM_STRATEGY_EVENT_size * event_number;
	
}