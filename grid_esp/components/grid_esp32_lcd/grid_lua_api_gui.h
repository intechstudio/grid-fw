#ifndef GRID_LUA_API_GUI_H
#define GRID_LUA_API_GUI_H

#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

#include <stdint.h>
#include <stdlib.h>

#include <string.h>

#include "grid_font.h"
#include "grid_gui.h"
#include "grid_protocol.h"
#include "grid_swsr.h"

int l_grid_gui_draw_swap(lua_State* L);
int l_grid_gui_draw_pixel(lua_State* L);
int l_grid_gui_draw_line(lua_State* L);
int l_grid_gui_draw_rectangle(lua_State* L);
int l_grid_gui_draw_rectangle_filled(lua_State* L);
int l_grid_gui_draw_rectangle_rounded(lua_State* L);
int l_grid_gui_draw_rectangle_rounded_filled(lua_State* L);
int l_grid_gui_draw_polygon(lua_State* L);
int l_grid_gui_draw_polygon_filled(lua_State* L);
int l_grid_gui_draw_text_fast(lua_State* L);
int l_grid_gui_draw_text(lua_State* L);
int l_grid_gui_draw_demo(lua_State* L);

extern struct luaL_Reg* grid_lua_api_gui_lib_reference;

#endif /* GRID_LUA_API_GUI_H */
