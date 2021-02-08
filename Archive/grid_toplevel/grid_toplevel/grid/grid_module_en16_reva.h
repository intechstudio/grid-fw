#ifndef GRID_MODULE_EN16_REVA_H_INCLUDED
#define GRID_MODULE_EN16_REVA_H_INCLUDED




#include "grid_module.h"



struct grid_ui_encoder{
	
	uint8_t controller_number;
	
	uint8_t button_value;
	uint8_t button_changed;
	
	uint8_t rotation_value;
	uint8_t rotation_changed;
	
	uint8_t rotation_direction;
	
	uint32_t last_real_time;
	
	uint8_t velocity;
	
	uint8_t phase_a_previous;
	uint8_t phase_b_previous;
    
    uint8_t phase_change_lock;
	
};

struct grid_ui_encoder grid_ui_encoder_array[16];

uint8_t UI_SPI_TX_BUFFER[14];
uint8_t UI_SPI_RX_BUFFER[14];
uint8_t UI_SPI_TRANSFER_LENGTH;

uint8_t UI_SPI_DEBUG;

volatile uint8_t UI_SPI_DONE;


volatile uint8_t UI_SPI_RX_BUFFER_LAST[16];


uint8_t UI_ENCODER_LOOKUP[16];




struct io_descriptor *grid_module_en16_reva_hardware_io;


volatile uint8_t grid_module_en16_reva_hardware_transfer_complete;

void grid_module_en16_reva_hardware_start_transfer(void);
void grid_module_en16_reva_hardware_transfer_complete_cb(void);
void grid_module_en16_reva_hardware_init(void);

void grid_module_en16_reva_init(void);



#endif