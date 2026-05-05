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
                                          {GRID_LUA_FNC_T_TOUCH_AREA_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_T_TOUCH_AREA_index)},
                                          {GRID_LUA_FNC_T_TOUCH_X_MIN_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_T_TOUCH_X_MIN_index)},
                                          {GRID_LUA_FNC_T_TOUCH_X_MAX_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_T_TOUCH_X_MAX_index)},
                                          {GRID_LUA_FNC_T_TOUCH_Y_MIN_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_T_TOUCH_Y_MIN_index)},
                                          {GRID_LUA_FNC_T_TOUCH_Y_MAX_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_T_TOUCH_Y_MAX_index)},
                                          {GRID_LUA_FNC_G_TIMER_START_short, XAFTERX(GRID_LUA_FNC_META_NAME, gtt)},
                                          {GRID_LUA_FNC_G_TIMER_STOP_short, XAFTERX(GRID_LUA_FNC_META_NAME, gtp)},
                                          {GRID_LUA_FNC_G_EVENT_TRIGGER_short, XAFTERX(GRID_LUA_FNC_META_NAME, get)},
                                          {GRID_LUA_FNC_G_ELEMENTNAME_SET_short, XAFTERX(GRID_LUA_FNC_META_NAME, gsen)},
                                          {GRID_LUA_FNC_G_ELEMENTNAME_GET_short, XAFTERX(GRID_LUA_FNC_META_NAME, ggen)},
                                          {NULL, NULL}};

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
  template_parameter_list[GRID_LUA_FNC_T_TOUCH_AREA_index] = 0;
  template_parameter_list[GRID_LUA_FNC_T_TOUCH_X_MIN_index] = 0;
  template_parameter_list[GRID_LUA_FNC_T_TOUCH_X_MAX_index] = 127;
  template_parameter_list[GRID_LUA_FNC_T_TOUCH_Y_MIN_index] = 0;
  template_parameter_list[GRID_LUA_FNC_T_TOUCH_Y_MAX_index] = 127;
}

static int32_t touch_scale(uint16_t raw, int32_t out_min, int32_t out_max) {
  int32_t scaled = out_min + ((int64_t)raw * (out_max - out_min)) / 1023;
  return clampi32(scaled, MIN(out_min, out_max), MAX(out_min, out_max));
}

void grid_ui_touch_store_input(struct grid_ui_touch_state* state, uint16_t x, uint16_t y, uint8_t area) {

  struct grid_ui_element* ele = state->parent;
  int32_t* tpl = ele->template_parameter_list;

  state->x = x;
  state->y = y;
  state->area = area;

  x = 1023 - x;

  int32_t x_min = tpl[GRID_LUA_FNC_T_TOUCH_X_MIN_index];
  int32_t x_max = tpl[GRID_LUA_FNC_T_TOUCH_X_MAX_index];
  int32_t y_min = tpl[GRID_LUA_FNC_T_TOUCH_Y_MIN_index];
  int32_t y_max = tpl[GRID_LUA_FNC_T_TOUCH_Y_MAX_index];

  int32_t new_x = touch_scale(x, x_min, x_max);
  int32_t new_y = touch_scale(y, y_min, y_max);

  bool changed = (new_x != tpl[GRID_LUA_FNC_T_TOUCH_X_index]) || (new_y != tpl[GRID_LUA_FNC_T_TOUCH_Y_index]);

  tpl[GRID_LUA_FNC_T_TOUCH_X_index] = new_x;
  tpl[GRID_LUA_FNC_T_TOUCH_Y_index] = new_y;
  tpl[GRID_LUA_FNC_T_TOUCH_AREA_index] = area;

  if (changed) {
    struct grid_ui_event* eve = grid_ui_event_find(ele, GRID_PARAMETER_EVENT_TOUCH);
    grid_ui_event_state_set(eve, GRID_EVE_STATE_TRIG);
  }
}
