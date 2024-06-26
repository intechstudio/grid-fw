#include "grid_lua_api_gui.h"

#include "grid_protocol.h"

#include "lua-5.4.3/src/lauxlib.h"
#include "lua-5.4.3/src/lua.h"
#include "lua-5.4.3/src/lualib.h"

// only for uint definitions
#include <stdint.h>
// only for malloc
#include <stdlib.h>

#include <string.h>

#include "grid_font.h"
#include "grid_gui.h"

extern void grid_platform_printf(char const* fmt, ...);
int l_grid_gui_draw_pixel(lua_State* L) {

  // grid_platform_printf("TEST GUI: l_grid_gui_draw_pixel\r\n");

  int x = luaL_checknumber(L, 1);
  int y = luaL_checknumber(L, 2);
  // Optional color parameter
  if (lua_gettop(L) >= 3) {
    luaL_checktype(L, 3, LUA_TTABLE);
    lua_rawgeti(L, 3, 1);
    int r = luaL_checknumber(L, -1);
    lua_rawgeti(L, 3, 2);
    int g = luaL_checknumber(L, -1);
    lua_rawgeti(L, 3, 3);
    int b = luaL_checknumber(L, -1);
    // Use r, g, b values to set color
    // grid_platform_printf("Received color: R=%d, G=%d, B=%d\n", r, g, b);

    grid_gui_draw_pixel(&grid_gui_state, x, y, grid_gui_color_from_rgb(r, g, b));
  }
  // Draw the pixel at (x, y)

  // Draw the line from (x1, y1) to (x2, y2) with optional color
  return 0;
}

int l_grid_gui_draw_rectangle(lua_State* L) {

  // grid_platform_printf("TEST GUI: l_grid_gui_draw_pixel\r\n");

  int x1 = luaL_checknumber(L, 1);
  int y1 = luaL_checknumber(L, 2);
  int x2 = luaL_checknumber(L, 3);
  int y2 = luaL_checknumber(L, 4);
  // Optional color parameter
  if (lua_gettop(L) >= 5) {
    luaL_checktype(L, 5, LUA_TTABLE);
    lua_rawgeti(L, 5, 1);
    int r = luaL_checknumber(L, -1);
    lua_rawgeti(L, 5, 2);
    int g = luaL_checknumber(L, -1);
    lua_rawgeti(L, 5, 3);
    int b = luaL_checknumber(L, -1);
    // Use r, g, b values to set color
    // grid_platform_printf("Received color: R=%d, G=%d, B=%d\n", r, g, b);

    grid_gui_draw_rectangle(&grid_gui_state, x1, y1, x2, y2, grid_gui_color_from_rgb(r, g, b));
  }
  // Draw the pixel at (x, y)

  // Draw the line from (x1, y1) to (x2, y2) with optional color
  return 0;
}

int l_grid_gui_draw_rectangle_filled(lua_State* L) {

  // grid_platform_printf("TEST GUI: l_grid_gui_draw_pixel\r\n");

  int x1 = luaL_checknumber(L, 1);
  int y1 = luaL_checknumber(L, 2);
  int x2 = luaL_checknumber(L, 3);
  int y2 = luaL_checknumber(L, 4);
  // Optional color parameter
  if (lua_gettop(L) >= 5) {
    luaL_checktype(L, 5, LUA_TTABLE);
    lua_rawgeti(L, 5, 1);
    int r = luaL_checknumber(L, -1);
    lua_rawgeti(L, 5, 2);
    int g = luaL_checknumber(L, -1);
    lua_rawgeti(L, 5, 3);
    int b = luaL_checknumber(L, -1);
    // Use r, g, b values to set color
    // grid_platform_printf("Received color: R=%d, G=%d, B=%d\n", r, g, b);

    grid_gui_draw_rectangle_filled(&grid_gui_state, x1, y1, x2, y2, grid_gui_color_from_rgb(r, g, b));
  }
  // Draw the pixel at (x, y)

  // Draw the line from (x1, y1) to (x2, y2) with optional color
  return 0;
}

// Function to draw text
int l_grid_gui_draw_text(lua_State* L) {
  const char* text = luaL_checkstring(L, 1);
  int x = luaL_checknumber(L, 2);
  int y = luaL_checknumber(L, 3);
  // Optional font_size parameter
  int font_size = 12; // Default font size
  if (lua_gettop(L) >= 4) {
    font_size = luaL_checknumber(L, 4);
  }
  // Optional color parameter
  int r = 0, g = 0, b = 0; // Default color: black
  if (lua_gettop(L) >= 5) {
    luaL_checktype(L, 5, LUA_TTABLE);
    lua_rawgeti(L, 5, 1);
    r = luaL_checknumber(L, -1);
    lua_rawgeti(L, 5, 2);
    g = luaL_checknumber(L, -1);
    lua_rawgeti(L, 5, 3);
    b = luaL_checknumber(L, -1);
  }
  // Draw the text at (x, y) with optional font size and color
  // grid_platform_printf("Drawing text: \"%s\" at (%d, %d) with font size %d and color R=%d, G=%d, B=%d\n", text, x, y, font_size, r, g, b);
  int cursor = 0;

  if (grid_font_state.initialized) {
    grid_font_draw_string(&grid_font_state, &grid_gui_state, x, y, font_size, text, &cursor, grid_gui_color_from_rgb(r, g, b));
  } else {
    // grid_platform_printf("NOT INITIALIZED\n");
  }

  return 0;
}

/*static*/ int l_grid_gui_draw_line(lua_State* L) { return 0; }

/*static*/ const struct luaL_Reg grid_lua_api_gui_lib[] = {
    {GRID_LUA_FNC_G_GUI_DRAW_PIXEL_short, GRID_LUA_FNC_G_GUI_DRAW_PIXEL_fnptr},
    {GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_short, GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_fnptr},
    {GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_FILLED_short, GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_FILLED_fnptr},
    {GRID_LUA_FNC_G_GUI_DRAW_TEXT_short, GRID_LUA_FNC_G_GUI_DRAW_TEXT_fnptr},
    {NULL, NULL} /* end of array */
};
