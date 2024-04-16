#pragma once

#ifndef GRID_UI_ENCODER_H_INCLUDED
#define GRID_UI_ENCODER_H_INCLUDED


#include <stdint.h>
#include "grid_ui.h"

void grid_ui_element_encoder_init(struct grid_ui_element* ele);
void grid_ui_element_encoder_template_parameter_init(struct grid_ui_template_buffer* buf);

void grid_ui_element_encoder_event_clear_cb(struct grid_ui_event* eve);
void grid_ui_element_encoder_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new);

void grid_ui_encoder_store_input(uint8_t input_channel, uint64_t* encoder_last_real_time, uint64_t* button_last_real_time, uint8_t old_value, uint8_t new_value, uint8_t* phase_change_lock);

#endif
