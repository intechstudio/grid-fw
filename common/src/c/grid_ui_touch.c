#include "grid_ui_touch.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "grid_lua_api.h"
#include "grid_math.h"
#include "grid_platform.h"
#include "grid_ui_system.h"

GRID_LUA_FNC_ELEMENT_POP(touch_pop, struct grid_ui_touch_state, swsr, value)

int touch_get(lua_State* L) {

  struct grid_ui_touch_state* state = grid_ui_lua_element_address(L, 1)->primary_state;

  switch (luaL_checkinteger(L, 2)) {
    GRID_LUA_GETV_CASE(GRID_LUA_FNC_T_ELEMENT_INDEX_index, state->parent->index)
    GRID_LUA_GETV_CASE(GRID_LUA_FNC_T_LED_INDEX_index, state->parent->index)
    GRID_LUA_GETV_CASE(GRID_LUA_FNC_T_LED_WIDTH_index, 9)
    GRID_LUA_GETV_CASE(GRID_LUA_FNC_T_TOUCH_ID_index, state->value.id)
    GRID_LUA_GETV_CASE(GRID_LUA_FNC_T_TOUCH_EVENT_index, state->value.event)
    GRID_LUA_GETV_CASE(GRID_LUA_FNC_T_TOUCH_X_VALUE_index, state->value.x)
    GRID_LUA_GETV_CASE(GRID_LUA_FNC_T_TOUCH_Y_VALUE_index, state->value.y)
  default:
    luaL_argcheck(L, false, 2, "inaccessible");
  }

  return 1;
}

int touch_set(lua_State* L) {

  struct grid_ui_touch_state* state = grid_ui_lua_element_address(L, 1)->primary_state;

  switch (luaL_checkinteger(L, 2)) {
    GRID_LUA_SETV_CASE(GRID_LUA_FNC_T_TOUCH_X_MIN_index, state->x_min)
    GRID_LUA_SETV_CASE(GRID_LUA_FNC_T_TOUCH_Y_MIN_index, state->y_min)
    GRID_LUA_SETV_CASE(GRID_LUA_FNC_T_TOUCH_X_MAX_index, state->x_max)
    GRID_LUA_SETV_CASE(GRID_LUA_FNC_T_TOUCH_Y_MAX_index, state->y_max)
  default:
    luaL_argcheck(L, false, 2, "inaccessible");
  }

  return 0;
}

const luaL_Reg GRID_LUA_T_INDEX_META[] = {{GRID_LUA_FNC_T_ELEMENT_INDEX_short, XAFTERX(GRID_LUA_FNC_GSV_NAME, GRID_LUA_FNC_T_ELEMENT_INDEX_index)},
                                          {GRID_LUA_FNC_T_LED_INDEX_short, XAFTERX(GRID_LUA_FNC_GSV_NAME, GRID_LUA_FNC_T_LED_INDEX_index)},
                                          {GRID_LUA_FNC_T_LED_WIDTH_short, XAFTERX(GRID_LUA_FNC_GSV_NAME, GRID_LUA_FNC_T_LED_WIDTH_index)},
                                          {GRID_LUA_FNC_T_TOUCH_X_MIN_short, XAFTERX(GRID_LUA_FNC_GSV_NAME, GRID_LUA_FNC_T_TOUCH_X_MIN_index)},
                                          {GRID_LUA_FNC_T_TOUCH_Y_MIN_short, XAFTERX(GRID_LUA_FNC_GSV_NAME, GRID_LUA_FNC_T_TOUCH_Y_MIN_index)},
                                          {GRID_LUA_FNC_T_TOUCH_X_MAX_short, XAFTERX(GRID_LUA_FNC_GSV_NAME, GRID_LUA_FNC_T_TOUCH_X_MAX_index)},
                                          {GRID_LUA_FNC_T_TOUCH_Y_MAX_short, XAFTERX(GRID_LUA_FNC_GSV_NAME, GRID_LUA_FNC_T_TOUCH_Y_MAX_index)},
                                          {GRID_LUA_FNC_T_TOUCH_ID_short, XAFTERX(GRID_LUA_FNC_GSV_NAME, GRID_LUA_FNC_T_TOUCH_ID_index)},
                                          {GRID_LUA_FNC_T_TOUCH_EVENT_short, XAFTERX(GRID_LUA_FNC_GSV_NAME, GRID_LUA_FNC_T_TOUCH_EVENT_index)},
                                          {GRID_LUA_FNC_T_TOUCH_X_VALUE_short, XAFTERX(GRID_LUA_FNC_GSV_NAME, GRID_LUA_FNC_T_TOUCH_X_VALUE_index)},
                                          {GRID_LUA_FNC_T_TOUCH_Y_VALUE_short, XAFTERX(GRID_LUA_FNC_GSV_NAME, GRID_LUA_FNC_T_TOUCH_Y_VALUE_index)},
                                          {GRID_LUA_FNC_G_TIMER_START_short, XAFTERX(GRID_LUA_FNC_META_NAME, gtt)},
                                          {GRID_LUA_FNC_G_TIMER_STOP_short, XAFTERX(GRID_LUA_FNC_META_NAME, gtp)},
                                          {GRID_LUA_FNC_G_EVENT_TRIGGER_short, XAFTERX(GRID_LUA_FNC_META_NAME, get)},
                                          {GRID_LUA_FNC_G_ELEMENTNAME_SET_short, XAFTERX(GRID_LUA_FNC_META_NAME, gsen)},
                                          {GRID_LUA_FNC_G_ELEMENTNAME_GET_short, XAFTERX(GRID_LUA_FNC_META_NAME, ggen)},
                                          {"touch_pop", touch_pop},
                                          {"getv", touch_get},
                                          {"setv", touch_set},
                                          {NULL, NULL}};

