#include "grid_ui_button.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "grid_ain.h"
#include "grid_sys.h"
#include "grid_platform.h"
#include "grid_ui_system.h"

extern uint8_t grid_platform_get_adc_bit_depth();

extern void grid_platform_printf(char const* fmt, ...);

const char grid_ui_button_init_actionstring[] = GRID_ACTIONSTRING_BUTTON_INIT;
const char grid_ui_button_change_actionstring[] = GRID_ACTIONSTRING_BUTTON_BUTTON;
const char grid_ui_button_timer_actionstring[] = GRID_ACTIONSTRING_SYSTEM_TIMER;

void grid_ui_button_state_init(struct grid_ui_button_state* state, uint8_t adc_bit_depth, double threshold, double hysteresis) {

  assert(adc_bit_depth >= 1 && adc_bit_depth <= 16);
  assert(threshold >= 0. && threshold <= 1.);
  assert(hysteresis >= 0. && hysteresis <= 1.);

  // The minimum observed bit depth is a fourth of the maximum by default,
  // or just 1 LSB if the input bit depth is 2 bits or less
  uint8_t min_range_depth = adc_bit_depth > 2 ? adc_bit_depth - 2 : 0;

  state->last_real_time = 0;
  state->full_range = 1 << adc_bit_depth;
  state->min_value = UINT16_MAX;
  state->max_value = 0;
  state->trig_lo = state->min_value;
  state->trig_hi = state->max_value;
  state->min_range = (1 << min_range_depth);
  state->threshold = threshold;
  state->hysteresis = hysteresis;
  state->prev_in = state->curr_in = 0;
  state->prev_out = state->curr_out = 0;
  state->prev_time = state->curr_time = 0;
}

bool grid_ui_button_state_range_valid(struct grid_ui_button_state* state) {

  bool interval_valid = state->min_value < state->max_value;

  uint16_t eighth_range = state->full_range >> 3;

  // The minimum observed value should be sufficiently low
  // (currently, this helps with making the first-press velocity meaningful)
  bool three_eighths_less = state->min_value <= eighth_range * 3;

  uint16_t curr_range = state->max_value - state->min_value;

  bool range_valid = curr_range >= state->min_range;

  return interval_valid && three_eighths_less && range_valid;
}

static double lerp(double a, double b, double x) { return a * (1.0 - x) + (b * x); }

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

void grid_ui_button_state_value_update(struct grid_ui_button_state* state, uint16_t value, uint64_t now) {

  if (value < state->min_value) {

    state->min_value = value;

    state->trig_lo = grid_ui_button_state_get_low_trigger(state);
    state->trig_hi = grid_ui_button_state_get_high_trigger(state);
  }

  if (value > state->max_value) {

    state->max_value = value;

    state->trig_lo = grid_ui_button_state_get_low_trigger(state);
    state->trig_hi = grid_ui_button_state_get_high_trigger(state);
  }

  state->prev_time = state->curr_time;
  state->curr_time = now;

  state->prev_in = state->curr_in;
  state->curr_in = value;
}

double grid_ui_button_state_derivate(struct grid_ui_button_state* state) {

  double rise = (state->curr_in - (int32_t)state->prev_in) / (double)state->full_range;

  uint64_t elapsed = state->curr_time - state->prev_time;

  double run = elapsed / 1250.;

  double deriv = -1 * rise / run;

  return clampf64(deriv, 0., 1.);
}

bool grid_ui_button_state_get_with_hysteresis(struct grid_ui_button_state* state, uint8_t* out) {

  if (state->curr_in <= state->trig_lo) {

    *out = 1;
    return true;
  }

  if (state->curr_in >= state->trig_hi) {

    *out = 0;
    return true;
  }

  return false;
}

bool grid_ui_button_state_process(struct grid_ui_button_state* state, int mode, uint16_t value) {

  if (mode == -2) {

    state->prev_out = state->curr_out;

    if (abs(state->curr_out - (int32_t)state->curr_in) > 5) {

      state->curr_out = state->curr_in;
    }

  } else {

    state->prev_out = state->curr_out;

    if (state->curr_in <= state->trig_lo && state->prev_out == 0) {
      state->curr_out = 1;
    }

    if (state->curr_in >= state->trig_hi && state->prev_out == 1) {
      state->curr_out = 0;
    }
  }

  return state->prev_out != state->curr_out;
}

