#include "grid_ui_button.h"

extern uint8_t grid_platform_get_adc_bit_depth();

extern void grid_platform_printf(char const* fmt, ...);

const char grid_ui_button_init_actionstring[] = GRID_ACTIONSTRING_BUTTON_INIT;
const char grid_ui_button_change_actionstring[] = GRID_ACTIONSTRING_BUTTON_BUTTON;
const char grid_ui_button_timer_actionstring[] = GRID_ACTIONSTRING_SYSTEM_TIMER;

void grid_ui_button_state_init(struct grid_ui_button_state* state, uint8_t adc_bit_depth, double threshold, double hysteresis) {

  assert(adc_bit_depth >= 1 && adc_bit_depth <= 16);
  assert(threshold >= 0. && threshold <= 1.);
  assert(hysteresis >= 0. && hysteresis <= 1.);

  // The minimum observed bit depth is an eighth of the maximum by default,
  // or just 1 LSB if the input bit depth is 3 bits or less
  uint8_t min_range_depth = adc_bit_depth > 3 ? adc_bit_depth - 3 : 0;

  state->last_real_time = 0;
  state->min_value = UINT16_MAX;
  state->max_value = 0;
  state->min_range = (1 << min_range_depth);
  state->max_range = (1 << adc_bit_depth);
  state->threshold = threshold;
  state->hysteresis = hysteresis;
}

bool grid_ui_button_state_range_valid(struct grid_ui_button_state* state) {

  if (!(state->min_value < state->max_value)) {
    return false;
  }

  uint16_t curr_range = state->max_value - state->min_value;

  return curr_range >= state->min_range;
}

void grid_ui_button_state_range_update(struct grid_ui_button_state* state, uint16_t value) {

  if (value < state->min_value) {
    state->min_value = value;
  }

  if (value > state->max_value) {
    state->max_value = value;
  }
}

static float lerp(float a, float b, float x) { return a * (1.0 - x) + (b * x); }

uint16_t grid_ui_button_state_get_low_trigger(struct grid_ui_button_state* state) {

  double curr_threshold = state->threshold - state->hysteresis / 2;

  assert(curr_threshold >= 0. && curr_threshold <= 1.);

  return lerp(state->min_value, state->max_value, curr_threshold);
}

uint16_t grid_ui_button_state_get_high_trigger(struct grid_ui_button_state* state) {

  double curr_threshold = state->threshold + state->hysteresis / 2;

  assert(curr_threshold >= 0. && curr_threshold <= 1.);

  return lerp(state->min_value, state->max_value, curr_threshold) + 1;
}

void grid_ui_element_button_init(struct grid_ui_element* ele) {

  ele->type = GRID_PARAMETER_ELEMENT_BUTTON;

  ele->event_list_length = 3;

  ele->event_list = malloc(ele->event_list_length * sizeof(struct grid_ui_event));

  grid_ui_event_init(ele, 0, GRID_PARAMETER_EVENT_INIT, GRID_LUA_FNC_A_INIT_short, grid_ui_button_init_actionstring);       // Element Initialization Event
  grid_ui_event_init(ele, 1, GRID_PARAMETER_EVENT_BUTTON, GRID_LUA_FNC_A_BUTTON_short, grid_ui_button_change_actionstring); // Button Change
  grid_ui_event_init(ele, 2, GRID_PARAMETER_EVENT_TIMER, GRID_LUA_FNC_A_TIMER_short, grid_ui_button_timer_actionstring);

  ele->template_initializer = &grid_ui_element_button_template_parameter_init;
  ele->template_parameter_list_length = GRID_LUA_FNC_B_LIST_length;
  ele->template_parameter_element_position_index_1 = GRID_LUA_FNC_B_BUTTON_STATE_index;
  ele->template_parameter_element_position_index_2 = GRID_LUA_FNC_B_BUTTON_STATE_index;

  ele->event_clear_cb = &grid_ui_element_button_event_clear_cb;
  ele->page_change_cb = &grid_ui_element_button_page_change_cb;
}

void grid_ui_element_button_template_parameter_init(struct grid_ui_template_buffer* buf) {

  uint8_t element_index = buf->parent->index;
  int32_t* template_parameter_list = buf->template_parameter_list;

  template_parameter_list[GRID_LUA_FNC_B_ELEMENT_INDEX_index] = element_index;
  template_parameter_list[GRID_LUA_FNC_B_BUTTON_NUMBER_index] = element_index;
  template_parameter_list[GRID_LUA_FNC_B_BUTTON_VALUE_index] = 0;
  template_parameter_list[GRID_LUA_FNC_B_BUTTON_MIN_index] = 0;
  template_parameter_list[GRID_LUA_FNC_B_BUTTON_MAX_index] = 127;
  template_parameter_list[GRID_LUA_FNC_B_BUTTON_MODE_index] = 0;
  template_parameter_list[GRID_LUA_FNC_B_BUTTON_ELAPSED_index] = 0;
  template_parameter_list[GRID_LUA_FNC_B_BUTTON_STATE_index] = 0;
}

