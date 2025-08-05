/*
 * grid_d51_nvm.h
 *
 * Created: 9/15/2020 3:41:53 PM
 *  Author: suku
 */

#ifndef GRID_D51_NVM_H_
#define GRID_D51_NVM_H_

#include "grid_d51_littlefs.h"
#include "grid_d51_module.h"

#include "grid_ui.h"

#define GRID_D51_NVM_GLOBAL_BASE_ADDRESS 0x7F000

#define GRID_D51_NVM_BLOCK_SIZE 8192

#define GRID_D51_NVM_PAGE_SIZE 512
#define GRID_D51_NVM_PAGE_OFFSET 0x200

// align local base address to start at a block start!!!
#define GRID_D51_NVM_LOCAL_BASE_ADDRESS 0x90000
#define GRID_D51_NVM_LOCAL_END_ADDRESS 0x100000
#define GRID_D51_NVM_LOCAL_PAGE_COUNT ((GRID_D51_NVM_LOCAL_END_ADDRESS - GRID_D51_NVM_LOCAL_BASE_ADDRESS) / GRID_D51_NVM_PAGE_OFFSET)

// 1024 page * 512byte/page = 0.5MB

// TODO clean up this file
struct grid_d51_nvm_toc_entry {

  uint8_t status;
  uint8_t page_id;
  uint8_t element_id;
  uint8_t event_type;

  uint16_t config_string_length;
  uint32_t config_string_offset;

  struct grid_d51_nvm_toc_entry* next;
  struct grid_d51_nvm_toc_entry* prev;
};

struct grid_d51_nvm_model {
  struct d51_littlefs_t dfs;
};

void grid_d51_nvm_toc_init(struct grid_d51_nvm_model* nvm);
uint8_t grid_d51_nvm_toc_entry_create(struct grid_d51_nvm_model* nvm, uint8_t page_id, uint8_t element_id, uint8_t event_type, uint32_t config_string_offset, uint16_t config_string_length);

struct grid_d51_nvm_toc_entry* grid_d51_nvm_toc_entry_find_next_on_page(struct grid_d51_nvm_model* mod, uint8_t page_id, int* last_element, int* last_event);
struct grid_d51_nvm_toc_entry* grid_d51_nvm_toc_entry_find(struct grid_d51_nvm_model* nvm, uint8_t page_id, uint8_t element_id, uint8_t event_type);
uint8_t grid_d51_nvm_toc_entry_update(struct grid_d51_nvm_toc_entry* entry, uint32_t config_string_offset, uint16_t config_string_length);

// Destroy the TOC entry + append defaultconfiguration to NVM
uint8_t grid_d51_nvm_toc_entry_destroy(struct grid_d51_nvm_model* nvm, struct grid_d51_nvm_toc_entry* entry);

// Only remove the TOC entry from the list, do not touch NVM
uint8_t grid_d51_nvm_toc_entry_remove(struct grid_d51_nvm_model* nvm, struct grid_d51_nvm_toc_entry* entry);

uint32_t grid_d51_nvm_toc_generate_actionstring(struct grid_d51_nvm_model* nvm, struct grid_d51_nvm_toc_entry* entry, char* targetstring);

uint32_t grid_d51_nvm_toc_defragment(struct grid_d51_nvm_model* nvm);

uint32_t grid_d51_nvm_config_mock(struct grid_d51_nvm_model* mod);
uint32_t grid_d51_nvm_config_store(struct grid_d51_nvm_model* mod, uint8_t page_number, uint8_t element_number, uint8_t event_type, uint8_t* config_buffer);

uint32_t grid_d51_nvm_append(struct grid_d51_nvm_model* mod, uint8_t* buffer, uint16_t length);
uint32_t grid_d51_nvm_clear(struct grid_d51_nvm_model* mod, uint32_t offset, uint16_t length);

uint32_t grid_d51_nvm_erase_all(struct grid_d51_nvm_model* mod);

void grid_d51_nvm_toc_debug(struct grid_d51_nvm_model* mod);

extern struct grid_d51_nvm_model grid_d51_nvm_state;

void grid_d51_nvm_init(struct grid_d51_nvm_model* mod, struct flash_descriptor* flash_instance);

void grid_d51_nvm_toc_init(struct grid_d51_nvm_model* mod);

uint8_t grid_d51_nvm_is_ready(struct grid_d51_nvm_model* nvm);

#endif /* GRID_D51_NVM_H_ */
