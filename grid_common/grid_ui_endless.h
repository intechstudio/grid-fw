#pragma once

#ifndef GRID_UI_ENDLESS_H_INCLUDED
#define GRID_UI_ENDLESS_H_INCLUDED


#include <stdint.h>
#include "grid_ui.h"

struct grid_module_endlesspot_state {

  uint16_t phase_a_value;
  uint16_t phase_b_value;
  uint16_t button_value;
  uint16_t knob_angle;
};

void grid_ui_element_endlesspot_init(struct grid_ui_element* ele);
void grid_ui_element_endlesspot_template_parameter_init(struct grid_ui_template_buffer* buf);

void grid_ui_element_endlesspot_event_clear_cb(struct grid_ui_event* eve);
void grid_ui_element_endlesspot_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new);

void grid_ui_endlesspot_store_input(uint8_t input_channel, uint64_t* encoder_last_real_time, struct grid_module_endlesspot_state* old_value, struct grid_module_endlesspot_state* new_value,
                                    uint8_t adc_bit_depth);


#endif
