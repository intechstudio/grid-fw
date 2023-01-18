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




void	grid_ui_bulk_nvmdefrag_init(struct grid_d51_nvm_model* nvm, void (*success_cb)());
uint8_t grid_ui_bulk_nvmdefrag_is_in_progress(struct grid_d51_nvm_model* nvm);
void	grid_ui_bulk_nvmdefrag_next(struct grid_d51_nvm_model* nvm);






#endif