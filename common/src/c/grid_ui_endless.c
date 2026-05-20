#include "grid_ui_endless.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "grid_ain.h"
#include "grid_lua_api.h"
#include "grid_math.h"
#include "grid_platform.h"
#include "grid_sys.h"
#include "grid_ui_button.h"
#include "grid_ui_system.h"

const luaL_Reg GRID_LUA_EP_INDEX_META[] = {{GRID_LUA_FNC_EP_ELEMENT_INDEX_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_EP_ELEMENT_INDEX_index)},
                                           {GRID_LUA_FNC_EP_LED_INDEX_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_EP_LED_INDEX_index)},
                                           {GRID_LUA_FNC_EP_BUTTON_VALUE_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_EP_BUTTON_VALUE_index)},
                                           {GRID_LUA_FNC_EP_BUTTON_MIN_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_EP_BUTTON_MIN_index)},
                                           {GRID_LUA_FNC_EP_BUTTON_MAX_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_EP_BUTTON_MAX_index)},
                                           {GRID_LUA_FNC_EP_BUTTON_MODE_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_EP_BUTTON_MODE_index)},
                                           {GRID_LUA_FNC_EP_BUTTON_ELAPSED_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_EP_BUTTON_ELAPSED_index)},
                                           {GRID_LUA_FNC_EP_BUTTON_STATE_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_EP_BUTTON_STATE_index)},
                                           {GRID_LUA_FNC_EP_LED_OFFSET_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_EP_LED_OFFSET_index)},
                                           {GRID_LUA_FNC_EP_ENDLESS_VALUE_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_EP_ENDLESS_VALUE_index)},
                                           {GRID_LUA_FNC_EP_ENDLESS_MIN_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_EP_ENDLESS_MIN_index)},
                                           {GRID_LUA_FNC_EP_ENDLESS_MAX_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_EP_ENDLESS_MAX_index)},
                                           {GRID_LUA_FNC_EP_ENDLESS_MODE_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_EP_ENDLESS_MODE_index)},
                                           {GRID_LUA_FNC_EP_ENDLESS_ELAPSED_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_EP_ENDLESS_ELAPSED_index)},
                                           {GRID_LUA_FNC_EP_ENDLESS_STATE_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_EP_ENDLESS_STATE_index)},
                                           {GRID_LUA_FNC_EP_ENDLESS_VELOCITY_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_EP_ENDLESS_VELOCITY_index)},
                                           {GRID_LUA_FNC_EP_ENDLESS_DIRECTION_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_EP_ENDLESS_DIRECTION_index)},
                                           {GRID_LUA_FNC_EP_ENDLESS_SENSITIVITY_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_EP_ENDLESS_SENSITIVITY_index)},
                                           {GRID_LUA_FNC_G_TIMER_START_short, XAFTERX(GRID_LUA_FNC_META_NAME, gtt)},
                                           {GRID_LUA_FNC_G_TIMER_STOP_short, XAFTERX(GRID_LUA_FNC_META_NAME, gtp)},
                                           {GRID_LUA_FNC_G_EVENT_TRIGGER_short, XAFTERX(GRID_LUA_FNC_META_NAME, get)},
                                           {GRID_LUA_FNC_G_ELEMENTNAME_SET_short, XAFTERX(GRID_LUA_FNC_META_NAME, gsen)},
                                           {GRID_LUA_FNC_G_ELEMENTNAME_GET_short, XAFTERX(GRID_LUA_FNC_META_NAME, ggen)},
                                           {NULL, NULL}};

void grid_ui_endless_state_init(struct grid_ui_endless_state* state, uint8_t adc_bit_depth, uint8_t button_adc_bit_depth, double button_threshold, double button_hysteresis) {

  state->adc_bit_depth = adc_bit_depth;

  grid_ui_button_state_init(&state->button, button_adc_bit_depth, button_threshold, button_hysteresis);
}

