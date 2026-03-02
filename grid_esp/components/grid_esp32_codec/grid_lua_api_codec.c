#include "grid_lua_api_codec.h"

#include "grid_esp32_codec.h"
#include "grid_protocol.h"

int l_grid_sound_enable(lua_State* L) {
  int nargs = lua_gettop(L);
  if (nargs != 0) {
    return 0;
  }
  grid_esp32_codec_init();
  return 0;
}

int l_grid_sound_disable(lua_State* L) {
  int nargs = lua_gettop(L);
  if (nargs != 0) {
    return 0;
  }
  grid_esp32_codec_deinit();
  return 0;
}

int l_grid_sound_configure(lua_State* L) {
  int nargs = lua_gettop(L);
  if (nargs != 3) {
    return 0;
  }

  double freq   = lua_tonumber(L, 1);
  double volume = lua_tonumber(L, 2);

  // shape is a table: {peak_time, half_height, zero_wait}
  if (!lua_istable(L, 3)) {
    return 0;
  }

  lua_rawgeti(L, 3, 1);
  double peak_time   = lua_tonumber(L, -1);
  lua_pop(L, 1);

  lua_rawgeti(L, 3, 2);
  double half_height = lua_tonumber(L, -1);
  lua_pop(L, 1);

  lua_rawgeti(L, 3, 3);
  double zero_wait   = lua_tonumber(L, -1);
  lua_pop(L, 1);

  grid_esp32_codec_configure(freq, volume, peak_time, half_height, zero_wait);
  return 0;
}

/*static*/ struct luaL_Reg grid_lua_api_codec_lib[] = {
    {GRID_LUA_FNC_G_SOUND_ENABLE_short,    GRID_LUA_FNC_G_SOUND_ENABLE_fnptr},
    {GRID_LUA_FNC_G_SOUND_DISABLE_short,   GRID_LUA_FNC_G_SOUND_DISABLE_fnptr},
    {GRID_LUA_FNC_G_SOUND_CONFIGURE_short, GRID_LUA_FNC_G_SOUND_CONFIGURE_fnptr},
    {NULL, NULL},
};

struct luaL_Reg* grid_lua_api_codec_lib_reference = grid_lua_api_codec_lib;
