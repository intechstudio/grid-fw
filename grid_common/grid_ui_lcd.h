#ifndef GRID_UI_LCD_H
#define GRID_UI_LCD_H

#include <stdint.h>

#include "grid_protocol.h"
#include "grid_ui.h"

void grid_ui_element_lcd_init(struct grid_ui_element* ele, template_init_t initializer);

void grid_ui_element_lcd_template_parameter_init(struct grid_ui_template_buffer* buf);

#define GRID_LUA_FNC_L_DRAW_SWAP_short "ldsw"
#define GRID_LUA_FNC_L_DRAW_SWAP_human "draw_swap"
#define GRID_LUA_FNC_L_DRAW_SWAP_usage "lcd:draw_swap() Updates the screen with the contents of the background buffer."

#define GRID_LUA_FNC_L_DRAW_PIXEL_short "ldpx"
#define GRID_LUA_FNC_L_DRAW_PIXEL_human "draw_pixel"
#define GRID_LUA_FNC_L_DRAW_PIXEL_usage "lcd:draw_pixel(x, y, {r, g, b}) Draws a pixel at (x, y) with the specified 8-bit color channels."

#define GRID_LUA_FNC_L_DRAW_LINE_short "ldl"
#define GRID_LUA_FNC_L_DRAW_LINE_human "draw_line"
#define GRID_LUA_FNC_L_DRAW_LINE_usage "lcd:draw_line(x1, y1, x2, y2, {r, g, b}) Draws a line between (x1, y1) and (x2, y2) points with the specified 8-bit color channels."

#define GRID_LUA_FNC_L_DRAW_RECTANGLE_short "ldr"
#define GRID_LUA_FNC_L_DRAW_RECTANGLE_human "draw_rectangle"
#define GRID_LUA_FNC_L_DRAW_RECTANGLE_usage "lcd:draw_rectangle(x1, y1, x2, y2, {r, g, b}) Draws a rectangle between (x1, y1) and (x2, y2) points with the specified 8-bit color channels."

#define GRID_LUA_FNC_L_DRAW_RECTANGLE_FILLED_short "ldrf"
#define GRID_LUA_FNC_L_DRAW_RECTANGLE_FILLED_human "draw_rectangle_filled"
#define GRID_LUA_FNC_L_DRAW_RECTANGLE_FILLED_usage                                                                                                                                                     \
  "lcd:draw_rectangle_filled(x1, y1, x2, y2, {r, g, b}) Draws a filled rectangle between (x1, y1) and (x2, y2) points with the specified 8-bit color channels."

#define GRID_LUA_FNC_L_DRAW_RECTANGLE_ROUNDED_short "ldrr"
#define GRID_LUA_FNC_L_DRAW_RECTANGLE_ROUNDED_human "draw_rectangle_rounded"
#define GRID_LUA_FNC_L_DRAW_RECTANGLE_ROUNDED_usage                                                                                                                                                    \
  "lcd:draw_rectangle_rounded(x1, y1, x2, y2, radius, {r, g, b}) Draws a rounded rectangle between (x1, y1) and (x2, y2) points using pixel based radius with the specified 8-bit color "              \
  "channels."

#define GRID_LUA_FNC_L_DRAW_RECTANGLE_ROUNDED_FILLED_short "ldrrf"
#define GRID_LUA_FNC_L_DRAW_RECTANGLE_ROUNDED_FILLED_human "draw_rectangle_rounded_filled"
#define GRID_LUA_FNC_L_DRAW_RECTANGLE_ROUNDED_FILLED_usage                                                                                                                                             \
  "lcd:draw_rectangle_rounded_filled(x1, y1, x2, y2, radius, {r, g, b}) Draws a filled rounded rectangle between (x1, y1) and (x2, y2) points using pixel based radius with the specified 8-bit "      \
  "color channels."

