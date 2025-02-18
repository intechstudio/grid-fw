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

#define GRID_LUA_FNC_L_DRAW_SWAP_short "ldsw"
#define GRID_LUA_FNC_L_DRAW_SWAP_human "draw_swap"

#define GRID_LUA_FNC_L_DRAW_PIXEL_short "ldpx"
#define GRID_LUA_FNC_L_DRAW_PIXEL_human "draw_pixel"

#define GRID_LUA_FNC_L_DRAW_LINE_short "ldl"
#define GRID_LUA_FNC_L_DRAW_LINE_human "draw_line"

#define GRID_LUA_FNC_L_DRAW_RECTANGLE_short "ldr"
#define GRID_LUA_FNC_L_DRAW_RECTANGLE_human "draw_rectangle"

#define GRID_LUA_FNC_L_DRAW_RECTANGLE_FILLED_short "ldrf"
#define GRID_LUA_FNC_L_DRAW_RECTANGLE_FILLED_human "draw_rectangle_filled"

#define GRID_LUA_FNC_L_DRAW_RECTANGLE_ROUNDED_short "ldrr"
#define GRID_LUA_FNC_L_DRAW_RECTANGLE_ROUNDED_human "draw_rectangle_rounded"

#define GRID_LUA_FNC_L_DRAW_RECTANGLE_ROUNDED_FILLED_short "ldrrf"
#define GRID_LUA_FNC_L_DRAW_RECTANGLE_ROUNDED_FILLED_human "draw_rectangle_rounded_filled"

#define GRID_LUA_FNC_L_DRAW_POLYGON_short "ldpo"
#define GRID_LUA_FNC_L_DRAW_POLYGON_human "draw_polygon"

#define GRID_LUA_FNC_L_DRAW_POLYGON_FILLED_short "ldpof"
#define GRID_LUA_FNC_L_DRAW_POLYGON_FILLED_human "draw_polygon_filled"

#define GRID_LUA_FNC_L_DRAW_TEXT_short "ldt"
#define GRID_LUA_FNC_L_DRAW_TEXT_human "draw_text"

#define GRID_LUA_FNC_L_DRAW_DEMO_short "ldd"
#define GRID_LUA_FNC_L_DRAW_DEMO_human "draw_demo"

// clang-format off

#define GRID_LUA_FNC_ASSIGN_META_DRAW(key, val) \
  key " = function (self, ...) " \
  "local screen_idx = self:" GRID_LUA_FNC_L_SCREEN_INDEX_short "(); " \
  val "(screen_idx, ...) end"

// clang-format on

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
  GRID_LUA_FNC_ASSIGN_META_DRAW(GRID_LUA_FNC_L_DRAW_SWAP_short, GRID_LUA_FNC_G_GUI_DRAW_SWAP_short) "," \
  GRID_LUA_FNC_ASSIGN_META_DRAW(GRID_LUA_FNC_L_DRAW_PIXEL_short, GRID_LUA_FNC_G_GUI_DRAW_PIXEL_short) "," \
  GRID_LUA_FNC_ASSIGN_META_DRAW(GRID_LUA_FNC_L_DRAW_LINE_short, GRID_LUA_FNC_G_GUI_DRAW_LINE_short) "," \
  GRID_LUA_FNC_ASSIGN_META_DRAW(GRID_LUA_FNC_L_DRAW_RECTANGLE_short, GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_short) "," \
  GRID_LUA_FNC_ASSIGN_META_DRAW(GRID_LUA_FNC_L_DRAW_RECTANGLE_FILLED_short, GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_FILLED_short) "," \
  GRID_LUA_FNC_ASSIGN_META_DRAW(GRID_LUA_FNC_L_DRAW_RECTANGLE_ROUNDED_short, GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_ROUNDED_short) "," \
  GRID_LUA_FNC_ASSIGN_META_DRAW(GRID_LUA_FNC_L_DRAW_RECTANGLE_ROUNDED_FILLED_short, GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_ROUNDED_FILLED_short) "," \
  GRID_LUA_FNC_ASSIGN_META_DRAW(GRID_LUA_FNC_L_DRAW_POLYGON_short, GRID_LUA_FNC_G_GUI_DRAW_POLYGON_short) "," \
  GRID_LUA_FNC_ASSIGN_META_DRAW(GRID_LUA_FNC_L_DRAW_POLYGON_FILLED_short, GRID_LUA_FNC_G_GUI_DRAW_POLYGON_FILLED_short) "," \
  GRID_LUA_FNC_ASSIGN_META_DRAW(GRID_LUA_FNC_L_DRAW_TEXT_short, GRID_LUA_FNC_G_GUI_DRAW_TEXT_short) "," \
  GRID_LUA_FNC_ASSIGN_META_DRAW(GRID_LUA_FNC_L_DRAW_DEMO_short, GRID_LUA_FNC_G_GUI_DRAW_DEMO_short) "," \
  \
  "}}"
// clang-format on

#define GRID_ACTIONSTRING_LCD_INIT                                                                                                                                                                     \
  "<?lua --[[@cb]] x = {0, 0, 38, 64, 64, 65, 104, 130, 130, 131, 170, 183, 183, 145, 119, 119, 118, 79, 53, 53, 52, 13} y = {42, 38, 0, 0, 39, 39, 0, 0, 39, 39, 0, 0, 4, 42, 42, 3, 3, 42, 42, 3, "  \
  "3, 42} self:ldrf(0,0,320,240,{0,0,0}) self:ldsw() ?>"

#endif
