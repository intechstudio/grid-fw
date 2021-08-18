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
#define GRID_NVM_LOCAL_BASE_ADDRESS			0x90000
#define GRID_NVM_LOCAL_END_ADDRESS			0x100000
#define GRID_NVM_LOCAL_PAGE_COUNT			((GRID_NVM_LOCAL_END_ADDRESS - GRID_NVM_LOCAL_BASE_ADDRESS)/GRID_NVM_PAGE_OFFSET)

// 1024 page * 512byte/page = 0.5MB



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

	uint8_t status;


	uint8_t read_bulk_status;
	uint8_t read_bulk_last_element;
	uint8_t read_bulk_last_event;
	
	uint8_t erase_bulk_status;
	uint32_t erase_bulk_address;
    
	uint8_t store_bulk_status;

	uint8_t clear_bulk_status;


	uint8_t bulk_nvmdefrag_status;
	uint8_t bulk_nvmdefrag_stage; // 0: move, 1: erase
	

	uint32_t next_write_offset;
	struct grid_nvm_toc_entry* toc_head;
	uint16_t toc_count;

};


void grid_nvm_toc_init(struct grid_nvm_model* mod);
uint8_t grid_nvm_toc_entry_create(struct grid_nvm_model* mod, uint8_t page_id, uint8_t element_id, uint8_t event_type, uint32_t config_string_offset, uint16_t config_string_length);
struct grid_nvm_toc_entry* grid_nvm_toc_entry_find(struct grid_nvm_model* mod, uint8_t page_id, uint8_t element_id, uint8_t event_type);
uint8_t grid_nvm_toc_entry_update(struct grid_nvm_toc_entry* entry, uint32_t config_string_offset, uint16_t config_string_length);

uint8_t grid_nvm_toc_entry_destroy(struct grid_nvm_model* nvm, struct grid_nvm_toc_entry* entry);

uint32_t grid_nvm_toc_generate_actionstring(struct grid_nvm_model* nvm, struct grid_nvm_toc_entry* entry, uint8_t* targetstring);

uint32_t grid_nvm_toc_defragment(struct grid_nvm_model* nvm);


uint32_t grid_nvm_config_mock(struct grid_nvm_model* mod);
uint32_t grid_nvm_config_store(struct grid_nvm_model* mod, uint8_t page_number, uint8_t element_number, uint8_t event_type, uint8_t* config_buffer);

uint32_t grid_nvm_append(struct grid_nvm_model* mod, uint8_t* buffer, uint16_t length);
uint32_t grid_nvm_clear(struct grid_nvm_model* mod, uint32_t offset, uint16_t length);

uint32_t grid_nvm_erase_all(struct grid_nvm_model* mod);

void grid_nvm_toc_debug(struct grid_nvm_model* mod);


struct grid_nvm_model grid_nvm_state;


void grid_nvm_init(struct grid_nvm_model* mod, struct flash_descriptor* flash_instance);


void grid_nvm_toc_init(struct grid_nvm_model* mod);

uint8_t grid_nvm_is_ready(struct grid_nvm_model* nvm);
uint8_t grid_nvm_ui_bluk_anything_is_in_progress(struct grid_nvm_model* nvm, struct grid_ui_model* ui);

void	grid_nvm_ui_bulk_pageread_init(struct grid_nvm_model* nvm, struct grid_ui_model* ui);
uint8_t grid_nvm_ui_bulk_pageread_is_in_progress(struct grid_nvm_model* nvm, struct grid_ui_model* ui);
void	grid_nvm_ui_bulk_pageread_next(struct grid_nvm_model* nvm, struct grid_ui_model* ui);

void	grid_nvm_ui_bulk_nvmerase_init(struct grid_nvm_model* nvm, struct grid_ui_model* ui);
uint8_t grid_nvm_ui_bulk_nvmerase_is_in_progress(struct grid_nvm_model* nvm, struct grid_ui_model* ui);
void	grid_nvm_ui_bulk_nvmerase_next(struct grid_nvm_model* nvm, struct grid_ui_model* ui);

void	grid_nvm_ui_bulk_pagestore_init(struct grid_nvm_model* nvm, struct grid_ui_model* ui);
uint8_t grid_nvm_ui_bulk_pagestore_is_in_progress(struct grid_nvm_model* nvm, struct grid_ui_model* ui);
void	grid_nvm_ui_bulk_pagestore_next(struct grid_nvm_model* nvm, struct grid_ui_model* ui);

void	grid_nvm_ui_bulk_pageclear_init(struct grid_nvm_model* nvm, struct grid_ui_model* ui);
uint8_t grid_nvm_ui_bulk_pageclear_is_in_progress(struct grid_nvm_model* nvm, struct grid_ui_model* ui);
void	grid_nvm_ui_bulk_pageclear_next(struct grid_nvm_model* nvm, struct grid_ui_model* ui);

void	grid_nvm_ui_bulk_nvmdefrag_init(struct grid_nvm_model* nvm);
uint8_t grid_nvm_ui_bulk_nvmdefrag_is_in_progress(struct grid_nvm_model* nvm);
void	grid_nvm_ui_bulk_nvmdefrag_next(struct grid_nvm_model* nvm);



#endif /* GRID_NVM_H_ */