void grid_ui_element_button_event_clear_cb(struct grid_ui_event* eve) {}

void grid_ui_element_button_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new) {

  // for (uint8_t i=0; i<grid_ui_state.element_list_length; i++){

  // 	struct grid_ui_event* eve = NULL;

  // 	eve = grid_ui_event_find(&grid_ui_state.element_list[i],
  // GRID_PARAMETER_EVENT_INIT); 	grid_ui_event_trigger_local(eve);

  // 	eve = grid_ui_event_find(&grid_ui_state.element_list[i],
  // GRID_PARAMETER_EVENT_BUTTON); 	grid_ui_event_trigger_local(eve);
  // }
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
  template_parameter_list[GRID_LUA_FNC_B_BUTTON_ELAPSED_index] = button_elapsed_time / MS_TO_US;

  if (new_button_value == 0) { // Button Press

    template_parameter_list[GRID_LUA_FNC_B_BUTTON_STATE_index] = 127;

    // Button ABS
    if (template_parameter_list[GRID_LUA_FNC_B_BUTTON_MODE_index] == 0) {

      int32_t max = template_parameter_list[GRID_LUA_FNC_B_BUTTON_MAX_index];
      template_parameter_list[GRID_LUA_FNC_B_BUTTON_VALUE_index] = max;
    } else {
      // IMPLEMENT STEP TOGGLE HERE					//
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

    struct grid_ui_event* eve = grid_ui_event_find(ele, GRID_PARAMETER_EVENT_BUTTON);

    grid_ui_event_trigger(eve);
  } else { // Button Release

    template_parameter_list[GRID_LUA_FNC_B_BUTTON_STATE_index] = 0;

    // Button ABS
    if (template_parameter_list[GRID_LUA_FNC_B_BUTTON_MODE_index] == 0) {

      int32_t min = template_parameter_list[GRID_LUA_FNC_B_BUTTON_MIN_index];

      template_parameter_list[GRID_LUA_FNC_B_BUTTON_VALUE_index] = min;
    } else {
      // IMPLEMENT STEP TOGGLE HERE
    }

    struct grid_ui_event* eve = grid_ui_event_find(ele, GRID_PARAMETER_EVENT_BUTTON);

    grid_ui_event_trigger(eve);
  }
}

void grid_ui_button_store_input(struct grid_ui_button_state* state, uint8_t input_channel, uint16_t value, uint8_t adc_bit_depth) {

  grid_ui_button_state_range_update(state, value);
  if (!grid_ui_button_state_range_valid(state)) {
    return;
  }

  const uint16_t adc_max_value = (1 << adc_bit_depth) - 1;

  int32_t* template_parameter_list = grid_ui_state.element_list[input_channel].template_parameter_list;

  // limit lastrealtime
  uint32_t elapsed_time = grid_platform_rtc_get_elapsed_time(state->last_real_time);
  if (GRID_PARAMETER_ELAPSED_LIMIT * MS_TO_US < grid_platform_rtc_get_elapsed_time(state->last_real_time)) {
    state->last_real_time = grid_platform_rtc_get_micros() - GRID_PARAMETER_ELAPSED_LIMIT * MS_TO_US;
    elapsed_time = GRID_PARAMETER_ELAPSED_LIMIT * MS_TO_US;
  }

  uint8_t result_valid = 0;

  if (value >= grid_ui_button_state_get_high_trigger(state)) {
    value = 0;
    result_valid = 1;
  } else if (value <= grid_ui_button_state_get_low_trigger(state)) {
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
  state->last_real_time = grid_platform_rtc_get_micros();
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

    struct grid_ui_event* eve = grid_ui_event_find(&grid_ui_state.element_list[input_channel], GRID_PARAMETER_EVENT_BUTTON);

    grid_ui_event_trigger(eve);
  } else { // Button Release Event

    if (template_parameter_list[GRID_LUA_FNC_B_BUTTON_MODE_index] == 0) {

      // Button ABS
      int32_t min = template_parameter_list[GRID_LUA_FNC_B_BUTTON_MIN_index];
      template_parameter_list[GRID_LUA_FNC_B_BUTTON_VALUE_index] = min;
    } else {

      // Toggle
    }

    struct grid_ui_event* eve = grid_ui_event_find(&grid_ui_state.element_list[input_channel], GRID_PARAMETER_EVENT_BUTTON);

    grid_ui_event_trigger(eve);
  }
}
