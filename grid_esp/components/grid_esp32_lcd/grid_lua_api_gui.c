#include "grid_lua_api_gui.h"

#include "grid_protocol.h"

#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

// only for uint definitions
#include <stdint.h>
// only for malloc
#include <stdlib.h>

#include <string.h>

#include "grid_font.h"
#include "grid_gui.h"

extern void grid_platform_printf(char const* fmt, ...);

int l_grid_gui_draw_pixel(lua_State* L) {

  int screen_index = luaL_checknumber(L, 1);

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  int x = luaL_checknumber(L, 2);
  int y = luaL_checknumber(L, 3);

  luaL_checktype(L, 4, LUA_TTABLE);
  lua_rawgeti(L, 4, 1);
  int r = luaL_checknumber(L, -1);
  lua_rawgeti(L, 4, 2);
  int g = luaL_checknumber(L, -1);
  lua_rawgeti(L, 4, 3);
  int b = luaL_checknumber(L, -1);

  grid_gui_draw_pixel(gui, x, y, grid_gui_color_from_rgb(r, g, b));

  return 0;
}

int l_grid_gui_draw_line(lua_State* L) {

  int screen_index = luaL_checknumber(L, 1);

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  int x1 = luaL_checknumber(L, 2);
  int y1 = luaL_checknumber(L, 3);
  int x2 = luaL_checknumber(L, 4);
  int y2 = luaL_checknumber(L, 5);

  luaL_checktype(L, 6, LUA_TTABLE);
  lua_rawgeti(L, 6, 1);
  int r = luaL_checknumber(L, -1);
  lua_rawgeti(L, 6, 2);
  int g = luaL_checknumber(L, -1);
  lua_rawgeti(L, 6, 3);
  int b = luaL_checknumber(L, -1);

  grid_gui_draw_line(gui, x1, y1, x2, y2, grid_gui_color_from_rgb(r, g, b));

  return 0;
}

int l_grid_gui_draw_rectangle(lua_State* L) {

  int screen_index = luaL_checknumber(L, 1);

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  int x1 = luaL_checknumber(L, 2);
  int y1 = luaL_checknumber(L, 3);
  int x2 = luaL_checknumber(L, 4);
  int y2 = luaL_checknumber(L, 5);

  luaL_checktype(L, 6, LUA_TTABLE);
  lua_rawgeti(L, 6, 1);
  int r = luaL_checknumber(L, -1);
  lua_rawgeti(L, 6, 2);
  int g = luaL_checknumber(L, -1);
  lua_rawgeti(L, 6, 3);
  int b = luaL_checknumber(L, -1);

  grid_gui_draw_rectangle(gui, x1, y1, x2, y2, grid_gui_color_from_rgb(r, g, b));

  return 0;
}

int l_grid_gui_draw_rectangle_filled(lua_State* L) {

  int screen_index = luaL_checknumber(L, 1);

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  int x1 = luaL_checknumber(L, 2);
  int y1 = luaL_checknumber(L, 3);
  int x2 = luaL_checknumber(L, 4);
  int y2 = luaL_checknumber(L, 5);

  luaL_checktype(L, 6, LUA_TTABLE);
  lua_rawgeti(L, 6, 1);
  int r = luaL_checknumber(L, -1);
  lua_rawgeti(L, 6, 2);
  int g = luaL_checknumber(L, -1);
  lua_rawgeti(L, 6, 3);
  int b = luaL_checknumber(L, -1);

  grid_gui_draw_rectangle_filled(gui, x1, y1, x2, y2, grid_gui_color_from_rgb(r, g, b));

  return 0;
}

int l_grid_gui_draw_rectangle_rounded(lua_State* L) {

  int screen_index = luaL_checknumber(L, 1);

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  int x1 = luaL_checknumber(L, 2);
  int y1 = luaL_checknumber(L, 3);
  int x2 = luaL_checknumber(L, 4);
  int y2 = luaL_checknumber(L, 5);
  int radius = luaL_checknumber(L, 6);

  luaL_checktype(L, 7, LUA_TTABLE);
  lua_rawgeti(L, 7, 1);
  int r = luaL_checknumber(L, -1);
  lua_rawgeti(L, 7, 2);
  int g = luaL_checknumber(L, -1);
  lua_rawgeti(L, 7, 3);
  int b = luaL_checknumber(L, -1);

  grid_gui_draw_rectangle_rounded(gui, x1, y1, x2, y2, radius, grid_gui_color_from_rgb(r, g, b));

  return 0;
}

