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


void grid_module_po16_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui);
void grid_module_bu16_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui);
void grid_module_pbf4_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui);
void grid_module_en16_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui);
void grid_module_ef44_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui);


void grid_ui_potmeter_store_input(uint8_t input_channel, uint64_t* last_real_time, uint16_t value, uint8_t adc_bit_depth);
void grid_ui_button_store_input(uint8_t input_channel, uint64_t* last_real_time, uint16_t value, uint8_t adc_bit_depth);
void grid_ui_encoder_store_input(uint8_t input_channel, uint64_t* encoder_last_real_time, uint64_t* button_last_real_time, uint8_t old_value, uint8_t new_value, uint8_t* phase_change_lock);

extern uint64_t grid_platform_rtc_get_elapsed_time(uint64_t told);


#endif