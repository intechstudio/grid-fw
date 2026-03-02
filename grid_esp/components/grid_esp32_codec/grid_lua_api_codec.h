#pragma once

#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

int l_grid_sound_enable(lua_State* L);
int l_grid_sound_disable(lua_State* L);
int l_grid_sound_configure(lua_State* L);

extern struct luaL_Reg* grid_lua_api_codec_lib_reference;
