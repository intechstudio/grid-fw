#ifndef GRID_MODULE_BU16_H_INCLUDED
#define GRID_MODULE_BU16_H_INCLUDED

#include "grid_d51_module.h"


volatile uint8_t grid_module_bu16_hardware_transfer_complete;

volatile uint8_t grid_module_bu16_mux_lookup[16];

volatile uint8_t	grid_module_bu16_mux;

void grid_module_bu16_hardware_start_transfer(void);
static void grid_module_bu16_hardware_transfer_complete_cb(void);
void grid_module_bu16_hardware_init(void);

void grid_module_bu16_init(void);



/*void test_init(struct grid_ui_model* mod);*/



#endif