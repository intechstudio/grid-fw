#pragma once

#ifndef GRID_LUA_API_H_INCLUDED
#define GRID_LUA_API_H_INCLUDED

// only for uint definitions
#include  <stdint.h>
// only for malloc
#include  <stdlib.h>

#include <string.h>


#include "lua-5.4.3/src/lua.h"
#include "lua-5.4.3/src/lualib.h"
#include "lua-5.4.3/src/lauxlib.h"

#include "grid_protocol.h"
#include "grid_msg.h"
#include "grid_sys.h"

extern void grid_platform_printf(char const *fmt, ...);
extern void grid_platform_delay_ms(uint32_t delay_milliseconds);

// GRID LOOKUP TABLE
#define GRID_LUA_GLUT_source \
"function glut (a, ...)      \
 local t = table.pack(...)   \
 for i = 1, t.n//2*2 do      \
  if i%2 == 1 then           \
   if t[i] == a then         \
    return t[i+1]            \
   end                       \
  end                        \
 end                         \
 return nil                  \
end"

// GRID LIMIT
#define GRID_LUA_GLIM_source  \
"function glim (a, min, max)  \
 if a>max then return max end \
 if a<min then return min end \
 return a \
end"

// GRID ELEMENT NAME
#define GRID_LUA_GEN_source  \
"function gen (a, b)  \
 if b==nil then \
  if ele[a].sn==nil then\
   return ''\
  else\
   return ele[a].sn \
  end\
 else \
  ele[a].sn=b \
  gens(a,b) \
 end \
end"

#define GRID_LUA_STDO_LENGTH    100
#define GRID_LUA_STDI_LENGTH    100

#define GRID_LUA_STDE_LENGTH    100

struct grid_lua_model{
    
    lua_State *L;

    uint32_t stdo_len;
    uint32_t stdi_len;

    uint32_t stde_len;

    char stdo[GRID_LUA_STDO_LENGTH];
    char stdi[GRID_LUA_STDI_LENGTH];

    char stde[GRID_LUA_STDE_LENGTH];

    uint32_t dostring_count;

};



extern struct grid_lua_model grid_lua_state;


void grid_lua_init(struct grid_lua_model* mod);
void grid_lua_deinit(struct grid_lua_model* mod);


void grid_lua_clear_stdi(struct grid_lua_model* mod);
void grid_lua_clear_stdo(struct grid_lua_model* mod);
void grid_lua_clear_stde(struct grid_lua_model* mod);


char* grid_lua_get_output_string(struct grid_lua_model* mod);
char* grid_lua_get_error_string(struct grid_lua_model* mod);


uint32_t grid_lua_dostring(struct grid_lua_model* mod, char* code);


void grid_lua_gc_try_collect(struct grid_lua_model* mod);
void grid_lua_gc_collect(struct grid_lua_model* mod);


void grid_lua_debug_memory_stats(struct grid_lua_model* mod, char* message);


/*static*/ int grid_lua_panic(lua_State *L);

/* ==================== LUA C API REGISTERED FUNCTIONS  ====================*/

/*static*/ int l_grid_send(lua_State* L);


// ==== MIDI ====

/*static*/ int l_grid_midirx_enabled(lua_State* L);
/*static*/ int l_grid_midirx_sync(lua_State* L);


///*static*/ int l_grid_midi_send(lua_State* L);

#endif /* GRID_LUA_API_H_INCLUDED */