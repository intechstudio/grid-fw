#include "grid_ui_endless.h"

#include <stdint.h>
#include "grid_ui.h"
#include "grid_ain.h"
#include "grid_lua_api.h"
#include "grid_protocol.h"
#include <stdlib.h>
#include <string.h>


const char grid_ui_endless_init_actionstring[] = GRID_ACTIONSTRING_INIT_EPOT;
const char grid_ui_endless_endlesschange_actionstring[] = GRID_ACTIONSTRING_EPOTC;
const char grid_ui_endless_buttonchange_actionstring[] = GRID_ACTIONSTRING_EPOTC_BC;
const char grid_ui_endless_timer_actionstring[] = GRID_ACTIONSTRING_TIMER;

void grid_ui_element_endlesspot_init(struct grid_ui_element* ele){

  ele->type = GRID_UI_ELEMENT_ENDLESSPOT;

  ele->event_list_length = 4;

  ele->event_list = malloc(ele->event_list_length * sizeof(struct grid_ui_event));

  grid_ui_event_init(ele, 0, GRID_UI_EVENT_INIT, GRID_LUA_FNC_ACTION_INIT_short, grid_ui_endless_init_actionstring);  // Element Initialization Event
  grid_ui_event_init(ele, 1, GRID_UI_EVENT_EPOTC, GRID_LUA_FNC_ACTION_ENDLESSPOTCHANGE_short, grid_ui_endless_endlesschange_actionstring); // Endlesspot Change
  grid_ui_event_init(ele, 2, GRID_UI_EVENT_BC, GRID_LUA_FNC_ACTION_BUTTONCHANGE_short, grid_ui_endless_buttonchange_actionstring);    // Button Change
  grid_ui_event_init(ele, 3, GRID_UI_EVENT_TIMER, GRID_LUA_FNC_ACTION_TIMER_short, grid_ui_endless_timer_actionstring);

  ele->template_initializer = &grid_ui_element_endlesspot_template_parameter_init;
  ele->template_parameter_list_length = GRID_LUA_FNC_EPOT_LIST_length;

  ele->event_clear_cb = &grid_ui_element_endlesspot_event_clear_cb;
  ele->page_change_cb = &grid_ui_element_endlesspot_page_change_cb;

}

void grid_ui_element_endlesspot_template_parameter_init(struct grid_ui_template_buffer* buf) {

  // printf("template parameter init\r\n");

  uint8_t element_index = buf->parent->index;
  int32_t* template_parameter_list = buf->template_parameter_list;

  template_parameter_list[GRID_LUA_FNC_EPOT_ELEMENT_INDEX_index] = element_index;
  template_parameter_list[GRID_LUA_FNC_EPOT_BUTTON_NUMBER_index] = element_index;
  template_parameter_list[GRID_LUA_FNC_EPOT_BUTTON_VALUE_index] = 0;
  template_parameter_list[GRID_LUA_FNC_EPOT_BUTTON_MIN_index] = 0;
  template_parameter_list[GRID_LUA_FNC_EPOT_BUTTON_MAX_index] = 127;
  template_parameter_list[GRID_LUA_FNC_EPOT_BUTTON_MODE_index] = 0;
  template_parameter_list[GRID_LUA_FNC_EPOT_BUTTON_ELAPSED_index] = 0;
  template_parameter_list[GRID_LUA_FNC_EPOT_BUTTON_STATE_index] = 0;

  template_parameter_list[GRID_LUA_FNC_EPOT_ENDLESSPOT_NUMBER_index] = element_index;
  template_parameter_list[GRID_LUA_FNC_EPOT_ENDLESSPOT_VALUE_index] = 0;
  template_parameter_list[GRID_LUA_FNC_EPOT_ENDLESSPOT_MIN_index] = 0;
  template_parameter_list[GRID_LUA_FNC_EPOT_ENDLESSPOT_MAX_index] = 128 - 1;
  template_parameter_list[GRID_LUA_FNC_EPOT_ENDLESSPOT_MODE_index] = 0;
  template_parameter_list[GRID_LUA_FNC_EPOT_ENDLESSPOT_ELAPSED_index] = 0;
  template_parameter_list[GRID_LUA_FNC_EPOT_ENDLESSPOT_STATE_index] = 64;
  template_parameter_list[GRID_LUA_FNC_EPOT_ENDLESSPOT_VELOCITY_index] = 50;
}

