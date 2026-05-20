#ifndef GRID_UI_TOUCH_H
#define GRID_UI_TOUCH_H

#include <stdint.h>

#include "grid_protocol.h"
#include "grid_ui.h"

struct grid_ui_touch_state {
  struct grid_ui_element* parent;
  uint16_t x;
  uint16_t y;
  uint8_t area; // 0 = not pressed
};

void grid_ui_element_touch_init(struct grid_ui_element* ele);
void grid_ui_element_touch_template_parameter_init(struct grid_ui_template_buffer* buf);

static inline struct grid_ui_touch_state* grid_ui_touch_get_state(struct grid_ui_element* ele) { return (struct grid_ui_touch_state*)ele->primary_state; }

void grid_ui_touch_store_input(struct grid_ui_touch_state* state, uint16_t x, uint16_t y, uint8_t area);

#define GRID_LUA_T_TYPE "Touch"

extern const luaL_Reg GRID_LUA_T_INDEX_META[];

#define GRID_LUA_T_META_init                                                                                                                                                                           \
  GRID_LUA_T_TYPE " = { __index = {"                                                                                                                                                                   \
                  "type = 'touch', "                                                                                                                                                                   \
                  "post_init_cb = function (self) "                                                                                                                                                    \
                  "self:" GRID_LUA_FNC_A_INIT_short "() "                                                                                                                                              \
                  "self:" GRID_LUA_FNC_A_TOUCH_short "() "                                                                                                                                             \
                  "end," GRID_LUA_FNC_ASSIGN_META_PAR1_RET("gen", GRID_LUA_FNC_G_ELEMENTNAME_short) ","                                                                                                \
                                                                                                    "}}"

#define GRID_ACTIONSTRING_TOUCH_INIT "--[[@cb]]--[[Touch Init]]"

#define GRID_ACTIONSTRING_TOUCH_TOUCH "--[[@cb]]print(self:ind(),self:tsx(),self:tsy(),self:tar())"

#endif /* GRID_UI_TOUCH_H */
