#pragma once



#include "lua-5.4.3/src/lauxlib.h"
#include "lua-5.4.3/src/lua.h"
#include "lua-5.4.3/src/lualib.h"



int l_grid_gui_draw_pixel(lua_State* L);
int l_grid_gui_draw_line(lua_State* L);
int l_grid_gui_draw_rectangle(lua_State* L);
int l_grid_gui_draw_rectangle_filled(lua_State* L);
int l_grid_gui_draw_text(lua_State* L);

