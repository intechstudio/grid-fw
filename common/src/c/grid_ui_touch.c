#include "grid_ui_touch.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "grid_lua_api.h"
#include "grid_math.h"
#include "grid_platform.h"
#include "grid_ui_system.h"

const luaL_Reg GRID_LUA_T_INDEX_META[] = {{GRID_LUA_FNC_T_ELEMENT_INDEX_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_T_ELEMENT_INDEX_index)},
                                          {GRID_LUA_FNC_T_LED_INDEX_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_T_LED_INDEX_index)},
                                          {GRID_LUA_FNC_T_TOUCH_X_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_T_TOUCH_X_index)},
                                          {GRID_LUA_FNC_T_TOUCH_Y_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_T_TOUCH_Y_index)},
                                          {GRID_LUA_FNC_T_TOUCH_X_MIN_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_T_TOUCH_X_MIN_index)},
                                          {GRID_LUA_FNC_T_TOUCH_X_MAX_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_T_TOUCH_X_MAX_index)},
                                          {GRID_LUA_FNC_T_TOUCH_Y_MIN_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_T_TOUCH_Y_MIN_index)},
                                          {GRID_LUA_FNC_T_TOUCH_Y_MAX_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_T_TOUCH_Y_MAX_index)},
                                          {GRID_LUA_FNC_T_TOUCH_X_STATE_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_T_TOUCH_X_STATE_index)},
                                          {GRID_LUA_FNC_T_TOUCH_Y_STATE_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_T_TOUCH_Y_STATE_index)},
                                          {GRID_LUA_FNC_T_LED_WIDTH_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_T_LED_WIDTH_index)},
                                          {GRID_LUA_FNC_G_TIMER_START_short, XAFTERX(GRID_LUA_FNC_META_NAME, gtt)},
                                          {GRID_LUA_FNC_G_TIMER_STOP_short, XAFTERX(GRID_LUA_FNC_META_NAME, gtp)},
                                          {GRID_LUA_FNC_G_EVENT_TRIGGER_short, XAFTERX(GRID_LUA_FNC_META_NAME, get)},
                                          {GRID_LUA_FNC_G_ELEMENTNAME_SET_short, XAFTERX(GRID_LUA_FNC_META_NAME, gsen)},
                                          {GRID_LUA_FNC_G_ELEMENTNAME_GET_short, XAFTERX(GRID_LUA_FNC_META_NAME, ggen)},
                                          {NULL, NULL}};

void grid_ui_touch_state_init(struct grid_ui_touch_state* state, uint8_t adc_bit_depth) { state->adc_bit_depth = adc_bit_depth; }

void grid_ui_element_touch_init(struct grid_ui_element* ele) {

  ele->type = GRID_PARAMETER_ELEMENT_TOUCH;

  ele->primary_state = grid_platform_allocate_volatile(sizeof(struct grid_ui_touch_state));
  memset(ele->primary_state, 0, sizeof(struct grid_ui_touch_state));
  ((struct grid_ui_touch_state*)ele->primary_state)->parent = ele;

  grid_ui_element_malloc_events(ele, 3);
  grid_ui_event_init(ele, 0, GRID_PARAMETER_EVENT_INIT, GRID_LUA_FNC_A_INIT_short, GRID_ACTIONSTRING_TOUCH_INIT);
  grid_ui_event_init(ele, 1, GRID_PARAMETER_EVENT_TOUCH, GRID_LUA_FNC_A_TOUCH_short, GRID_ACTIONSTRING_TOUCH_TOUCH);
  grid_ui_event_init(ele, 2, GRID_PARAMETER_EVENT_TIMER, GRID_LUA_FNC_A_TIMER_short, GRID_ACTIONSTRING_SYSTEM_TIMER);

  ele->template_initializer = &grid_ui_element_touch_template_parameter_init;
  ele->template_parameter_list_length = GRID_LUA_FNC_T_LIST_length;

  ele->template_parameter_element_position_index_1 = GRID_LUA_FNC_T_TOUCH_X_STATE_index;
  ele->template_parameter_element_position_index_2 = GRID_LUA_FNC_T_TOUCH_Y_STATE_index;

  ele->event_clear_cb = NULL;
  ele->page_change_cb = NULL;
}

