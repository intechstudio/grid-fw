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


void grid_module_pbf4_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui);
void grid_module_pbf4_store_input(uint8_t input_channel, uint32_t* last_real_time, uint16_t value, uint8_t adc_bit_depth);


void grid_module_bu16_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui);
void grid_module_bu16_store_input(uint8_t input_channel, uint32_t* last_real_time, uint16_t value, uint8_t adc_bit_depth);

#endif