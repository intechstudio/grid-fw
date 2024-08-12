#pragma once

#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

void grid_gui_lua_draw_demo(lua_State* L, uint8_t loopcounter);

int l_grid_gui_draw_pixel(lua_State* L);
int l_grid_gui_draw_line(lua_State* L);
int l_grid_gui_draw_rectangle(lua_State* L);
int l_grid_gui_draw_rectangle_filled(lua_State* L);
int l_grid_gui_draw_rectangle_rounded(lua_State* L);
int l_grid_gui_draw_rectangle_rounded_filled(lua_State* L);
int l_grid_gui_draw_polygon(lua_State* L);
int l_grid_gui_draw_polygon_filled(lua_State* L);
int l_grid_gui_draw_text(lua_State* L);

extern struct luaL_Reg* grid_lua_api_gui_lib_reference;
