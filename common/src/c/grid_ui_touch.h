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

struct grid_ui_touch_state {
  struct grid_ui_element* parent;
  uint8_t adc_bit_depth;
  int32_t prev_x[5];
  int32_t prev_y[5];
  uint8_t prev_event[5];
  struct grid_swsr_t swsr;
  struct touchinfo_t info;
};

void grid_ui_touch_state_init(struct grid_ui_touch_state* state, uint8_t adc_bit_depth);

int luaopen_touch(lua_State* L);

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

#define GRID_ACTIONSTRING_TOUCH_INIT "--[[@cb]] function touch_dist(led,x,y)local lw=self:lwi()local lp={led%lw+0.0,(lw-1)-led//lw+0.0}local tp={x*(lw-1),y*(lw-1)}local d={tp[1]-lp[1],tp[2]-lp[2]}return math.sqrt(d[1]*d[1]+d[2]*d[2])end local leds=self:lwi()*self:lwi()for i=0,leds-1 do gln(i,1,0,0,0)gld(i,1,16,16,16)glx(i,1,32,32,32)glp(i,1,0)end"

#define GRID_ACTIONSTRING_TOUCH_TOUCH "--[[@cb]] local evts=ges(ele[0])while touch.pop(evts)do print(touch.id(evts),touch.event(evts),touch.x(evts),touch.y(evts))end"

// clang-format on

#endif /* GRID_UI_TOUCH_H */
