#include "grid_module.h"

void grid_module_po16_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui) {

  // 16 pot, depth of 5, 14bit internal, 7bit result;
  grid_ain_init(ain, 16, 5);
  grid_led_init(led, 16);

  grid_ui_model_init(ui, 16 + 1); // +1 for the system element

  for (uint8_t j = 0; j < 16; j++) {

    grid_ui_element_init(ui, j, GRID_UI_ELEMENT_POTENTIOMETER);
  }

  grid_ui_element_init(ui, ui->element_list_length - 1, GRID_UI_ELEMENT_SYSTEM);

  ui->lua_ui_init_callback = grid_lua_ui_init_po16;
}

void grid_ui_potmeter_store_input(uint8_t input_channel, uint64_t* last_real_time, uint16_t value, uint8_t adc_bit_depth) {

  const uint16_t adc_max_value = (1 << adc_bit_depth) - 1;

  int32_t* template_parameter_list = grid_ui_state.element_list[input_channel].template_parameter_list;

  int32_t resolution = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MODE_index];

  grid_ain_add_sample(&grid_ain_state, input_channel, value, adc_bit_depth, (uint8_t)resolution);

  // limit lastrealtime
  uint64_t elapsed_time = grid_platform_rtc_get_elapsed_time(*last_real_time);
  if (GRID_PARAMETER_ELAPSED_LIMIT * MS_TO_US < grid_platform_rtc_get_elapsed_time(*last_real_time)) {
    *last_real_time = grid_platform_rtc_get_micros() - GRID_PARAMETER_ELAPSED_LIMIT * MS_TO_US;
    elapsed_time = GRID_PARAMETER_ELAPSED_LIMIT * MS_TO_US;
  }

  if (grid_ain_get_changed(&grid_ain_state, input_channel)) {

    // update lastrealtime
    *last_real_time = grid_platform_rtc_get_micros();
    template_parameter_list[GRID_LUA_FNC_P_POTMETER_ELAPSED_index] = elapsed_time / MS_TO_US;

    int32_t resolution = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MODE_index];

    if (resolution < 1) {
      resolution = 1;
    } else if (resolution > 12) {
      resolution = 12;
    }

    int32_t min = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MIN_index];
    int32_t max = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MAX_index];

    int32_t next = grid_ain_get_average_scaled(&grid_ain_state, input_channel, adc_bit_depth, resolution, min, max);
    template_parameter_list[GRID_LUA_FNC_P_POTMETER_VALUE_index] = next;

    // for display in editor
    int32_t state = grid_ain_get_average_scaled(&grid_ain_state, input_channel, adc_bit_depth, resolution, 0, 127);
    template_parameter_list[GRID_LUA_FNC_P_POTMETER_STATE_index] = state;

    struct grid_ui_event* eve = grid_ui_event_find(&grid_ui_state.element_list[input_channel], GRID_UI_EVENT_AC);

    if (grid_ui_state.ui_interaction_enabled) {
      grid_ui_event_trigger(eve);
    }
  }
}

void grid_module_bu16_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui) {

  // 16 pot, depth of 5, 14bit internal, 7bit result;
  grid_ain_init(ain, 16, 5);
  grid_led_init(led, 16);

  grid_ui_model_init(ui, 16 + 1); // +1 for the system element

  for (uint8_t j = 0; j < 16; j++) {

    grid_ui_element_init(ui, j, GRID_UI_ELEMENT_BUTTON);
  }

  grid_ui_element_init(ui, ui->element_list_length - 1, GRID_UI_ELEMENT_SYSTEM);

  ui->lua_ui_init_callback = grid_lua_ui_init_bu16;
}

