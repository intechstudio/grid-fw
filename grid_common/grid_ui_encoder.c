#include "grid_ui_encoder.h"
#include "grid_ui_button.h"

#include "grid_ui_system.h"

#include "grid_ain.h"
#include "grid_lua_api.h"
#include "grid_protocol.h"
#include "grid_ui.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern uint8_t grid_platform_get_adc_bit_depth();

extern void grid_platform_printf(char const* fmt, ...);

const char grid_ui_encoder_init_actionstring[] = GRID_ACTIONSTRING_ENCODER_INIT;
const char grid_ui_encoder_encoderchange_actionstring[] = GRID_ACTIONSTRING_ENCODER_ENCODER;
const char grid_ui_encoder_buttonchange_actionstring[] = GRID_ACTIONSTRING_BUTTON_BUTTON;
const char grid_ui_encoder_timer_actionstring[] = GRID_ACTIONSTRING_SYSTEM_TIMER;

void grid_ui_element_encoder_init(struct grid_ui_element* ele) {

  ele->type = GRID_PARAMETER_ELEMENT_ENCODER;

  ele->event_list_length = 4;

  ele->event_list = malloc(ele->event_list_length * sizeof(struct grid_ui_event));

  grid_ui_event_init(ele, 0, GRID_PARAMETER_EVENT_INIT, GRID_LUA_FNC_A_INIT_short, grid_ui_encoder_init_actionstring);                // Element Initialization Event
  grid_ui_event_init(ele, 1, GRID_PARAMETER_EVENT_ENCODER, GRID_LUA_FNC_A_ENCODER_short, grid_ui_encoder_encoderchange_actionstring); // Encoder Change
  grid_ui_event_init(ele, 2, GRID_PARAMETER_EVENT_BUTTON, GRID_LUA_FNC_A_BUTTON_short, grid_ui_encoder_buttonchange_actionstring);    // Button Change
  grid_ui_event_init(ele, 3, GRID_PARAMETER_EVENT_TIMER, GRID_LUA_FNC_A_TIMER_short, grid_ui_encoder_timer_actionstring);

  ele->template_initializer = &grid_ui_element_encoder_template_parameter_init;
  ele->template_parameter_list_length = GRID_LUA_FNC_E_LIST_length;
  ele->template_parameter_element_position_index_1 = GRID_LUA_FNC_E_BUTTON_STATE_index;
  ele->template_parameter_element_position_index_2 = GRID_LUA_FNC_E_ENCODER_STATE_index;

  ele->event_clear_cb = &grid_ui_element_encoder_event_clear_cb;
  ele->page_change_cb = &grid_ui_element_encoder_page_change_cb;
}

void grid_ui_element_encoder_template_parameter_init(struct grid_ui_template_buffer* buf) {

  // printf("template parameter init\r\n");

  uint8_t element_index = buf->parent->index;
  int32_t* template_parameter_list = buf->template_parameter_list;

  template_parameter_list[GRID_LUA_FNC_E_ELEMENT_INDEX_index] = element_index;
  template_parameter_list[GRID_LUA_FNC_E_BUTTON_NUMBER_index] = element_index;
  template_parameter_list[GRID_LUA_FNC_E_BUTTON_VALUE_index] = 0;
  template_parameter_list[GRID_LUA_FNC_E_BUTTON_MIN_index] = 0;
  template_parameter_list[GRID_LUA_FNC_E_BUTTON_MAX_index] = 127;
  template_parameter_list[GRID_LUA_FNC_E_BUTTON_MODE_index] = 0;
  template_parameter_list[GRID_LUA_FNC_E_BUTTON_ELAPSED_index] = 0;
  template_parameter_list[GRID_LUA_FNC_E_BUTTON_STATE_index] = 0;

  template_parameter_list[GRID_LUA_FNC_E_ENCODER_NUMBER_index] = element_index;
  template_parameter_list[GRID_LUA_FNC_E_ENCODER_VALUE_index] = 0;
  template_parameter_list[GRID_LUA_FNC_E_ENCODER_MIN_index] = 0;
  template_parameter_list[GRID_LUA_FNC_E_ENCODER_MAX_index] = 128 - 1;
  template_parameter_list[GRID_LUA_FNC_E_ENCODER_MODE_index] = 0;
  template_parameter_list[GRID_LUA_FNC_E_ENCODER_ELAPSED_index] = 0;
  template_parameter_list[GRID_LUA_FNC_E_ENCODER_STATE_index] = 64;
  template_parameter_list[GRID_LUA_FNC_E_ENCODER_VELOCITY_index] = 50;
  template_parameter_list[GRID_LUA_FNC_E_ENCODER_SENSITIVITY_index] = 100;
}

