#pragma once

#ifndef GRID_UI_BUTTON_H_INCLUDED
#define GRID_UI_BUTTON_H_INCLUDED


#include <stdint.h>
#include "grid_ui.h"

void grid_ui_element_button_init(struct grid_ui_element* ele);
void grid_ui_element_button_template_parameter_init(struct grid_ui_template_buffer* buf);

void grid_ui_element_button_event_clear_cb(struct grid_ui_event* eve);
void grid_ui_element_button_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new);

void grid_ui_button_update_trigger(struct grid_ui_element* ele, uint64_t* button_last_real_time, uint8_t old_button_value, uint8_t new_button_value);
void grid_ui_button_store_input(uint8_t input_channel, uint64_t* last_real_time, uint16_t value, uint8_t adc_bit_depth);

#endif
