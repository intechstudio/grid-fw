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

#define GRID_NVM_BLOCK_SIZE 				8192

#define GRID_NVM_PAGE_SIZE					512
#define GRID_NVM_PAGE_OFFSET				0x200


// align local base address to start at a block start!!!
#define GRID_NVM_LOCAL_BASE_ADDRESS			0x80000
#define GRID_NVM_LOCAL_END_ADDRESS			0x100000
#define GRID_NVM_LOCAL_PAGE_COUNT			((GRID_NVM_LOCAL_END_ADDRESS - GRID_NVM_LOCAL_BASE_ADDRESS)/GRID_NVM_PAGE_OFFSET)

// 1024 page * 512byte/page = 0.5MB


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



struct grid_nvm_toc_entry{

	uint8_t status;
	uint8_t page_id;
	uint8_t element_id;
	uint8_t event_type;

	uint16_t config_string_length;
	uint32_t config_string_offset;

	struct grid_nvm_toc_entry* next;
	struct grid_nvm_toc_entry* prev;

};


struct grid_nvm_model{

	struct flash_descriptor* flash;

	uint32_t next_write_offset;


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

	struct grid_nvm_toc_entry* toc_head;
	uint16_t toc_count;

};

uint8_t grid_nvm_toc_entry_create(struct grid_nvm_model* mod, uint8_t page_id, uint8_t element_id, uint8_t event_type, uint32_t config_string_offset, uint16_t config_string_length);
struct grid_nvm_toc_entry* grid_nvm_toc_entry_find(struct grid_nvm_model* mod, uint8_t page_id, uint8_t element_id, uint8_t event_type);
uint8_t grid_nvm_toc_entry_update(struct grid_nvm_toc_entry* entry, uint32_t config_string_offset, uint16_t config_string_length);

uint8_t grid_nvm_toc_entry_destroy(struct grid_nvm_model* mod, uint8_t page_id, uint8_t element_id, uint8_t event_type);


uint32_t grid_nvm_toc_defragmant(struct grid_nvm_model* mod);


uint32_t grid_nvm_config_mock(struct grid_nvm_model* mod);
uint32_t grid_nvm_config_store(struct grid_nvm_model* mod, uint8_t page_number, uint8_t element_number, uint8_t event_type, uint8_t* config_buffer, uint16_t config_length);

uint32_t grid_nvm_append(struct grid_nvm_model* mod, uint8_t* buffer, uint16_t length);
uint32_t grid_nvm_clear(struct grid_nvm_model* mod, uint32_t offset, uint16_t length);

uint32_t grid_nvm_erase_all(struct grid_nvm_model* mod);

void grid_nvm_toc_debug(struct grid_nvm_model* mod);


struct grid_nvm_model grid_nvm_state;


void grid_nvm_init(struct grid_nvm_model* mod, struct flash_descriptor* flash_instance);



void grid_nvm_toc_init(struct grid_nvm_model* mod);



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