void grid_ui_button_store_input(uint8_t input_channel, uint64_t* last_real_time, uint16_t value, uint8_t adc_bit_depth) {

  const uint16_t adc_max_value = (1 << adc_bit_depth) - 1;

  int32_t* template_parameter_list = grid_ui_state.element_list[input_channel].template_parameter_list;

  // limit lastrealtime
  uint32_t elapsed_time = grid_platform_rtc_get_elapsed_time(*last_real_time);
  if (GRID_PARAMETER_ELAPSED_LIMIT * MS_TO_US < grid_platform_rtc_get_elapsed_time(*last_real_time)) {
    *last_real_time = grid_platform_rtc_get_micros() - GRID_PARAMETER_ELAPSED_LIMIT * MS_TO_US;
    elapsed_time = GRID_PARAMETER_ELAPSED_LIMIT * MS_TO_US;
  }

  uint8_t result_valid = 0;

  if (value > adc_max_value * 0.9) {
    value = 0;
    result_valid = 1;
  } else if (value < adc_max_value * 0.01) {
    value = 127;
    result_valid = 1;
  }

  // schmitt trigger test failed, result not valid
  if (result_valid == 0) {
    return;
  }

  // value is the same as it was last time
  if (value == template_parameter_list[GRID_LUA_FNC_B_BUTTON_STATE_index]) {
    return;
  }

  // button change happened
  template_parameter_list[GRID_LUA_FNC_B_BUTTON_STATE_index] = value;

  // update lastrealtime
  *last_real_time = grid_platform_rtc_get_micros();
  template_parameter_list[GRID_LUA_FNC_B_BUTTON_ELAPSED_index] = elapsed_time / MS_TO_US;

  if (value != 0) { // Button Press Event

    if (template_parameter_list[GRID_LUA_FNC_B_BUTTON_MODE_index] == 0) {

      // Button ABS
      int32_t max = template_parameter_list[GRID_LUA_FNC_B_BUTTON_MAX_index];
      template_parameter_list[GRID_LUA_FNC_B_BUTTON_VALUE_index] = max;
    } else {

      // Toggle

      int32_t min = template_parameter_list[GRID_LUA_FNC_B_BUTTON_MIN_index];
      int32_t max = template_parameter_list[GRID_LUA_FNC_B_BUTTON_MAX_index];
      int32_t steps = template_parameter_list[GRID_LUA_FNC_B_BUTTON_MODE_index];
      int32_t last = template_parameter_list[GRID_LUA_FNC_B_BUTTON_VALUE_index];
      int32_t next = last + (max - min) / steps;

      if (next > max) {

        // overflow
        next = min;
      }

      template_parameter_list[GRID_LUA_FNC_B_BUTTON_VALUE_index] = next;
    }

    struct grid_ui_event* eve = grid_ui_event_find(&grid_ui_state.element_list[input_channel], GRID_UI_EVENT_BC);

    if (grid_ui_state.ui_interaction_enabled) {
      grid_ui_event_trigger(eve);
    }
  } else { // Button Release Event

    if (template_parameter_list[GRID_LUA_FNC_B_BUTTON_MODE_index] == 0) {

      // Button ABS
      int32_t min = template_parameter_list[GRID_LUA_FNC_B_BUTTON_MIN_index];
      template_parameter_list[GRID_LUA_FNC_B_BUTTON_VALUE_index] = min;
    } else {

      // Toggle
    }

    struct grid_ui_event* eve = grid_ui_event_find(&grid_ui_state.element_list[input_channel], GRID_UI_EVENT_BC);

    if (grid_ui_state.ui_interaction_enabled) {
      grid_ui_event_trigger(eve);
    }
  }
}

void grid_module_pbf4_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui) {

  // 16 pot, depth of 5, 14bit internal, 7bit result;
  grid_ain_init(ain, 16, 5);
  grid_led_init(led, 12);

  grid_ui_model_init(ui, 12 + 1); // +1 for the system element

  for (uint8_t j = 0; j < 8; j++) {

    grid_ui_element_init(ui, j, GRID_UI_ELEMENT_POTENTIOMETER);
  }

  for (uint8_t j = 8; j < 12; j++) {

    grid_ui_element_init(ui, j, GRID_UI_ELEMENT_BUTTON);
  }

  grid_ui_element_init(ui, ui->element_list_length - 1, GRID_UI_ELEMENT_SYSTEM);

  ui->lua_ui_init_callback = grid_lua_ui_init_pbf4;
}

