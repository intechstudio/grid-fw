#ifndef GRID_MODULE_PO16_REVB_H_INCLUDED
#define GRID_MODULE_PO16_REVB_H_INCLUDED

#include "grid_module.h"


volatile uint8_t grid_module_po16_revb_hardware_transfer_complete;
volatile uint8_t grid_module_po16_revb_mux_lookup[16];
volatile uint8_t grid_module_po16_revb_mux;



void grid_module_po16_event_clear_cb(struct grid_ui_event* eve);
void grid_module_po16_page_change_cb(uint8_t page_old, uint8_t page_new);

void grid_module_po16_revb_hardware_start_transfer(void);
static void grid_module_po16_revb_hardware_transfer_complete_cb(void);
void grid_module_po16_revb_hardware_init(void);

void grid_module_po16_revb_init(void);



#endif