void grid_ui_element_endless_init(struct grid_ui_element* ele) {

  ele->type = GRID_PARAMETER_ELEMENT_ENDLESS;

  ele->primary_state = grid_platform_allocate_volatile(sizeof(struct grid_ui_endless_state));
  memset(ele->primary_state, 0, sizeof(struct grid_ui_endless_state));
  struct grid_ui_endless_state* end_state = (struct grid_ui_endless_state*)ele->primary_state;
  end_state->parent = ele;
  end_state->button.parent = ele;

  grid_ui_element_malloc_events(ele, 4);

  grid_ui_event_init(ele, 0, GRID_PARAMETER_EVENT_INIT, GRID_LUA_FNC_A_INIT_short, GRID_ACTIONSTRING_ENDLESS_INIT);
  grid_ui_event_init(ele, 1, GRID_PARAMETER_EVENT_ENDLESS, GRID_LUA_FNC_A_ENDLESS_short, GRID_ACTIONSTRING_ENDLESS_ENDLESS);
  grid_ui_event_init(ele, 2, GRID_PARAMETER_EVENT_BUTTON, GRID_LUA_FNC_A_BUTTON_short, GRID_ACTIONSTRING_ENDLESS_BUTTON);
  grid_ui_event_init(ele, 3, GRID_PARAMETER_EVENT_TIMER, GRID_LUA_FNC_A_TIMER_short, GRID_ACTIONSTRING_SYSTEM_TIMER);

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

uint8_t grid_ui_endless_update_trigger(struct grid_ui_element* ele, uint16_t norm, int16_t delta, double* delta_frac, uint64_t* last_real_time) {

  // limit lastrealtime
  uint64_t now = grid_platform_rtc_get_micros();
  uint64_t elapsed_us = grid_platform_rtc_get_diff(now, *last_real_time);
  elapsed_us = MIN(elapsed_us, GRID_PARAMETER_ELAPSED_LIMIT * MS_TO_US);

  // update lastrealtime
  *last_real_time = now;

  int32_t* template_parameter_list = ele->template_parameter_list;
  template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_ELAPSED_index] = elapsed_us / MS_TO_US;

  int16_t angle = norm * (3600. / 0x10000);
  template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_DIRECTION_index] = angle / 20;

  int32_t tmin = template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_MIN_index];
  int32_t tmax = template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_MAX_index];
  int32_t min = MIN(tmin, tmax);
  int32_t max = MAX(tmin, tmax);

  // invert delta if necessary
  if (tmin > tmax) {
    delta = -delta;
  }

  double elapsed_ms = clampu32(elapsed_us, 1000, 25000) / MS_TO_US;

  double vel_param = template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_VELOCITY_index] / 100.0;
  double sen_param = template_parameter_list[GRID_LUA_FNC_EP_ENDLESS_SENSITIVITY_index] / 100.0;

  double rate_of_change = abs(delta) / elapsed_ms;
  double vel_comp = (rate_of_change * rate_of_change * 0.084866 / 2000.0) * vel_param;
  double sen_comp = sen_param;
  double minmaxscale = (max - min) / (double)0xffff;
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

  grid_ui_event_state_set(eve, GRID_EVE_STATE_TRIG);

  return 1;
}

static uint16_t endless_ab_to_norm(uint16_t phase_a, uint16_t phase_b, uint8_t adc_bit_depth) {

  assert(adc_bit_depth <= 16);

  uint16_t a_norm = phase_a << (16 - adc_bit_depth);
  uint16_t b_norm = phase_b << (16 - adc_bit_depth);

  uint16_t a_rot = b_norm > 0x8000 ? a_norm >> 1 : 0xffff - (a_norm >> 1);
  uint16_t b_rot = a_norm > 0x8000 ? 0xffff - (b_norm >> 1) : b_norm >> 1;
  b_rot += 0xc000;

  uint16_t a_wei = (b_norm >= 0x8000 ? b_norm - 0x8000 : 0x7fff - b_norm) << 1;
  uint16_t b_wei = 0xffff - a_wei;

  return 0xffff - ((a_rot * (uint32_t)a_wei + b_rot * (uint32_t)b_wei) >> 16);
}

void grid_ui_endless_store_input(struct grid_ui_endless_state* state, struct grid_ui_endless_sample sample) {

  struct grid_ui_element* ele = state->parent;

  // Handle button input using embedded button state
  grid_ui_button_store_input(&state->button, sample.button_value);

  uint16_t norm = endless_ab_to_norm(sample.phase_a, sample.phase_b, state->adc_bit_depth);

  grid_ain_add_sample_raw(&grid_ain_state, ele->index, norm);

  bool stabilized = grid_ain_stabilized(&grid_ain_state, ele->index);

  if (!stabilized) {
    state->prev_norm = norm;
    return;
  }

  norm = grid_ain_endless_avg(&grid_ain_state, ele->index);

  int16_t diff = norm - state->prev_norm;

  if (diff < 182 && diff > -182) {
    return;
  }

  uint64_t* last = &state->encoder_last_real_time;
  grid_ui_endless_update_trigger(ele, norm, diff, &state->delta_vel_frac, last);

  state->prev_norm = norm;
}
