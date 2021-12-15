#ifndef GRID_MODULE_EN16_H_INCLUDED
#define GRID_MODULE_EN16_H_INCLUDED




#include "grid_module.h"



struct grid_ui_encoder{
	
	uint8_t controller_number;
	
	uint8_t button_value;
	uint8_t button_changed;
	
	uint8_t rotation_value;
	uint8_t rotation_changed;
	
	uint8_t rotation_direction;

	uint8_t velocity;
	
	uint8_t phase_a_previous;
	uint8_t phase_b_previous;
    
    uint8_t phase_change_lock;
	
};


 
static void grid_module_en16_hardware_start_transfer(void);
static void grid_module_en16_hardware_transfer_complete_cb(void);
static void grid_module_en16_hardware_init(void);

void grid_module_en16_init(void);



#endif