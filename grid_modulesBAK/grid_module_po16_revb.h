#ifndef GRID_MODULE_PO16_REVB_H_INCLUDED
#define GRID_MODULE_PO16_REVB_H_INCLUDED

#include "sam.h"
#include <stdlib.h>

#include "../../grid_lib/grid_ui.h"
#include "../../grid_modules/grid_module.h"


volatile static uint8_t grid_ui_button_hardware_transfer_complete;



static const uint8_t grid_module_mux_lookup[];



static uint8_t grid_module_mux;


static void grid_ui_button_hardware_start_transfer(void);
static void grid_ui_button_hardware_transfer_complete_cb(void);
static void grid_ui_button_hardware_init(void);

void grid_module_po16_revb_init(struct grid_ui_model* mod);



#endif