void grid_ui_element_button_init(struct grid_ui_element* ele) {

  ele->type = GRID_PARAMETER_ELEMENT_BUTTON;

  grid_ui_element_malloc_events(ele, 3);

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

void grid_ui_button_store_input(struct grid_ui_element* ele, struct grid_ui_button_state* state, uint16_t value, uint8_t adc_bit_depth) {

  assert(ele);

  int32_t* template_parameter_list = ele->template_parameter_list;

  uint64_t now = grid_platform_rtc_get_micros();
  grid_ui_button_state_value_update(state, value, now);
  if (!grid_ui_button_state_range_valid(state)) {
    return;
  }

  // limit lastrealtime
  uint32_t elapsed_time = now - state->last_real_time;
  uint32_t limit = GRID_PARAMETER_ELAPSED_LIMIT * MS_TO_US;
  if (elapsed_time > limit) {
    state->last_real_time = now - limit;
    elapsed_time = limit;
  }

  int32_t mode = template_parameter_list[GRID_LUA_FNC_B_BUTTON_MODE_index];
  if (!grid_ui_button_state_process(state, mode, value)) {
    return;
  }

  // update lastrealtime
  state->last_real_time = now;
  template_parameter_list[GRID_LUA_FNC_B_BUTTON_ELAPSED_index] = elapsed_time / MS_TO_US;

  // 1-bit output with hysteresis
  uint8_t hyst = 0;

  if (template_parameter_list[GRID_LUA_FNC_B_BUTTON_MODE_index] == -2) {

    int32_t min = template_parameter_list[GRID_LUA_FNC_B_BUTTON_MIN_index];
    int32_t max = template_parameter_list[GRID_LUA_FNC_B_BUTTON_MAX_index];

    int32_t old_value = template_parameter_list[GRID_LUA_FNC_B_BUTTON_VALUE_index];

    uint16_t curr_range = state->max_value - state->min_value;
    double normalized = (state->curr_out - state->min_value) / (double)curr_range;
    double deadzone = 0.02;
    double deadzoned = lerp(0 - deadzone, 1 + deadzone, normalized);
    int32_t new_value = clampi32(max - lerp(min, max, deadzoned), min, max);

    if (old_value == new_value) {
      return;
    }

    template_parameter_list[GRID_LUA_FNC_B_BUTTON_VALUE_index] = new_value;

    if (grid_ui_button_state_get_with_hysteresis(state, &hyst)) {
      template_parameter_list[GRID_LUA_FNC_B_BUTTON_STATE_index] = hyst * 127;
    }

  } else if (template_parameter_list[GRID_LUA_FNC_B_BUTTON_MODE_index] == -1) {

    int32_t min = template_parameter_list[GRID_LUA_FNC_B_BUTTON_MIN_index];
    int32_t max = template_parameter_list[GRID_LUA_FNC_B_BUTTON_MAX_index];

    int32_t old_dir = template_parameter_list[GRID_LUA_FNC_B_BUTTON_VALUE_index] != min;

    int32_t new_dir = state->curr_out;

    if (old_dir == new_dir) {
      return;
    }

    double derivate = grid_ui_button_state_derivate(state);
    int32_t minmax_dir = (max - min >= 0) * 2 - 1;
    int32_t velocity = clampi32(lerp(min, max, derivate), min + minmax_dir, max);
    int32_t new_value = new_dir ? velocity : min;

    template_parameter_list[GRID_LUA_FNC_B_BUTTON_VALUE_index] = new_value;

    if (grid_ui_button_state_get_with_hysteresis(state, &hyst)) {
      template_parameter_list[GRID_LUA_FNC_B_BUTTON_STATE_index] = hyst * 127;
    }

  } else if (template_parameter_list[GRID_LUA_FNC_B_BUTTON_MODE_index] == 0) {

    int32_t min = template_parameter_list[GRID_LUA_FNC_B_BUTTON_MIN_index];
    int32_t max = template_parameter_list[GRID_LUA_FNC_B_BUTTON_MAX_index];

    int32_t old_value = template_parameter_list[GRID_LUA_FNC_B_BUTTON_VALUE_index];

    int32_t new_value = state->curr_out ? max : min;

    if (old_value == new_value) {
      return;
    }

    template_parameter_list[GRID_LUA_FNC_B_BUTTON_VALUE_index] = new_value;

    if (grid_ui_button_state_get_with_hysteresis(state, &hyst)) {
      template_parameter_list[GRID_LUA_FNC_B_BUTTON_STATE_index] = hyst * 127;
    }

  } else {

    int32_t new_value = state->curr_out;

    if (!new_value) {
      return;
    }

    int32_t min = template_parameter_list[GRID_LUA_FNC_B_BUTTON_MIN_index];
    int32_t max = template_parameter_list[GRID_LUA_FNC_B_BUTTON_MAX_index];
    int32_t steps = template_parameter_list[GRID_LUA_FNC_B_BUTTON_MODE_index];
    int32_t last = template_parameter_list[GRID_LUA_FNC_B_BUTTON_VALUE_index];
    int32_t next = last + (max - min) / steps;

    if (next > max) {
      next = min;
    }

    template_parameter_list[GRID_LUA_FNC_B_BUTTON_VALUE_index] = next;
    template_parameter_list[GRID_LUA_FNC_B_BUTTON_STATE_index] = 127;
  }

  struct grid_ui_event* eve = grid_ui_event_find(ele, GRID_PARAMETER_EVENT_BUTTON);

  grid_ui_event_trigger(eve);
}