void grid_ui_element_endlesspot_event_clear_cb(struct grid_ui_event* eve) {

  int32_t* template_parameter_list = eve->parent->template_parameter_list;

  template_parameter_list[GRID_LUA_FNC_EPOT_ENDLESSPOT_STATE_index] = 64;

  if (template_parameter_list[GRID_LUA_FNC_EPOT_ENDLESSPOT_MODE_index] == 1) { // relative

    int32_t min = template_parameter_list[GRID_LUA_FNC_EPOT_ENDLESSPOT_MIN_index];
    int32_t max = template_parameter_list[GRID_LUA_FNC_EPOT_ENDLESSPOT_MAX_index];

    template_parameter_list[GRID_LUA_FNC_EPOT_ENDLESSPOT_VALUE_index] = ((max + 1) - min) / 2;
  } else if (template_parameter_list[GRID_LUA_FNC_EPOT_ENDLESSPOT_MODE_index] == 2) {

    template_parameter_list[GRID_LUA_FNC_EPOT_ENDLESSPOT_VALUE_index] = 0;
  }
}

void grid_ui_element_endlesspot_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new) {

  // for (uint8_t i = 0; i<16; i++)
  // {

  // 	struct grid_ui_event* eve = NULL;

  // 	eve = grid_ui_event_find(&grid_ui_state.element_list[i],
  // GRID_UI_EVENT_INIT); 	grid_ui_event_trigger_local(eve);

  // 	eve = grid_ui_event_find(&grid_ui_state.element_list[i],
  // GRID_UI_EVENT_EC); 	grid_ui_event_trigger_local(eve);
  // }
}

