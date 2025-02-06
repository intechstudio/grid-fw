#pragma once

#ifndef GRID_UI_LCD_H_INCLUDED
#define GRID_UI_LCD_H_INCLUDED

#include "grid_ui.h"
#include <stdint.h>

void grid_ui_element_lcd_init(struct grid_ui_element* ele, template_init_t initializer);

void grid_ui_element_lcd_template_parameter_init(struct grid_ui_template_buffer* buf);

#define GRID_LUA_FNC_L_ELEMENT_INDEX_index 0
#define GRID_LUA_FNC_L_ELEMENT_INDEX_short "ind"
#define GRID_LUA_FNC_L_ELEMENT_INDEX_human "element_index"

#define GRID_LUA_FNC_L_SCREEN_INDEX_index 1
#define GRID_LUA_FNC_L_SCREEN_INDEX_short "lin"
#define GRID_LUA_FNC_L_SCREEN_INDEX_human "screen_index"

#define GRID_LUA_FNC_L_SCREEN_WIDTH_index 2
#define GRID_LUA_FNC_L_SCREEN_WIDTH_short "lsw"
#define GRID_LUA_FNC_L_SCREEN_WIDTH_human "screen_width"

#define GRID_LUA_FNC_L_SCREEN_HEIGHT_index 3
#define GRID_LUA_FNC_L_SCREEN_HEIGHT_short "lsh"
#define GRID_LUA_FNC_L_SCREEN_HEIGHT_human "screen_height"

#define GRID_LUA_FNC_L_LIST_length 4

#define GRID_LUA_FNC_L_DRAW_PIXEL_short "ldp"
#define GRID_LUA_FNC_L_DRAW_PIXEL_human "draw_pixel"

#define GRID_LUA_FNC_ASSIGN_META_DRAW(key, val)                                                                                                                                                        \
  key " = function (self, ...) "                                                                                                                                                                       \
      "local screen_idx = self:" GRID_LUA_FNC_L_SCREEN_INDEX_short "(); " val "(screen_idx, ...) end"

// LCD init function
// clang-format off
#define GRID_LUA_L_META_init                                                                                                                                                                           \
  "lcd_meta = { __index = {" \
  \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_L_ELEMENT_INDEX_short, GRID_LUA_FNC_L_ELEMENT_INDEX_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_L_SCREEN_INDEX_short, GRID_LUA_FNC_L_SCREEN_INDEX_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_L_SCREEN_WIDTH_short, GRID_LUA_FNC_L_SCREEN_WIDTH_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_L_SCREEN_HEIGHT_short, GRID_LUA_FNC_L_SCREEN_HEIGHT_index) "," \
  \
  GRID_LUA_FNC_ASSIGN_META_UNDEF(GRID_LUA_FNC_A_INIT_short) "," \
  \
  GRID_LUA_FNC_ASSIGN_META_PAR1("gtt", GRID_LUA_FNC_G_TIMER_START_short) "," \
  GRID_LUA_FNC_ASSIGN_META_PAR0("gtp", GRID_LUA_FNC_G_TIMER_STOP_short) "," \
  GRID_LUA_FNC_ASSIGN_META_PAR1("get", GRID_LUA_FNC_G_EVENT_TRIGGER_short) "," \
  \
  GRID_LUA_FNC_ASSIGN_META_DRAW(GRID_LUA_FNC_L_DRAW_PIXEL_short, GRID_LUA_FNC_G_GUI_DRAW_PIXEL_short) "," \
  \
  "}}"
// clang-format on

#define GRID_ACTIONSTRING_LCD_INIT "<?lua --[[@cb]] --[[lcd init]] ?>"

#endif