void grid_module_pb44_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui) {

  // 16 pot, depth of 5, 14bit internal, 7bit result;
  grid_ain_init(ain, 16, 5);
  grid_led_init(led, 16);

  grid_ui_model_init(ui, 16 + 1); // +1 for the system element

  for (uint8_t j = 0; j < 8; j++) {

    grid_ui_element_init(ui, j, GRID_UI_ELEMENT_POTENTIOMETER);
  }

  for (uint8_t j = 8; j < 16; j++) {

    grid_ui_element_init(ui, j, GRID_UI_ELEMENT_BUTTON);
  }

  grid_ui_element_init(ui, ui->element_list_length - 1, GRID_UI_ELEMENT_SYSTEM);

  ui->lua_ui_init_callback = grid_lua_ui_init_pb44;
}

void grid_module_ef44_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui) {

  // should be 4 but indexing is bad at
  // grid_element_potmeter_template_parameter_init
  grid_ain_init(&grid_ain_state, 8, 5);

  grid_led_init(&grid_led_state, 8);

  grid_ui_model_init(ui, 8 + 1); // +1 for the system element

  for (uint8_t j = 0; j < 4; j++) {

    grid_ui_element_init(ui, j, GRID_UI_ELEMENT_ENCODER);
  }

  for (uint8_t j = 4; j < 8; j++) {

    grid_ui_element_init(ui, j, GRID_UI_ELEMENT_POTENTIOMETER);
  }

  grid_ui_element_init(ui, grid_ui_state.element_list_length - 1, GRID_UI_ELEMENT_SYSTEM);

  ui->lua_ui_init_callback = grid_lua_ui_init_ef44;
}

