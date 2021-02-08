#ifndef GRID_MODULE_BU16_REVB_H_INCLUDED
#define GRID_MODULE_BU16_REVB_H_INCLUDED

#include "grid_module.h"


volatile uint8_t grid_module_bu16_revb_hardware_transfer_complete;

volatile uint8_t grid_module_bu16_revb_mux_lookup[16];

volatile uint8_t	grid_module_bu16_revb_mux;

void grid_module_bu16_revb_hardware_start_transfer(void);
static void grid_module_bu16_revb_hardware_transfer_complete_cb(void);
void grid_module_bu16_revb_hardware_init(void);

void grid_module_bu16_revb_init(void);



/*void test_init(struct grid_ui_model* mod);*/



#endif