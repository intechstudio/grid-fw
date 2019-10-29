#ifndef GRID_MODULE_PBF4_REVA_H_INCLUDED
#define GRID_MODULE_PBF4_REVA_H_INCLUDED

#include "grid_module.h"


volatile static uint8_t grid_module_hardware_transfer_complete;



static const uint8_t grid_module_mux_lookup[];



static uint8_t grid_module_mux;


static void grid_module_hardware_start_transfer(void);
static void grid_module_hardware_transfer_complete_cb(void);
static void grid_module_hardware_init(void);

void grid_module_pbf4_revb_init(struct grid_ui_model* mod);



#endif