void grid_ui_element_encoder_event_clear_cb(struct grid_ui_event* eve) {

  int32_t* template_parameter_list = eve->parent->template_parameter_list;

  template_parameter_list[GRID_LUA_FNC_E_ENCODER_STATE_index] = 64;

  if (template_parameter_list[GRID_LUA_FNC_E_ENCODER_MODE_index] == 1) { // relative

    int32_t min = template_parameter_list[GRID_LUA_FNC_E_ENCODER_MIN_index];
    int32_t max = template_parameter_list[GRID_LUA_FNC_E_ENCODER_MAX_index];

    template_parameter_list[GRID_LUA_FNC_E_ENCODER_VALUE_index] = ((max + 1) - min) / 2;
  } else if (template_parameter_list[GRID_LUA_FNC_E_ENCODER_MODE_index] == 2) {

    template_parameter_list[GRID_LUA_FNC_E_ENCODER_VALUE_index] = 0;
  }
}

void grid_ui_element_encoder_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new) {

  // for (uint8_t i = 0; i<16; i++)
  // {

  // 	struct grid_ui_event* eve = NULL;

  // 	eve = grid_ui_event_find(&grid_ui_state.element_list[i],
  // GRID_PARAMETER_EVENT_INIT); 	grid_ui_event_trigger_local(eve);

  // 	eve = grid_ui_event_find(&grid_ui_state.element_list[i],
  // GRID_PARAMETER_EVENT_ENCODER); 	grid_ui_event_trigger_local(eve);
  // }
}

int16_t grid_ui_encoder_rotation_delta(uint8_t old_value, uint8_t new_value) {

  // lookup table, of state machine of the combination of old encoder AB and new
  // encoder AB
  static int8_t encoder_heading[] = {
      0, 1, -1, 0, -1, 0, 0, 1, 1, 0, 0, -1, 0, -1, 1, 0,
  };
  uint8_t encoder_state = (old_value & 0b11) << 2 | (new_value & 0b11);
  int16_t delta = encoder_heading[encoder_state];

  // Only non-detent modules want updates every quadrature phase.
  if (grid_sys_get_hwcfg(&grid_sys_state) != GRID_MODULE_EN16_ND_RevA && grid_sys_get_hwcfg(&grid_sys_state) != GRID_MODULE_EN16_ND_RevD) {
    if ((new_value & 0b11) != 0b11) { // we haven't landed on the detent
      // override delta to ignore in-between movement of the encoder for
      // detented modules
      delta = 0;
    }
  }

  return delta;
}

