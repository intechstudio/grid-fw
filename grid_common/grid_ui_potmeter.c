#include "grid_ui_potmeter.h"

#include <stdlib.h>
#include <string.h>

#include "grid_ain.h"
#include "grid_math.h"
#include "grid_platform.h"
#include "grid_sys.h"
#include "grid_ui_system.h"

extern uint8_t grid_platform_get_adc_bit_depth();

extern void grid_platform_printf(char const* fmt, ...);

const char grid_ui_potmeter_init_actionstring[] = GRID_ACTIONSTRING_POTMETER_INIT;
const char grid_ui_potmeter_potmeterchange_actionstring[] = GRID_ACTIONSTRING_POTMETER_POTMETER;
const char grid_ui_potmeter_timer_actionstring[] = GRID_ACTIONSTRING_SYSTEM_TIMER;

void grid_ui_element_potmeter_init(struct grid_ui_element* ele) {

  ele->type = GRID_PARAMETER_ELEMENT_POTMETER;

  grid_ui_element_malloc_events(ele, 3);

  grid_ui_event_init(ele, 0, GRID_PARAMETER_EVENT_INIT, GRID_LUA_FNC_A_INIT_short, grid_ui_potmeter_init_actionstring);                   // Element Initialization Event
  grid_ui_event_init(ele, 1, GRID_PARAMETER_EVENT_POTMETER, GRID_LUA_FNC_A_POTMETER_short, grid_ui_potmeter_potmeterchange_actionstring); // Absolute Value Change (7bit)
  grid_ui_event_init(ele, 2, GRID_PARAMETER_EVENT_TIMER, GRID_LUA_FNC_A_TIMER_short, grid_ui_potmeter_timer_actionstring);

  ele->template_initializer = &grid_ui_element_potmeter_template_parameter_init;
  ele->template_parameter_list_length = GRID_LUA_FNC_P_LIST_length;
  ele->template_parameter_element_position_index_1 = GRID_LUA_FNC_P_POTMETER_STATE_index;
  ele->template_parameter_element_position_index_2 = GRID_LUA_FNC_P_POTMETER_STATE_index;

  ele->template_parameter_index_value[0] = GRID_LUA_FNC_P_POTMETER_VALUE_index;
  ele->template_parameter_index_min[0] = GRID_LUA_FNC_P_POTMETER_MIN_index;
  ele->template_parameter_index_max[0] = GRID_LUA_FNC_P_POTMETER_MAX_index;
  ele->template_parameter_index_value[1] = ele->template_parameter_index_value[0];
  ele->template_parameter_index_min[1] = ele->template_parameter_index_min[0];
  ele->template_parameter_index_max[1] = ele->template_parameter_index_max[0];

  ele->event_clear_cb = &grid_ui_element_potmeter_event_clear_cb;
  ele->page_change_cb = &grid_ui_element_potmeter_page_change_cb;
}

void grid_ui_element_potmeter_update_value(int32_t* template_parameter_list, uint8_t input_channel, uint8_t adc_bit_depth) {

  int32_t resolution = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MODE_index];
  resolution = clampi32(resolution, 1, 12);

  int32_t tmin = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MIN_index];
  int32_t tmax = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MAX_index];
  int32_t min = MIN(tmin, tmax);
  int32_t max = MAX(tmin, tmax);

  int32_t new_value = grid_ain_get_average_scaled(&grid_ain_state, input_channel, adc_bit_depth, resolution, min, max);

  if (tmin > tmax) {
    new_value = mirrori32(new_value, min, max);
  }

  template_parameter_list[GRID_LUA_FNC_P_POTMETER_VALUE_index] = new_value;
}

void grid_ui_element_potmeter_template_parameter_init(struct grid_ui_template_buffer* buf) {

  uint8_t element_index = buf->parent->index;
  int32_t* template_parameter_list = buf->template_parameter_list;

  template_parameter_list[GRID_LUA_FNC_P_ELEMENT_INDEX_index] = element_index;
  template_parameter_list[GRID_LUA_FNC_P_LED_INDEX_index] = element_index;
  template_parameter_list[GRID_LUA_FNC_P_POTMETER_VALUE_index] = 0;
  template_parameter_list[GRID_LUA_FNC_P_POTMETER_MIN_index] = 0;
  template_parameter_list[GRID_LUA_FNC_P_POTMETER_MAX_index] = 127;
  template_parameter_list[GRID_LUA_FNC_P_POTMETER_MODE_index] = 7;
  template_parameter_list[GRID_LUA_FNC_P_POTMETER_ELAPSED_index] = 0;
  template_parameter_list[GRID_LUA_FNC_P_POTMETER_STATE_index] = 0;

  uint8_t adc_bit_depth = grid_platform_get_adc_bit_depth();
  grid_ui_element_potmeter_update_value(template_parameter_list, element_index, adc_bit_depth);
}

void grid_ui_element_potmeter_event_clear_cb(struct grid_ui_event* eve) {}

void grid_ui_element_potmeter_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new) {

  uint8_t element_index = ele->index;
  int32_t* template_parameter_list = ele->template_parameter_list;

  uint8_t adc_bit_depth = grid_platform_get_adc_bit_depth();
  grid_ui_element_potmeter_update_value(template_parameter_list, element_index, adc_bit_depth);
}

void grid_ui_potmeter_store_input(struct grid_ui_element* ele, uint8_t input_channel, uint64_t* last_real_time, uint16_t value, uint8_t adc_bit_depth) {

  assert(ele);

  // const uint16_t adc_max_value = (1 << adc_bit_depth) - 1;

  int32_t* template_parameter_list = ele->template_parameter_list;

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

    grid_ui_element_potmeter_update_value(template_parameter_list, ele->index, adc_bit_depth);

    // for display in editor
    int32_t state = grid_ain_get_average_scaled(&grid_ain_state, input_channel, adc_bit_depth, resolution, 0, 127);
    template_parameter_list[GRID_LUA_FNC_P_POTMETER_STATE_index] = state;

    struct grid_ui_event* eve = grid_ui_event_find(ele, GRID_PARAMETER_EVENT_POTMETER);

    if (grid_ain_stabilized(&grid_ain_state, input_channel)) {
      grid_ui_event_state_set(eve, GRID_EVE_STATE_TRIG);
    }
  }
}
