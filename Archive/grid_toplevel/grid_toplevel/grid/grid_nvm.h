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

#define GRID_NVM_LOCAL_BASE_ADDRESS		0x80000

#define GRID_NVM_PAGE_SIZE					512
#define GRID_NVM_PAGE_OFFSET				0x200

// Every event is stored in one page
#define GRID_NVM_STRATEGY_EVENT_maxcount	10

// Max 10 event in each element
#define GRID_NVM_STRATEGY_ELEMENT_maxcount	16

// Max 16 element in each bank
#define GRID_NVM_STRATEGY_BANK_maxcount		4


// Next event is 1 page away
#define GRID_NVM_STRATEGY_EVENT_size		1

// Next element is 10 page away
#define GRID_NVM_STRATEGY_ELEMENT_size		GRID_NVM_STRATEGY_EVENT_size * GRID_NVM_STRATEGY_EVENT_maxcount

// Next bank is 160 page away
#define GRID_NVM_STRATEGY_BANK_size			GRID_NVM_STRATEGY_ELEMENT_size * GRID_NVM_STRATEGY_ELEMENT_maxcount




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
	
	
	
	uint32_t read_bulk_page_index;
	uint8_t read_bulk_status;
	
	uint32_t clear_bulk_page_index;
	uint8_t clear_bulk_status;
    
	uint32_t store_bulk_page_index;
	uint8_t store_bulk_status;
	
	uint32_t write_bulk_page_index;
	uint8_t write_bulk_status;


};

struct grid_nvm_model grid_nvm_state;


void grid_nvm_init(struct grid_nvm_model* mod, struct flash_descriptor* flash_instance);



void	grid_nvm_ui_bulk_read_init(struct grid_nvm_model* nvm, struct grid_ui_model* ui);
uint8_t grid_nvm_ui_bulk_read_is_in_progress(struct grid_nvm_model* nvm, struct grid_ui_model* ui);
void	grid_nvm_ui_bulk_read_next(struct grid_nvm_model* nvm, struct grid_ui_model* ui);

void	grid_nvm_ui_bulk_clear_init(struct grid_nvm_model* nvm, struct grid_ui_model* ui);
uint8_t grid_nvm_ui_bulk_clear_is_in_progress(struct grid_nvm_model* nvm, struct grid_ui_model* ui);
void	grid_nvm_ui_bulk_clear_next(struct grid_nvm_model* nvm, struct grid_ui_model* ui);


void	grid_nvm_ui_bulk_store_init(struct grid_nvm_model* nvm, struct grid_ui_model* ui);
uint8_t grid_nvm_ui_bulk_store_is_in_progress(struct grid_nvm_model* nvm, struct grid_ui_model* ui);
void	grid_nvm_ui_bulk_store_next(struct grid_nvm_model* nvm, struct grid_ui_model* ui);


void grid_nvm_clear_read_buffer(struct grid_nvm_model* mod);
void grid_nvm_clear_write_buffer(struct grid_nvm_model* mod);


uint32_t grid_nvm_calculate_event_page_offset(struct grid_nvm_model* nvm, struct grid_ui_event* eve);


#endif /* GRID_NVM_H_ */