uint8_t grid_ui_encoder_update_trigger(struct grid_ui_element* ele, uint64_t* encoder_last_real_time, int16_t delta, uint8_t is_endless_pot) {

  uint32_t encoder_elapsed_time = grid_platform_rtc_get_elapsed_time(*encoder_last_real_time);
  if (GRID_PARAMETER_ELAPSED_LIMIT * MS_TO_US < grid_platform_rtc_get_elapsed_time(*encoder_last_real_time)) {
    *encoder_last_real_time = grid_platform_rtc_get_micros() - GRID_PARAMETER_ELAPSED_LIMIT * MS_TO_US;
    encoder_elapsed_time = GRID_PARAMETER_ELAPSED_LIMIT * MS_TO_US;
  }

  if (delta == 0) {
    // nothing left to do
    return 0; // did not trigger
  }

  // update lastrealtime
  *encoder_last_real_time = grid_platform_rtc_get_micros();
  int32_t* template_parameter_list = ele->template_parameter_list;
  template_parameter_list[GRID_LUA_FNC_E_ENCODER_ELAPSED_index] = encoder_elapsed_time / MS_TO_US;

  int32_t min = template_parameter_list[GRID_LUA_FNC_E_ENCODER_MIN_index];
  int32_t max = template_parameter_list[GRID_LUA_FNC_E_ENCODER_MAX_index];

  double elapsed_ms = encoder_elapsed_time / MS_TO_US;

  if (elapsed_ms > 25) {
    elapsed_ms = 25;
  }

  if (elapsed_ms < 1) {
    elapsed_ms = 1;
  }

  double minmaxscale = (max - min) / 128.0;

  double velocityparam = template_parameter_list[GRID_LUA_FNC_E_ENCODER_VELOCITY_index] / 100.0;

  // implement configurable velocity parameters here
  double velocityfactor = ((25 * 25 - elapsed_ms * elapsed_ms) / 75.0) * minmaxscale * velocityparam + (1.0 * template_parameter_list[GRID_LUA_FNC_E_ENCODER_SENSITIVITY_index] / 100.0);

  if (is_endless_pot) {
    velocityfactor = minmaxscale * velocityparam / 15.0;
  }

  int32_t delta_velocity = delta * velocityfactor;

  int32_t old_value = template_parameter_list[GRID_LUA_FNC_E_ENCODER_VALUE_index];

  if (is_endless_pot) {

    if (delta_velocity == 0) {

      return 0; // did not trigger
    }
  } else {
    template_parameter_list[GRID_LUA_FNC_E_ENCODER_STATE_index] += delta_velocity;
  }

  if (template_parameter_list[GRID_LUA_FNC_E_ENCODER_MODE_index] == 0) { // Absolute

    int32_t new_value = 0;

    if (old_value + delta_velocity < min) {
      new_value = min;
    } else if (old_value + delta_velocity > max) {
      new_value = max;
    } else {
      new_value = old_value + delta_velocity;
    }

    template_parameter_list[GRID_LUA_FNC_E_ENCODER_VALUE_index] = new_value;
  } else if (template_parameter_list[GRID_LUA_FNC_E_ENCODER_MODE_index] == 1) { // Relative

    int32_t new_value = 0;

    if (old_value + delta_velocity < min) {
      new_value = min;
    } else if (old_value + delta_velocity > max) {
      new_value = max;
    } else {
      new_value = old_value + delta_velocity;
    }

    template_parameter_list[GRID_LUA_FNC_E_ENCODER_VALUE_index] = new_value;
  } else if (template_parameter_list[GRID_LUA_FNC_E_ENCODER_MODE_index] == 2) { // Relative 2's complement

    // Two's complement magic 7 bit signed variable

    int32_t old_twoscomplement = template_parameter_list[GRID_LUA_FNC_E_ENCODER_VALUE_index];

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

    template_parameter_list[GRID_LUA_FNC_E_ENCODER_VALUE_index] = new_7bit_twoscomplement;
  }

  struct grid_ui_event* eve = grid_ui_event_find(ele, GRID_PARAMETER_EVENT_ENCODER);

  if (grid_ui_state.ui_interaction_enabled) {
    grid_ui_event_trigger(eve);
  }

  return 1; // did trigger
}

void grid_ui_encoder_store_input(uint8_t input_channel, uint64_t* encoder_last_real_time, uint64_t* button_last_real_time, uint8_t old_value, uint8_t new_value, uint8_t* phase_change_lock) {

  if (old_value == new_value) {
    // no change since the last time we read the shift register
    return;
  }

  int16_t delta = grid_ui_encoder_rotation_delta(old_value, new_value); // delta can be -1, 0 or 1

  // shift register bits arrangement: MSB to LSB
  // GND Button PhaseB PhaseA
  uint8_t new_button_value = (new_value & 0b00000100) ? 1 : 0;
  uint8_t old_button_value = (old_value & 0b00000100) ? 1 : 0;

  struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, input_channel);

  // Evaluate the results
  grid_ui_button_update_trigger(ele, button_last_real_time, old_button_value, new_button_value);
  grid_ui_encoder_update_trigger(ele, encoder_last_real_time, delta, 0);
}