static uint8_t grid_ui_endlesspot_update_trigger(struct grid_ui_element* ele, uint64_t* endlesspot_last_real_time, int16_t delta, uint8_t is_endless_pot) {

  uint32_t encoder_elapsed_time = grid_platform_rtc_get_elapsed_time(*endlesspot_last_real_time);
  if (GRID_PARAMETER_ELAPSED_LIMIT * MS_TO_US < grid_platform_rtc_get_elapsed_time(*endlesspot_last_real_time)) {
    *endlesspot_last_real_time = grid_platform_rtc_get_micros() - GRID_PARAMETER_ELAPSED_LIMIT * MS_TO_US;
    encoder_elapsed_time = GRID_PARAMETER_ELAPSED_LIMIT * MS_TO_US;
  }

  if (delta == 0) {
    // nothing left to do
    return 0; // did not trigger
  }

  // update lastrealtime
  *endlesspot_last_real_time = grid_platform_rtc_get_micros();
  int32_t* template_parameter_list = ele->template_parameter_list;
  template_parameter_list[GRID_LUA_FNC_EPOT_ENDLESSPOT_ELAPSED_index] = encoder_elapsed_time / MS_TO_US;

  int32_t min = template_parameter_list[GRID_LUA_FNC_EPOT_ENDLESSPOT_MIN_index];
  int32_t max = template_parameter_list[GRID_LUA_FNC_EPOT_ENDLESSPOT_MAX_index];

  double elapsed_ms = encoder_elapsed_time / MS_TO_US;

  if (elapsed_ms > 25) {
    elapsed_ms = 25;
  }

  if (elapsed_ms < 1) {
    elapsed_ms = 1;
  }

  double minmaxscale = (max - min) / 128.0;

  double velocityparam = template_parameter_list[GRID_LUA_FNC_EPOT_ENDLESSPOT_VELOCITY_index] / 100.0;

  // implement configurable velocity parameters here
  double velocityfactor = ((25 * 25 - elapsed_ms * elapsed_ms) / 75.0) * minmaxscale * velocityparam + 1.0;

  if (is_endless_pot) {
    velocityfactor = minmaxscale * velocityparam / 15.0;
  }

  int32_t delta_velocity = delta * velocityfactor;

  int32_t old_value = template_parameter_list[GRID_LUA_FNC_EPOT_ENDLESSPOT_VALUE_index];

  if (is_endless_pot) {

    if (delta_velocity == 0) {

      return 0; // did not trigger
    }
  } else {
    template_parameter_list[GRID_LUA_FNC_EPOT_ENDLESSPOT_STATE_index] += delta_velocity;
  }

  if (template_parameter_list[GRID_LUA_FNC_EPOT_ENDLESSPOT_MODE_index] == 0) { // Absolute

    int32_t new_value = 0;

    if (old_value + delta_velocity < min) {
      new_value = min;
    } else if (old_value + delta_velocity > max) {
      new_value = max;
    } else {
      new_value = old_value + delta_velocity;
    }

    template_parameter_list[GRID_LUA_FNC_EPOT_ENDLESSPOT_VALUE_index] = new_value;
  } else if (template_parameter_list[GRID_LUA_FNC_EPOT_ENDLESSPOT_MODE_index] == 1) { // Relative

    int32_t new_value = 0;

    if (old_value + delta_velocity < min) {
      new_value = min;
    } else if (old_value + delta_velocity > max) {
      new_value = max;
    } else {
      new_value = old_value + delta_velocity;
    }

    template_parameter_list[GRID_LUA_FNC_EPOT_ENDLESSPOT_VALUE_index] = new_value;
  } else if (template_parameter_list[GRID_LUA_FNC_EPOT_ENDLESSPOT_MODE_index] == 2) { // Relative 2's complement

    // Two's complement magic 7 bit signed variable

    int32_t old_twoscomplement = template_parameter_list[GRID_LUA_FNC_EPOT_ENDLESSPOT_VALUE_index];

    uint8_t old_8bit_extended_twoscomplement = old_twoscomplement;

    // Limit to signed -64 +63 range
    if (old_twoscomplement > 127) {
      old_8bit_extended_twoscomplement = 127;
    }
    if (old_twoscomplement < 0) {
      old_8bit_extended_twoscomplement = 0;
    }

    if (old_twoscomplement > 63) { // extend sign bit to 8 bit size
      old_8bit_extended_twoscomplement += 128;
    }

    int8_t old_signed;

    if (old_8bit_extended_twoscomplement > 127) { // negative number
      old_signed = -((~old_8bit_extended_twoscomplement) + 1 + 256);
    } else { // positive number
      old_signed = -(~old_8bit_extended_twoscomplement) - 1;
    }

    int16_t new_signed = old_signed - delta_velocity;

    // Limit to signed -64 +63 range
    if (new_signed < -64) {
      new_signed = -64;
    }

    if (new_signed > 63) {
      new_signed = 63;
    }

    int8_t new_signed_8bit = new_signed;

    // Two's complement magic
    uint8_t new_twoscomplement = (~new_signed_8bit) + 1;

    // reduce the result to 7 bit length
    uint8_t new_7bit_twoscomplement = new_twoscomplement & 127;

    template_parameter_list[GRID_LUA_FNC_EPOT_ENDLESSPOT_VALUE_index] = new_7bit_twoscomplement;
  }

  struct grid_ui_event* eve = grid_ui_event_find(ele, GRID_UI_EVENT_EPOTC);

  if (grid_ui_state.ui_interaction_enabled) {
    grid_ui_event_trigger(eve);
  }

  return 1; // did trigger
}