#define GRID_LUA_FNC_L_DRAW_POLYGON_short "ldpo"
#define GRID_LUA_FNC_L_DRAW_POLYGON_human "draw_polygon"
#define GRID_LUA_FNC_L_DRAW_POLYGON_usage "lcd:draw_polygon({x1, x2, x3 ...}, {y1, y2, y3 ...}, {r, g, b}) Draws a polygon using the x and y coordinate pairs with the specified 8-bit color channels."

#define GRID_LUA_FNC_L_DRAW_POLYGON_FILLED_short "ldpof"
#define GRID_LUA_FNC_L_DRAW_POLYGON_FILLED_human "draw_polygon_filled"
#define GRID_LUA_FNC_L_DRAW_POLYGON_FILLED_usage                                                                                                                                                       \
  "lcd:draw_polygon_filled({x1, x2, x3 ...}, {y1, y2, y3 ...}, {r, g, b}) Draws a filled polygon using the x and y coordinate pairs with the specified 8-bit color channels."

#define GRID_LUA_FNC_L_DRAW_TEXT_short "ldt"
#define GRID_LUA_FNC_L_DRAW_TEXT_human "draw_text"
#define GRID_LUA_FNC_L_DRAW_TEXT_usage "lcd:draw_text('text', x, y, size, {r, g, b}) Draws the specified text at (x, y) with the specified font size and 8-bit color channels."

#define GRID_LUA_FNC_L_DRAW_FASTTEXT_short "ldft"
#define GRID_LUA_FNC_L_DRAW_FASTTEXT_human "draw_text_fast"
#define GRID_LUA_FNC_L_DRAW_FASTTEXT_usage "lcd:draw_text_fast('text', x, y, size, {r, g, b}) Draws the specified text at (x, y) with the specified font size and 8-bit color channels."

#define GRID_LUA_FNC_L_DRAW_AREA_FILLED_short "ldaf"
#define GRID_LUA_FNC_L_DRAW_AREA_FILLED_human "draw_area_filled"
#define GRID_LUA_FNC_L_DRAW_AREA_FILLED_usage "lcd:draw_area_filled(x1, y1, x2, y2, {r, g, b}) Fills an area with the specified color, without alpha blending."

#define GRID_LUA_FNC_L_DRAW_DEMO_short "ldd"
#define GRID_LUA_FNC_L_DRAW_DEMO_human "draw_demo"
#define GRID_LUA_FNC_L_DRAW_DEMO_usage "lcd:draw_demo(n) Draws the n-th iteration of the demo."

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
  GRID_LUA_FNC_ASSIGN_META_PAR1_RET("gen", GRID_LUA_FNC_G_ELEMENTNAME_short) "," \
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
  GRID_LUA_FNC_ASSIGN_META_DRAW(GRID_LUA_FNC_L_DRAW_FASTTEXT_short, GRID_LUA_FNC_G_GUI_DRAW_FASTTEXT_short) "," \
  GRID_LUA_FNC_ASSIGN_META_DRAW(GRID_LUA_FNC_L_DRAW_AREA_FILLED_short, GRID_LUA_FNC_G_GUI_DRAW_AREA_FILLED_short) "," \
  GRID_LUA_FNC_ASSIGN_META_DRAW(GRID_LUA_FNC_L_DRAW_DEMO_short, GRID_LUA_FNC_G_GUI_DRAW_DEMO_short) "," \
  \
  "}}"
// clang-format on

#define GRID_ACTIONSTRING_LCD_INIT                                                                                                                                                                     \
  "<?lua --[[@cb]] local x={0,0,38,64,64,65,104,130,130,131,170,183,183,145,119,119,118,79,53,53,52,13}local "                                                                                         \
  "y={42,38,0,0,39,39,0,0,39,39,0,0,4,42,42,3,3,42,42,3,3,42}self:ldrf(0,0,320,240,{0,0,0})self:ldpof(x,y,{200,200,200})self:ldt('Intech Studio',2,55,34,{160,160,160})self:ldsw() ?>"

#endif /* GRID_UI_LCD_H */
