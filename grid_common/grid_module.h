#pragma once

#ifndef GRID_MODULE_H_INCLUDED
#define GRID_MODULE_H_INCLUDED

// only for uint definitions
#include  <stdint.h>
// only for malloc
#include  <stdlib.h>


#include "grid_protocol.h"
#include "grid_ain.h"
#include "grid_led.h"
#include "grid_ui.h"

struct grid_ui_encoder{
	
	uint8_t controller_number;
	
	uint8_t button_value;
	
	uint8_t rotation_value;
	uint8_t rotation_changed;
	
	uint8_t rotation_direction;

	uint8_t velocity;
	
	uint8_t phase_a_previous;
	uint8_t phase_b_previous;
    
    uint8_t phase_change_lock;
	
};

void grid_module_po16_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui);
void grid_module_bu16_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui);
void grid_module_pbf4_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui);
void grid_module_en16_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui);


void grid_ui_potmeter_store_input(uint8_t input_channel, uint32_t* last_real_time, uint16_t value, uint8_t adc_bit_depth);
void grid_ui_button_store_input(uint8_t input_channel, uint32_t* last_real_time, uint16_t value, uint8_t adc_bit_depth);
void grid_ui_encoder_store_input(uint8_t input_channel, uint32_t* encoder_last_real_time, uint32_t* button_last_real_time, uint8_t old_value, uint8_t new_value, uint8_t* phase_change_lock);


#endif