static uint16_t grid_ui_endlesspot_calculate_angle(uint16_t phase_a, uint16_t phase_b, uint8_t adc_bit_depth) {

  uint16_t value_degrees = 0;

  // calculate absolute angle based on phase_a and phase_b
  // .....

  double phase_a_norm = (double)phase_a / ((1 << adc_bit_depth) - 1);
  double phase_b_norm = (double)phase_b / ((1 << adc_bit_depth) - 1);

  uint16_t phase_a_degrees, phase_b_degrees;

  // Calculate rotation based on phase A
  if (phase_b_norm > 0.5) {
    phase_a_degrees = phase_a_norm * 1800;
  } else {
    phase_a_degrees = (1800 - phase_a_norm * 1800) + 1800;
  }

  // Calculate rotation based on phase B
  if (phase_a_norm < 0.5) {
    phase_b_degrees = (uint16_t)(phase_b_norm * 1800 + 2700) % 3600;
  } else {
    phase_b_degrees = (uint16_t)((1800 - phase_b_norm * 1800) + 1800 + 2700) % 3600;
  }

  // if one of the phasees are close to 0 and the other is close to 360 then
  // averaging will not work directly
  if (phase_b_degrees > 3000 && phase_a_degrees < 600) {
    phase_a_degrees += 3600;
  } else if (phase_a_degrees > 3000 && phase_b_degrees < 600) {
    phase_b_degrees += 3600;
  }

  // Average the two to eliminate deadzones
  double weight_a = (phase_b_norm - 0.5) * 2;

  if (weight_a < 0) {
    weight_a = -weight_a;
  }

  double weight_b = 1 - weight_a;
  double range_calibration = 1 + (1 / 3600.0);

  value_degrees = (phase_a_degrees * weight_a + phase_b_degrees * weight_b) * range_calibration;

  if (value_degrees > 3599)
    value_degrees = 3599;

  value_degrees = 3600 - value_degrees;

  // ENDLESS POT ROTATION
  return value_degrees;
}

void grid_ui_endlesspot_store_input(uint8_t input_channel, uint64_t* endlesspot_last_real_time, struct grid_module_endlesspot_state* old_value, struct grid_module_endlesspot_state* new_value,
                                    uint8_t adc_bit_depth) {

  if (!memcmp(old_value, new_value, sizeof(struct grid_module_endlesspot_state))) {
    // no change
    return;
  }

  struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, input_channel);
  int32_t* template_parameter_list = ele->template_parameter_list;

  uint16_t value_degrees_new = grid_ui_endlesspot_calculate_angle(new_value->phase_a_value, new_value->phase_b_value, 12);
  uint16_t value_degrees_old = grid_ui_endlesspot_calculate_angle(old_value->phase_a_value, old_value->phase_b_value, 12);

  int32_t resolution = 9;

  if (resolution < 1) {
    resolution = 1;
  } else if (resolution > 12) {
    resolution = 12;
  }

  grid_ain_add_sample(&grid_ain_state, input_channel, value_degrees_new, 12, (uint8_t)resolution);

  if (grid_ain_get_changed(&grid_ain_state, input_channel)) {

    int16_t delta = (value_degrees_new - value_degrees_old);

    if (delta < -1800) {
      delta += 3600;
    } else if (delta > 1800) {
      delta -= 3600;
    }

    if (abs(delta) > 10) {

      template_parameter_list[GRID_LUA_FNC_EPOT_ENDLESSPOT_STATE_index] = value_degrees_new / 20;
      uint8_t has_triggered = grid_ui_endlesspot_update_trigger(ele, endlesspot_last_real_time, delta, 1);

      if (has_triggered) {
        old_value->phase_a_value = new_value->phase_a_value;
        old_value->phase_b_value = new_value->phase_b_value;
        old_value->knob_angle = new_value->knob_angle;
      }
    }
  }
}