void grid_module_tek2_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui) {

  // 16 pot, depth of 5, 14bit internal, 7bit result;
  grid_ain_init(ain, 16, 5); // TODO: 12 ain for TEK2
  grid_led_init(led, 18);    // TODO: 18 led for TEK2

  uint8_t led_lookup[18] = {10, 11, 12, 13, 14, 15, 16, 17, 0, 5, 1, 6, 2, 7, 3, 8, 4, 9};

  grid_led_lookup_init(led, led_lookup); // initialize the optional led index
                                         // lookup table for array remapping

  grid_ui_model_init(ui, 10 + 1); // 10+1 for the system element on TEK2

  for (uint8_t j = 0; j < 10; j++) {

    if (j < 8) {

      grid_ui_element_init(ui, j, GRID_UI_ELEMENT_BUTTON);
    } else if (j < 10) {
      grid_ui_element_init(ui, j, GRID_UI_ELEMENT_ENCODER);
    }
  }

  grid_ui_element_init(ui, ui->element_list_length - 1, GRID_UI_ELEMENT_SYSTEM);

  ui->lua_ui_init_callback = grid_lua_ui_init_tek2;
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

void grid_ui_button_update_trigger(struct grid_ui_element* ele, uint64_t* button_last_real_time, uint8_t old_button_value, uint8_t new_button_value) {

  // limit lastrealtime
  uint32_t button_elapsed_time = grid_platform_rtc_get_elapsed_time(*button_last_real_time);
  if (GRID_PARAMETER_ELAPSED_LIMIT * MS_TO_US < grid_platform_rtc_get_elapsed_time(*button_last_real_time)) {
    *button_last_real_time = grid_platform_rtc_get_micros() - GRID_PARAMETER_ELAPSED_LIMIT * MS_TO_US;
    button_elapsed_time = GRID_PARAMETER_ELAPSED_LIMIT * MS_TO_US;
  }

  if (new_button_value == old_button_value) {
    // nothing left to do
    return;
  }

  // BUTTON CHANGE
  // update lastrealtime
  *button_last_real_time = grid_platform_rtc_get_micros();

  int32_t* template_parameter_list = ele->template_parameter_list;
  template_parameter_list[GRID_LUA_FNC_E_BUTTON_ELAPSED_index] = button_elapsed_time / MS_TO_US;

  if (new_button_value == 0) { // Button Press

    template_parameter_list[GRID_LUA_FNC_E_BUTTON_STATE_index] = 127;

    // Button ABS
    if (template_parameter_list[GRID_LUA_FNC_E_BUTTON_MODE_index] == 0) {

      int32_t max = template_parameter_list[GRID_LUA_FNC_E_BUTTON_MAX_index];
      template_parameter_list[GRID_LUA_FNC_E_BUTTON_VALUE_index] = max;
    } else {
      // IMPLEMENT STEP TOGGLE HERE					//
      // Toggle

      int32_t min = template_parameter_list[GRID_LUA_FNC_E_BUTTON_MIN_index];
      int32_t max = template_parameter_list[GRID_LUA_FNC_E_BUTTON_MAX_index];
      int32_t steps = template_parameter_list[GRID_LUA_FNC_E_BUTTON_MODE_index];
      int32_t last = template_parameter_list[GRID_LUA_FNC_E_BUTTON_VALUE_index];

      int32_t next = last + (max - min) / steps;

      if (next > max) {

        // overflow
        next = min;
      }

      template_parameter_list[GRID_LUA_FNC_E_BUTTON_VALUE_index] = next;
    }

    struct grid_ui_event* eve = grid_ui_event_find(ele, GRID_UI_EVENT_BC);

    if (grid_ui_state.ui_interaction_enabled) {
      grid_ui_event_trigger(eve);
    }
  } else { // Button Release

    template_parameter_list[GRID_LUA_FNC_E_BUTTON_STATE_index] = 0;

    // Button ABS
    if (template_parameter_list[GRID_LUA_FNC_E_BUTTON_MODE_index] == 0) {

      int32_t min = template_parameter_list[GRID_LUA_FNC_E_BUTTON_MIN_index];

      template_parameter_list[GRID_LUA_FNC_E_BUTTON_VALUE_index] = min;
    } else {
      // IMPLEMENT STEP TOGGLE HERE
    }

    struct grid_ui_event* eve = grid_ui_event_find(ele, GRID_UI_EVENT_BC);

    if (grid_ui_state.ui_interaction_enabled) {
      grid_ui_event_trigger(eve);
    }
  }
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
  double velocityfactor = ((25 * 25 - elapsed_ms * elapsed_ms) / 75.0) * minmaxscale * velocityparam + 1.0;

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

  struct grid_ui_event* eve = grid_ui_event_find(ele, GRID_UI_EVENT_EC);

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

uint16_t grid_ui_endlesspot_calculate_angle(uint16_t phase_a, uint16_t phase_b, uint8_t adc_bit_depth) {

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

void grid_ui_endlesspot_store_input(uint8_t input_channel, uint64_t* encoder_last_real_time, struct grid_module_endlesspot_state* old_value, struct grid_module_endlesspot_state* new_value,
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

      template_parameter_list[GRID_LUA_FNC_E_ENCODER_STATE_index] = value_degrees_new / 20;
      uint8_t has_triggered = grid_ui_encoder_update_trigger(ele, encoder_last_real_time, delta, 1);

      if (has_triggered) {
        old_value->phase_a_value = new_value->phase_a_value;
        old_value->phase_b_value = new_value->phase_b_value;
        old_value->knob_angle = new_value->knob_angle;
      }
    }
  }
}

void grid_module_en16_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui) {

  grid_led_init(&grid_led_state, 16);

  grid_ui_model_init(ui, 16 + 1); // +1 for the system element

  for (uint8_t j = 0; j < 16; j++) {

    grid_ui_element_init(ui, j, GRID_UI_ELEMENT_ENCODER);
  }

  grid_ui_element_init(ui, grid_ui_state.element_list_length - 1, GRID_UI_ELEMENT_SYSTEM);

  ui->lua_ui_init_callback = grid_lua_ui_init_en16;
}
