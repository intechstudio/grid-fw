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
	
	
	grid_nvm_clear_read_buffer(mod);
	grid_nvm_clear_write_buffer(mod);

}

void grid_nvm_clear_read_buffer(struct grid_nvm_model* mod){
	
	for (uint32_t i=0; i<GRID_NVM_PAGE_SIZE; i++){
		
		mod->read_buffer[i] = 0;
		
	}

	mod->read_buffer_status = GRID_NVM_BUFFER_STATUS_EMPTY;
	mod->read_buffer_length = 0;
	mod->read_source_address = -1;
	
}

void grid_nvm_clear_write_buffer(struct grid_nvm_model* mod){
	
	for (uint32_t i=0; i<GRID_NVM_PAGE_SIZE; i++){
		
		mod->write_buffer[i] = 0;
		
	}
	
	mod->write_buffer_status = GRID_NVM_BUFFER_STATUS_EMPTY;
	mod->write_buffer_length = 0;
	mod->write_target_address = -1;
}


void grid_nvm_read_element_cfg(struct grid_nvm_model* mod, uint8_t bank_number, uint8_t element_number, uint8_t event_number);