void grid_ui_element_touch_template_parameter_init(struct grid_ui_template_buffer* buf) {

  uint8_t element_index = buf->parent->index;
  int32_t* template_parameter_list = buf->template_parameter_list;

  template_parameter_list[GRID_LUA_FNC_T_ELEMENT_INDEX_index] = element_index;
  template_parameter_list[GRID_LUA_FNC_T_LED_INDEX_index] = element_index;
  template_parameter_list[GRID_LUA_FNC_T_TOUCH_X_index] = 0;
  template_parameter_list[GRID_LUA_FNC_T_TOUCH_Y_index] = 0;
  template_parameter_list[GRID_LUA_FNC_T_TOUCH_X_MIN_index] = 0;
  template_parameter_list[GRID_LUA_FNC_T_TOUCH_X_MAX_index] = 127;
  template_parameter_list[GRID_LUA_FNC_T_TOUCH_Y_MIN_index] = 0;
  template_parameter_list[GRID_LUA_FNC_T_TOUCH_Y_MAX_index] = 127;
  template_parameter_list[GRID_LUA_FNC_T_TOUCH_X_STATE_index] = 0;
  template_parameter_list[GRID_LUA_FNC_T_TOUCH_Y_STATE_index] = 0;
  template_parameter_list[GRID_LUA_FNC_T_LED_WIDTH_index] = 9;
}

static double lerp(double a, double b, double x) { return a * (1.0 - x) + (b * x); }

void grid_ui_touch_store_input(struct grid_ui_touch_state* state, struct touchinfo_t info) {

  struct grid_ui_element* ele = state->parent;
  int32_t* tpl = ele->template_parameter_list;

  int32_t tmin[2] = {tpl[GRID_LUA_FNC_T_TOUCH_X_MIN_index], tpl[GRID_LUA_FNC_T_TOUCH_Y_MIN_index]};
  int32_t tmax[2] = {tpl[GRID_LUA_FNC_T_TOUCH_X_MAX_index], tpl[GRID_LUA_FNC_T_TOUCH_Y_MAX_index]};
  int32_t min[2] = {MIN(tmin[0], tmax[0]), MIN(tmin[1], tmax[1])};
  int32_t max[2] = {MAX(tmin[0], tmax[0]), MAX(tmin[1], tmax[1])};

  int32_t new[2] = {info.x, info.y};
  for (int i = 0; i < 2; ++i) {

    new[i] = lerp(min[i], max[i] + 1, new[i] / (double)(1 << state->adc_bit_depth));

    new[i] = clampi32(new[i], min[i], max[i]);

    if (tmin[i] > tmax[i]) {
      new[i] = mirrori32(new[i], min[i], max[i]);
    }
  }

  bool x_delta = new[0] != tpl[GRID_LUA_FNC_T_TOUCH_X_index];
  bool y_delta = new[1] != tpl[GRID_LUA_FNC_T_TOUCH_Y_index];
  if (x_delta || y_delta || info.event != tpl[GRID_LUA_FNC_T_TOUCH_EVENT_index]) {

    tpl[GRID_LUA_FNC_T_TOUCH_EVENT_index] = info.event;
    tpl[GRID_LUA_FNC_T_TOUCH_X_index] = new[0];
    tpl[GRID_LUA_FNC_T_TOUCH_Y_index] = new[1];

    uint8_t diff_res = state->adc_bit_depth - 7;
    tpl[GRID_LUA_FNC_T_TOUCH_X_STATE_index] = info.x >> diff_res;
    tpl[GRID_LUA_FNC_T_TOUCH_Y_STATE_index] = info.y >> diff_res;

    struct grid_ui_event* eve = grid_ui_event_find(ele, GRID_PARAMETER_EVENT_TOUCH);
    grid_ui_event_state_set(eve, GRID_EVE_STATE_TRIG);
  }
}
