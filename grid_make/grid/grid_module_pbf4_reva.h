#ifndef GRID_MODULE_PBF4_REVA_H_INCLUDED
#define GRID_MODULE_PBF4_REVA_H_INCLUDED

#include "grid_module.h"


volatile uint8_t grid_module_pbf4_reva_hardware_transfer_complete;
volatile uint8_t grid_module_pbf4_reva_mux_lookup[];
volatile uint8_t grid_module_pbf4_reva_mux;

void grid_module_pbf4_event_clear_cb(struct grid_ui_event* eve);
void grid_module_pbf4_page_change_cb(uint8_t page_old, uint8_t page_new);

void grid_module_pbf4_reva_hardware_start_transfer(void);
void grid_module_pbf4_reva_hardware_transfer_complete_cb(void);
void grid_module_pbf4_reva_hardware_init(void);

void grid_module_pbf4_reva_init(void);



#endif