int l_grid_gui_draw_rectangle_rounded_filled(lua_State* L) {

  int screen_index = luaL_checknumber(L, 1);

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  int x1 = luaL_checknumber(L, 2);
  int y1 = luaL_checknumber(L, 3);
  int x2 = luaL_checknumber(L, 4);
  int y2 = luaL_checknumber(L, 5);
  int radius = luaL_checknumber(L, 6);

  luaL_checktype(L, 7, LUA_TTABLE);
  lua_rawgeti(L, 7, 1);
  int r = luaL_checknumber(L, -1);
  lua_rawgeti(L, 7, 2);
  int g = luaL_checknumber(L, -1);
  lua_rawgeti(L, 7, 3);
  int b = luaL_checknumber(L, -1);

  grid_gui_draw_rectangle_rounded_filled(gui, x1, y1, x2, y2, radius, grid_gui_color_from_rgb(r, g, b));

  return 0;
}

int l_grid_gui_draw_polygon(lua_State* L) {

  int screen_index = luaL_checknumber(L, 1);

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  // The C function that will be called from Lua
  // Check that the first argument is a table
  if (!lua_istable(L, 2)) {
    return luaL_error(L, "Expected a table as the argument");
  }

  // Get the length of the table
  size_t num_points = lua_rawlen(L, 2);
  uint16_t x_points[num_points];
  uint16_t y_points[num_points];

  // Iterate over the table and print the numbers
  for (size_t i = 0; i < num_points; i++) {
    // Push the index onto the stack
    lua_pushnumber(L, i + 1);

    // Get the value at the given index in the table
    lua_gettable(L, 2);

    // Get the number
    x_points[i] = lua_tonumber(L, -1);

    // printf("x[%d]=%d", i, x_points[i]);
    //  Pop the value off the stack
    lua_pop(L, 1);
  }

  // Iterate over the table and print the numbers
  for (size_t i = 0; i < num_points; i++) {
    // Push the index onto the stack
    lua_pushnumber(L, i + 1);

    // Get the value at the given index in the table
    lua_gettable(L, 3);

    // Get the number
    y_points[i] = lua_tonumber(L, -1);
    // printf("x[%d]=%d", i, y_points[i]);

    // Pop the value off the stack
    lua_pop(L, 1);
  }

  int r = 0, g = 0, b = 0; // Default color: black
  if (lua_gettop(L) >= 4) {
    luaL_checktype(L, 4, LUA_TTABLE);
    lua_rawgeti(L, 4, 1);
    r = luaL_checknumber(L, -1);
    lua_rawgeti(L, 4, 2);
    g = luaL_checknumber(L, -1);
    lua_rawgeti(L, 4, 3);
    b = luaL_checknumber(L, -1);
  }

  grid_gui_draw_polygon(gui, x_points, y_points, num_points, grid_gui_color_from_rgb(r, g, b));

  return 0; // Number of return values
}

int l_grid_gui_draw_polygon_filled(lua_State* L) {

  int screen_index = luaL_checknumber(L, 1);

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  // The C function that will be called from Lua
  // Check that the first argument is a table
  if (!lua_istable(L, 2)) {
    return luaL_error(L, "Expected a table as the argument");
  }

  // Get the length of the table
  size_t num_points = lua_rawlen(L, 2);
  uint16_t x_points[num_points];
  uint16_t y_points[num_points];

  // Iterate over the table and print the numbers
  for (size_t i = 0; i < num_points; i++) {
    // Push the index onto the stack
    lua_pushnumber(L, i + 1);

    // Get the value at the given index in the table
    lua_gettable(L, 2);

    // Get the number
    x_points[i] = lua_tonumber(L, -1);

    // printf("x[%d]=%d", i, x_points[i]);
    //  Pop the value off the stack
    lua_pop(L, 1);
  }

  // Iterate over the table and print the numbers
  for (size_t i = 0; i < num_points; i++) {
    // Push the index onto the stack
    lua_pushnumber(L, i + 1);

    // Get the value at the given index in the table
    lua_gettable(L, 3);

    // Get the number
    y_points[i] = lua_tonumber(L, -1);
    // printf("x[%d]=%d", i, y_points[i]);

    // Pop the value off the stack
    lua_pop(L, 1);
  }

  int r = 0, g = 0, b = 0; // Default color: black
  if (lua_gettop(L) >= 4) {
    luaL_checktype(L, 4, LUA_TTABLE);
    lua_rawgeti(L, 4, 1);
    r = luaL_checknumber(L, -1);
    lua_rawgeti(L, 4, 2);
    g = luaL_checknumber(L, -1);
    lua_rawgeti(L, 4, 3);
    b = luaL_checknumber(L, -1);
  }

  grid_gui_draw_polygon_filled(gui, x_points, y_points, num_points, grid_gui_color_from_rgb(r, g, b));

  return 0; // Number of return values
}