void grid_ui_touch_state_alloc(struct grid_ui_touch_state* state) { assert(grid_swsr_malloc(&state->swsr, 10 * sizeof(struct touchvalue_t)) == 0); }

void grid_ui_touch_state_init(struct grid_ui_touch_state* state, uint8_t adc_bit_depth) { state->adc_bit_depth = adc_bit_depth; }

bool grid_ui_touch_state_any(struct grid_ui_touch_state* state) { return grid_swsr_readable(&state->swsr, sizeof(state->value)); }

void grid_ui_element_touch_init(struct grid_ui_element* ele) {

  ele->type = GRID_PARAMETER_ELEMENT_TOUCH;

  ele->primary_state = grid_platform_allocate_volatile(sizeof(struct grid_ui_touch_state));
  memset(ele->primary_state, 0, sizeof(struct grid_ui_touch_state));
  grid_ui_touch_state_alloc(ele->primary_state);
  ((struct grid_ui_touch_state*)ele->primary_state)->parent = ele;

  grid_ui_element_malloc_events(ele, 3);
  grid_ui_event_init(ele, 0, GRID_PARAMETER_EVENT_INIT, GRID_LUA_FNC_A_INIT_short, GRID_ACTIONSTRING_TOUCH_INIT);
  grid_ui_event_init(ele, 1, GRID_PARAMETER_EVENT_TOUCH, GRID_LUA_FNC_A_TOUCH_short, GRID_ACTIONSTRING_TOUCH_TOUCH);
  grid_ui_event_init(ele, 2, GRID_PARAMETER_EVENT_TIMER, GRID_LUA_FNC_A_TIMER_short, GRID_ACTIONSTRING_SYSTEM_TIMER);
}

static double lerp(double a, double b, double x) { return a * (1.0 - x) + (b * x); }

void grid_ui_touch_store_input(struct grid_ui_touch_state* state, struct touchinfo_t info) {

  struct grid_ui_element* ele = state->parent;

  int32_t tmin[2] = {state->x_min, state->y_min};
  int32_t tmax[2] = {state->x_max, state->y_max};
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

  struct touchvalue_t value = {info.id, info.event, new[0], new[1]};

  assert(value.id < 5);

  bool delta_x = value.x != state->prev_x[value.id];
  bool delta_y = value.y != state->prev_y[value.id];
  bool delta_event = value.event != state->prev_event[value.id];

  if (delta_x || delta_y || delta_event) {

    state->prev_x[value.id] = value.x;
    state->prev_y[value.id] = value.y;
    state->prev_event[value.id] = value.event;

    uint8_t diff_res = state->adc_bit_depth - 7;
    // TODO write state to be read by event render

    if (grid_swsr_writable(&state->swsr, sizeof(struct touchvalue_t))) {
      grid_swsr_write(&state->swsr, &value, sizeof(struct touchvalue_t));
    }
  }
}
