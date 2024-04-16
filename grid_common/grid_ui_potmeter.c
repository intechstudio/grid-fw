#include "grid_ui_potmeter.h"

#include <stdint.h>
#include "grid_ui.h"
#include "grid_ain.h"
#include "grid_lua_api.h"
#include "grid_protocol.h"
#include <stdlib.h>
#include <string.h>

extern uint8_t grid_platform_get_adc_bit_depth();

extern void grid_platform_printf(char const* fmt, ...);


const char grid_ui_potmeter_init_actionstring[] = GRID_ACTIONSTRING_INIT_POT;
const char grid_ui_potmeter_potmeterchange_actionstring[] = GRID_ACTIONSTRING_AC;
const char grid_ui_potmeter_timer_actionstring[] = GRID_ACTIONSTRING_TIMER;

void grid_ui_element_potentiometer_init(struct grid_ui_element* ele){

  ele->type = GRID_UI_ELEMENT_POTENTIOMETER;

  ele->event_list_length = 3;

  ele->event_list = malloc(ele->event_list_length * sizeof(struct grid_ui_event));

  grid_ui_event_init(ele, 0, GRID_UI_EVENT_INIT, GRID_LUA_FNC_ACTION_INIT_short, grid_ui_potmeter_init_actionstring); // Element Initialization Event
  grid_ui_event_init(ele, 1, GRID_UI_EVENT_AC, GRID_LUA_FNC_ACTION_POTMETERCHANGE_short, grid_ui_potmeter_potmeterchange_actionstring); // Absolute Value Change (7bit)
  grid_ui_event_init(ele, 2, GRID_UI_EVENT_TIMER, GRID_LUA_FNC_ACTION_TIMER_short, grid_ui_potmeter_timer_actionstring);

  ele->template_initializer = &grid_ui_element_potmeter_template_parameter_init;
  ele->template_parameter_list_length = GRID_LUA_FNC_P_LIST_length;

  ele->event_clear_cb = &grid_ui_element_potmeter_event_clear_cb;
  ele->page_change_cb = &grid_ui_element_potmeter_page_change_cb;


}


void grid_ui_element_potmeter_template_parameter_init(struct grid_ui_template_buffer* buf) {

  uint8_t element_index = buf->parent->index;
  int32_t* template_parameter_list = buf->template_parameter_list;

  template_parameter_list[GRID_LUA_FNC_P_ELEMENT_INDEX_index] = element_index;
  template_parameter_list[GRID_LUA_FNC_P_POTMETER_NUMBER_index] = element_index;
  template_parameter_list[GRID_LUA_FNC_P_POTMETER_VALUE_index] = 0;
  template_parameter_list[GRID_LUA_FNC_P_POTMETER_MIN_index] = 0;
  template_parameter_list[GRID_LUA_FNC_P_POTMETER_MAX_index] = 127;
  template_parameter_list[GRID_LUA_FNC_P_POTMETER_MODE_index] = 7;
  template_parameter_list[GRID_LUA_FNC_P_POTMETER_ELAPSED_index] = 0;
  template_parameter_list[GRID_LUA_FNC_P_POTMETER_STATE_index] = 0;

  // Load AIN value to VALUE register

  int32_t resolution = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MODE_index];

  if (resolution < 1) {
    resolution = 1;
  } else if (resolution > 12) {
    resolution = 12;
  }

  int32_t min = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MIN_index];
  int32_t max = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MAX_index];

  int32_t next = grid_ain_get_average_scaled(&grid_ain_state, element_index, 16, resolution, min, max);
  template_parameter_list[GRID_LUA_FNC_P_POTMETER_VALUE_index] = next;
}

void grid_ui_element_potmeter_event_clear_cb(struct grid_ui_event* eve) {}

void grid_ui_element_potmeter_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new) {

  uint8_t element_index = ele->index;
  int32_t* template_parameter_list = ele->template_parameter_list;

  int32_t resolution = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MODE_index];

  if (resolution < 1) {
    resolution = 1;
  } else if (resolution > 12) {
    resolution = 12;
  }

  int32_t min = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MIN_index];
  int32_t max = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MAX_index];

  int32_t next = grid_ain_get_average_scaled(&grid_ain_state, element_index, grid_platform_get_adc_bit_depth(), resolution, min, max);
  template_parameter_list[GRID_LUA_FNC_P_POTMETER_VALUE_index] = next;
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
