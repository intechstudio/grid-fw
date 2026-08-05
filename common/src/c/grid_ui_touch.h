#ifndef GRID_UI_TOUCH_H
#define GRID_UI_TOUCH_H

#include <stdint.h>

#include "grid_protocol.h"
#include "grid_ui.h"

struct touchinfo_t {
  uint8_t id;
  uint8_t event;
  uint16_t x;
  uint16_t y;
};

struct touchvalue_t {
  uint8_t id;
  uint8_t event;
  int32_t x;
  int32_t y;
};

struct grid_ui_touch_state {
  struct grid_ui_element* parent;
  uint8_t adc_bit_depth;
  int32_t prev_x[5];
  int32_t prev_y[5];
  uint8_t prev_event[5];
  int32_t x_min;
  int32_t y_min;
  int32_t x_max;
  int32_t y_max;
  struct touchvalue_t value;
  struct grid_swsr_t swsr;
};

void grid_ui_touch_state_init(struct grid_ui_touch_state* state, uint8_t adc_bit_depth);

int touch_get(lua_State* L);
int touch_set(lua_State* L);

void grid_ui_element_touch_init(struct grid_ui_element* ele);
void grid_ui_element_touch_template_parameter_init(struct grid_ui_template_buffer* buf);

static inline struct grid_ui_touch_state* grid_ui_touch_get_state(struct grid_ui_element* ele) { return (struct grid_ui_touch_state*)ele->primary_state; }

void grid_ui_touch_store_input(struct grid_ui_touch_state* state, struct touchinfo_t info);

// clang-format off

#define GRID_LUA_T_TYPE "Touch"

extern const luaL_Reg GRID_LUA_T_INDEX_META[];

#define GRID_LUA_T_META_init \
  GRID_LUA_T_TYPE " = { __index = {" \
  \
  "type = 'touch', " \
  \
  "post_init_cb = function (self) " \
  "self:" GRID_LUA_FNC_A_INIT_short "() " \
  "self:" GRID_LUA_FNC_A_TOUCH_short "() " \
  "end," \
  \
  GRID_LUA_FNC_ASSIGN_META_PAR1_RET("gen", GRID_LUA_FNC_G_ELEMENTNAME_short) "," \
  \
  GRID_LUA_FNC_ASSIGN_META_EVENT(INIT, TOUCH_INIT) \
  GRID_LUA_FNC_ASSIGN_META_EVENT(TOUCH, TOUCH_TOUCH) \
  \
  "}}"

#define GRID_ACTIONSTRING_TOUCH_INIT "--[[@cb]]--[[Touch Init]] self:txmi(0)self:txma(127)self:tymi(0)self:tyma(127)"

#define GRID_ACTIONSTRING_TOUCH_TOUCH "--[[@cb]] while self:pop()do print(self:tid(),self:tev(),self:txv(),self:tyv())end"

// clang-format on

#endif /* GRID_UI_TOUCH_H */
