#pragma once

#ifndef GRID_UI_POTMETER_H_INCLUDED
#define GRID_UI_POTMETER_H_INCLUDED


#include <stdint.h>
#include "grid_ui.h"

void grid_ui_element_potentiometer_init(struct grid_ui_element* ele);
void grid_ui_element_potmeter_template_parameter_init(struct grid_ui_template_buffer* buf);

void grid_ui_element_potmeter_event_clear_cb(struct grid_ui_event* eve);
void grid_ui_element_potmeter_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new);

void grid_ui_potmeter_store_input(uint8_t input_channel, uint64_t* last_real_time, uint16_t value, uint8_t adc_bit_depth);


#endif
