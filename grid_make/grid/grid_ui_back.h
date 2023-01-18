#ifndef GRID_UI_BACK_H_INCLUDED
#define GRID_UI_BACK_H_INCLUDED

#include "grid_module.h"

#include "grid_ui.h"

#include <stdint.h>
#include <stdlib.h>

extern void grid_platform_printf(char const *fmt, ...);










void grid_port_process_ui(struct grid_ui_model* ui);
void grid_port_process_ui_local(struct grid_ui_model* ui);






// requires NVM
uint8_t grid_ui_page_load(struct grid_ui_model* ui, uint8_t page);



void grid_ui_page_load_success_callback(void);

uint8_t grid_ui_bluk_anything_is_in_progress(struct grid_ui_model* ui);

void	grid_ui_bulk_pageread_init(struct grid_ui_model* ui, void (*success_cb)());
// grid_ui_bluk_pageread_is_in_progress method is already migrated
void	grid_ui_bulk_pageread_next(struct grid_ui_model* ui);

void	grid_ui_bulk_pagestore_init(struct grid_ui_model* ui, void (*success_cb)());
uint8_t grid_ui_bulk_pagestore_is_in_progress(struct grid_ui_model* ui);
void	grid_ui_bulk_pagestore_next(struct grid_ui_model* ui);

void	grid_ui_bulk_pageclear_init(struct grid_ui_model* ui, void (*success_cb)());
uint8_t grid_ui_bulk_pageclear_is_in_progress(struct grid_ui_model* ui);
void	grid_ui_bulk_pageclear_next(struct grid_ui_model* ui);

void	grid_ui_bulk_nvmerase_init(struct grid_ui_model* ui, void (*success_cb)());
uint8_t grid_ui_bulk_nvmerase_is_in_progress(struct grid_ui_model* ui);
void	grid_ui_bulk_nvmerase_next(struct grid_ui_model* ui);


void	grid_ui_bulk_nvmdefrag_init(struct grid_d51_nvm_model* nvm, void (*success_cb)());
uint8_t grid_ui_bulk_nvmdefrag_is_in_progress(struct grid_d51_nvm_model* nvm);
void	grid_ui_bulk_nvmdefrag_next(struct grid_d51_nvm_model* nvm);






#endif