#include "grid_ui_endless.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "grid_ain.h"
#include "grid_math.h"
#include "grid_platform.h"
#include "grid_sys.h"
#include "grid_ui_system.h"

const char grid_ui_endless_init_actionstring[] = GRID_ACTIONSTRING_ENDLESS_INIT;
const char grid_ui_endless_endlesschange_actionstring[] = GRID_ACTIONSTRING_ENDLESS_ENDLESS;
const char grid_ui_endless_buttonchange_actionstring[] = GRID_ACTIONSTRING_ENDLESS_BUTTON;
const char grid_ui_endless_timer_actionstring[] = GRID_ACTIONSTRING_SYSTEM_TIMER;

void grid_ui_element_endless_init(struct grid_ui_element* ele) {

  ele->type = GRID_PARAMETER_ELEMENT_ENDLESS;

  grid_ui_element_malloc_events(ele, 4);

  grid_ui_event_init(ele, 0, GRID_PARAMETER_EVENT_INIT, GRID_LUA_FNC_A_INIT_short, grid_ui_endless_init_actionstring);                // Element Initialization Event
  grid_ui_event_init(ele, 1, GRID_PARAMETER_EVENT_ENDLESS, GRID_LUA_FNC_A_ENDLESS_short, grid_ui_endless_endlesschange_actionstring); // Endlesspot Change
  grid_ui_event_init(ele, 2, GRID_PARAMETER_EVENT_BUTTON, GRID_LUA_FNC_A_BUTTON_short, grid_ui_endless_buttonchange_actionstring);    // Button Change
  grid_ui_event_init(ele, 3, GRID_PARAMETER_EVENT_TIMER, GRID_LUA_FNC_A_TIMER_short, grid_ui_endless_timer_actionstring);

  ele->template_initializer = &grid_ui_element_endless_template_parameter_init;
  ele->template_parameter_list_length = GRID_LUA_FNC_EP_LIST_length;
  ele->template_parameter_element_position_index_1 = GRID_LUA_FNC_EP_BUTTON_STATE_index;
  ele->template_parameter_element_position_index_2 = GRID_LUA_FNC_EP_ENDLESS_DIRECTION_index;

  ele->template_parameter_index_value[0] = GRID_LUA_FNC_EP_BUTTON_VALUE_index;
  ele->template_parameter_index_min[0] = GRID_LUA_FNC_EP_BUTTON_MIN_index;
  ele->template_parameter_index_max[0] = GRID_LUA_FNC_EP_BUTTON_MAX_index;
  ele->template_parameter_index_value[1] = GRID_LUA_FNC_EP_ENDLESS_VALUE_index;
  ele->template_parameter_index_min[1] = GRID_LUA_FNC_EP_ENDLESS_MIN_index;
  ele->template_parameter_index_max[1] = GRID_LUA_FNC_EP_ENDLESS_MAX_index;

  ele->event_clear_cb = &grid_ui_element_endless_event_clear_cb;
  ele->page_change_cb = &grid_ui_element_endless_page_change_cb;
}

void grid_ui_element_endless_template_parameter_init(struct grid_ui_template_buffer* buf) {

  // printf("template parameter init\r\n");

  uint8_t element_index = buf->parent->index;
  int32_t* template_parameter_list = buf->template_parameter_list;

  template_parameter_list[GRID_LUA_FNC_EP_ELEMENT_INDEX_index] = element_index;
  template_parameter_list[GRID_LUA_FNC_EP_LED_INDEX_index] = element_index;
  template_parameter_list[GRID_LUA_FNC_EP_BUTTON_VALUE_index] = 0;
  template_parameter_list[GRID_LUA_FNC_EP_BUTTON_MIN_index] = 0;
  template_parameter_list[GRID_LUA_FNC_EP_BUTTON_MAX_index] = 127;
  template_parameter_list[GRID_LUA_FNC_EP_BUTTON_MODE_index] = 0;
  template_parameter_list[GRID_LUA_FNC_EP_BUTTON_ELAPSED_index] = 0;
  template_parameter_list[GRID_LUA_FNC_EP_BUTTON_STATE_index] = 0;

  template_parameter_list[GRID_LUA_FNC_EP_LED_OFFSET_index] = 2;
  template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_VALUE_index] = 0;
  template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_MIN_index] = 0;
  template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_MAX_index] = 16384 - 1;
  template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_MODE_index] = 0;
  template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_ELAPSED_index] = 0;
  template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_STATE_index] = 64;
  template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_VELOCITY_index] = 50;
  template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_DIRECTION_index] = -1;
  template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_SENSITIVITY_index] = 50;
}

void grid_ui_element_endless_event_clear_cb(struct grid_ui_event* eve) {

  int32_t* template_parameter_list = eve->parent->template_parameter_list;

  template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_STATE_index] = 64;

  if (template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_MODE_index] == 1) { // relative

    int32_t min = template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_MIN_index];
    int32_t max = template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_MAX_index];

    template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_VALUE_index] = ((max + 1) + min) / 2;
  } else if (template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_MODE_index] == 2) {

    template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_VALUE_index] = 0;
  }
}

