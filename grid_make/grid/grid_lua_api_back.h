#ifndef GRID_LUA_API_BACK_H_INCLUDED
#define GRID_LUA_API_BACK_H_INCLUDED

#include "sam.h"
#include "grid_module.h"

#include "grid_lua_api.h"

#include "lua-5.4.3/src/lua.h"
#include "lua-5.4.3/src/lualib.h"
#include "lua-5.4.3/src/lauxlib.h"


int _gettimeofday(void);
int _open(void);
int _times(void);
int _unlink(void);
int _link(void);






uint8_t grid_lua_ui_init(struct grid_lua_model* mod, struct grid_sys_model* sys);

uint8_t grid_lua_ui_init_po16(struct grid_lua_model* mod);
uint8_t grid_lua_ui_init_bu16(struct grid_lua_model* mod);
uint8_t grid_lua_ui_init_pbf4(struct grid_lua_model* mod);
uint8_t grid_lua_ui_init_en16(struct grid_lua_model* mod);
uint8_t grid_lua_ui_init_ef44(struct grid_lua_model* mod);



uint8_t grid_lua_start_vm(struct grid_lua_model* mod);
uint8_t grid_lua_stop_vm(struct grid_lua_model* mod);




#endif