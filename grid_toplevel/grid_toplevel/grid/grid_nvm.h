/*
 * grid_nvm.h
 *
 * Created: 9/15/2020 3:41:53 PM
 *  Author: suku
 */ 


#ifndef GRID_NVM_H_
#define GRID_NVM_H_

#include "grid_module.h"


#define GRID_NVM_GLOBAL_BASE_ADDRESS		0x7F000

#define GRID_NVM_ELEMENT_BASE_ADDRESS		0x80000

#define GRID_NVM_PAGE_SIZE					512
#define GRID_NVM_PAGE_OFFSET				0x200


#define GRID_NVM_STRATEGY_BANK_OFFSET
#define GRID_NVM_STRATEGY_ELEMENT_OFFSET
#define GRID_NVM_STRATEGY_EVENT_OFFSET
#define GRID_NVM_STRATEGY_BANK_OFFSET




enum grid_nvm_buffer_status_t{

	GRID_NVM_BUFFER_STATUS_UNINITIALIZED,
	GRID_NVM_BUFFER_STATUS_INITIALIZED,
	GRID_NVM_BUFFER_STATUS_EMPTY,
	GRID_NVM_BUFFER_STATUS_DIRTY,
	GRID_NVM_BUFFER_STATUS_DONE,
		
};

struct grid_nvm_model{

	struct flash_descriptor* flash;

	uint32_t bank_settings_page_address;

	uint8_t status;
	
	uint8_t read_buffer[GRID_NVM_PAGE_SIZE];
	uint32_t read_buffer_length;
	enum grid_nvm_buffer_status_t read_buffer_status;
	uint32_t read_source_address;
	
	uint8_t write_buffer[GRID_NVM_PAGE_SIZE];
	uint32_t write_buffer_length;
	enum grid_nvm_buffer_status_t write_buffer_status;
	uint32_t write_target_address;



};

struct grid_nvm_model grid_nvm_state;


void grid_nvm_init(struct grid_nvm_model* mod, struct flash_descriptor* flash_instance);

void grid_nvm_read_uicfg(struct grid_nvm_model* mod, uint8_t bank_number, uint8_t element_number, uint8_t event_number);

void grid_nvm_clear_read_buffer(struct grid_nvm_model* mod);
void grid_nvm_clear_write_buffer(struct grid_nvm_model* mod);


#endif /* GRID_NVM_H_ */