void grid_ui_element_endless_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new) {

  // for (uint8_t i = 0; i<16; i++)
  // {

  // 	struct grid_ui_event* eve = NULL;

  // 	eve = grid_ui_event_find(&grid_ui_state.element_list[i],
  // GRID_PARAMETER_EVENT_INIT); 	grid_ui_event_trigger_local(eve);

  // 	eve = grid_ui_event_find(&grid_ui_state.element_list[i],
  // GRID_PARAMETER_EVENT_ENCODER); 	grid_ui_event_trigger_local(eve);
  // }
}

uint8_t grid_ui_endless_update_trigger(struct grid_ui_element* ele, int stabilized, int16_t delta, uint64_t* endless_last_real_time, double* delta_frac) {

  uint32_t encoder_elapsed_time = grid_platform_rtc_get_elapsed_time(*endless_last_real_time);
  if (GRID_PARAMETER_ELAPSED_LIMIT * MS_TO_US < grid_platform_rtc_get_elapsed_time(*endless_last_real_time)) {
    *endless_last_real_time = grid_platform_rtc_get_micros() - GRID_PARAMETER_ELAPSED_LIMIT * MS_TO_US;
    encoder_elapsed_time = GRID_PARAMETER_ELAPSED_LIMIT * MS_TO_US;
  }

  /*
  if (delta == 0) {
    // nothing left to do
    return 0; // did not trigger
  }
  */

  // update lastrealtime
  *endless_last_real_time = grid_platform_rtc_get_micros();
  int32_t* template_parameter_list = ele->template_parameter_list;
  template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_ELAPSED_index] = encoder_elapsed_time / MS_TO_US;

  int32_t tmin = template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_MIN_index];
  int32_t tmax = template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_MAX_index];
  int32_t min = MIN(tmin, tmax);
  int32_t max = MAX(tmin, tmax);

  // invert delta if necessary
  if (tmin > tmax) {
    delta = -delta;
  }

  double elapsed_ms = encoder_elapsed_time / MS_TO_US;
  elapsed_ms = clampf64(elapsed_ms, 1, 25);

  double minmaxscale = (max - min) / 3600.0;

  double vel_param = template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_VELOCITY_index] / 100.0;
  double sen_param = template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_SENSITIVITY_index] / 100.0;

  double rate_of_change = abs(delta) / elapsed_ms;
  double vel_comp = ((rate_of_change * rate_of_change * 28.125) / 2000.0) * vel_param;
  double sen_comp = sen_param;
  double factor = (vel_comp + sen_comp) * minmaxscale;

  double delta_full = delta * factor + *delta_frac;

  int32_t delta_velocity = ((delta_full > 0) * 2 - 1) * (int32_t)fabs(delta_full);

  *delta_frac = delta_full - delta_velocity;

  if (delta_velocity == 0) {
    return 0; // did not trigger
  }

  int32_t old_value = template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_VALUE_index];

  template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_STATE_index] += delta_velocity;

  if (template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_MODE_index] == 0) { // Absolute

    int32_t new_value = clampi32(old_value + delta_velocity, min, max);
    template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_VALUE_index] = new_value;

  } else if (template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_MODE_index] == 1) { // Relative

    int32_t new_value = clampi32(old_value + delta_velocity, min, max);
    template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_VALUE_index] = new_value;

  } else if (template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_MODE_index] == 2) { // Relative 2's complement

    // Two's complement magic 7 bit signed variable

    int32_t old_twoscomplement = template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_VALUE_index];

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

    template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_VALUE_index] = new_7bit_twoscomplement;
  }

  struct grid_ui_event* eve = grid_ui_event_find(ele, GRID_PARAMETER_EVENT_ENDLESS);

  if (stabilized) {
    grid_ui_event_state_set(eve, GRID_EVE_STATE_TRIG);
  }

  return 1;
}

static uint16_t grid_ui_endless_calculate_angle(uint16_t phase_a, uint16_t phase_b, uint8_t adc_bit_depth) {

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

void grid_ui_endless_store_input(struct grid_ui_element* ele, uint8_t input_channel, uint8_t adc_bit_depth, struct grid_ui_endless_state* new_value, struct grid_ui_endless_state* old_value) {

  assert(ele);

  if (!memcmp(old_value, new_value, sizeof(struct grid_ui_endless_state))) {
    // no change
    return;
  }

  int32_t* template_parameter_list = ele->template_parameter_list;

  int stabilized = grid_ain_stabilized(&grid_ain_state, input_channel);

  if (!stabilized) {
    memcpy(old_value, new_value, sizeof(struct grid_ui_endless_state));
  }

  uint16_t value_degrees_new = grid_ui_endless_calculate_angle(new_value->phase_a, new_value->phase_b, 12);
  uint16_t value_degrees_old = grid_ui_endless_calculate_angle(old_value->phase_a, old_value->phase_b, 12);

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

      template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_DIRECTION_index] = value_degrees_new / 20;
      grid_ui_endless_update_trigger(ele, stabilized, delta, &old_value->encoder_last_real_time, &old_value->delta_vel_frac);

      old_value->phase_a = new_value->phase_a;
      old_value->phase_b = new_value->phase_b;
    }
  }
}