// Function to draw text
int l_grid_gui_draw_text(lua_State* L) {

  int screen_index = luaL_checknumber(L, 1);

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  const char* text = luaL_checkstring(L, 2);
  int x = luaL_checknumber(L, 3);
  int y = luaL_checknumber(L, 4);
  // Optional font_size parameter
  int font_size = 12; // Default font size
  if (lua_gettop(L) >= 5) {
    font_size = luaL_checknumber(L, 5);
  }
  // Optional color parameter
  int r = 0, g = 0, b = 0; // Default color: black
  if (lua_gettop(L) >= 6) {
    luaL_checktype(L, 6, LUA_TTABLE);
    lua_rawgeti(L, 6, 1);
    r = luaL_checknumber(L, -1);
    lua_rawgeti(L, 6, 2);
    g = luaL_checknumber(L, -1);
    lua_rawgeti(L, 6, 3);
    b = luaL_checknumber(L, -1);
  }
  // Draw the text at (x, y) with optional font size and color
  // grid_platform_printf("Drawing text: \"%s\" at (%d, %d) with font size %d and color R=%d, G=%d, B=%d\n", text, x, y, font_size, r, g, b);
  int cursor = 0;

  if (grid_font_state.initialized) {
    grid_font_draw_string(&grid_font_state, gui, x, y, font_size, text, &cursor, grid_gui_color_from_rgb(r, g, b));
  } else {
    // grid_platform_printf("NOT INITIALIZED\n");
  }

  return 0;
}

int l_grid_gui_draw_demo(lua_State* L) {

  int screen_index = luaL_checknumber(L, 1);

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  int counter = luaL_checknumber(L, 2);

  grid_gui_draw_demo(gui, counter);

  return 0;
}

/*static*/ struct luaL_Reg grid_lua_api_gui_lib[] = {
    {GRID_LUA_FNC_G_GUI_DRAW_PIXEL_short, GRID_LUA_FNC_G_GUI_DRAW_PIXEL_fnptr},
    {GRID_LUA_FNC_G_GUI_DRAW_LINE_short, GRID_LUA_FNC_G_GUI_DRAW_LINE_fnptr},
    {GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_short, GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_fnptr},
    {GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_FILLED_short, GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_FILLED_fnptr},
    {GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_ROUNDED_short, GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_ROUNDED_fnptr},
    {GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_ROUNDED_FILLED_short, GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_ROUNDED_FILLED_fnptr},
    {GRID_LUA_FNC_G_GUI_DRAW_POLYGON_short, GRID_LUA_FNC_G_GUI_DRAW_POLYGON_fnptr},
    {GRID_LUA_FNC_G_GUI_DRAW_POLYGON_FILLED_short, GRID_LUA_FNC_G_GUI_DRAW_POLYGON_FILLED_fnptr},
    {GRID_LUA_FNC_G_GUI_DRAW_TEXT_short, GRID_LUA_FNC_G_GUI_DRAW_TEXT_fnptr},
    {GRID_LUA_FNC_G_GUI_DRAW_DEMO_short, GRID_LUA_FNC_G_GUI_DRAW_DEMO_fnptr},
    {NULL, NULL} /* end of array */
};

struct luaL_Reg* grid_lua_api_gui_lib_reference = grid_lua_api_gui_lib;
