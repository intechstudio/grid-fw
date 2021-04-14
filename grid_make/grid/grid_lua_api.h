#ifndef GRID_LUA_API_H_INCLUDED
#define GRID_LUA_API_H_INCLUDED

#include "sam.h"
#include "grid_module.h"

#include "thirdparty/Lua/lua-5.4.3/src/lua.h"
#include "thirdparty/Lua/lua-5.4.3/src/lualib.h"
#include "thirdparty/Lua/lua-5.4.3/src/lauxlib.h"


int _gettimeofday(void);
int _open(void);
int _times(void);
int _unlink(void);
int _link(void);


#define GRID_LUA_STDO_LENGTH    100
#define GRID_LUA_STDI_LENGTH    100

struct grid_lua_model{
    
    lua_State *L;

    uint32_t stdo_len;
    uint32_t stdi_len;

    uint8_t stdo[GRID_LUA_STDO_LENGTH];
    uint8_t stdi[GRID_LUA_STDI_LENGTH];

};

struct grid_lua_model grid_lua_state;

uint8_t grid_lua_init(struct grid_lua_model* mod);
uint8_t grid_lua_deinit(struct grid_lua_model* mod);

uint8_t grid_lua_start_vm(struct grid_lua_model* mod);
uint8_t grid_lua_stop_vm(struct grid_lua_model* mod);

uint32_t grid_lua_dostring(struct grid_lua_model* mod, char* code);

void grid_lua_clear_stdi(struct grid_lua_model* mod);
void grid_lua_clear_stdo(struct grid_lua_model* mod);



#endif