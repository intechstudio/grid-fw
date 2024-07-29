#pragma once

#ifndef GRID_LUA_H_INCLUDED
#define GRID_LUA_H_INCLUDED

// only for uint definitions
#include <stdint.h>
// only for malloc
#include <stdlib.h>

#include <string.h>

#include "lua-5.4.3/src/lauxlib.h"
#include "lua-5.4.3/src/lua.h"
#include "lua-5.4.3/src/lualib.h"

#include "grid_ui.h"

extern void grid_platform_printf(char const* fmt, ...);
extern void grid_platform_delay_ms(uint32_t delay_milliseconds);

// GRID LOOKUP TABLE
#define GRID_LUA_GLUT_source                                                                                                                                                                           \
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
#define GRID_LUA_GLIM_source                                                                                                                                                                           \
  "function glim (a, min, max)  \
 if a>max then return max end \
 if a<min then return min end \
 return a \
end"

// GRID ELEMENT NAME
#define GRID_LUA_GEN_source                                                                                                                                                                            \
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

#define GRID_LUA_MAPSAT_source                                                                                                                                                                         \
  "function " GRID_LUA_FNC_G_MAPSAT_short "(x, in_min, in_max, o_min, o_max) \
	local n = (x - in_min) * (o_max - o_min) / (in_max - in_min) + o_min \
	if n > o_max then \
		return o_max \
	elseif n < o_min then \
		return o_min \
	else \
		return n \
	end \
end"

#define GRID_LUA_SIGN_source                                                                                                                                                                           \
  "function " GRID_LUA_FNC_G_SIGN_short "(x) \
    if x > 0 then \
        return 1 \
    elseif x < 0 then \
        return -1 \
    else \
        return 0 \
    end \
end"

#define GRID_LUA_SEGCALC_source                                                                                                                                                                        \
  "function " GRID_LUA_FNC_G_SEGCALC_short "(seg, enc_val, enc_min, enc_max) \
	local s_min = enc_min + (enc_max - enc_min) / 5 * seg; \
	local s_max = enc_min + (enc_max - enc_min) / 5 * (seg + 1) \
	return " GRID_LUA_FNC_G_MAPSAT_short "(enc_val, s_min, s_max, 0, 127) // 1 \
end"

#define GRID_LUA_STDO_LENGTH 100
#define GRID_LUA_STDI_LENGTH 100

#define GRID_LUA_STDE_LENGTH 100

struct grid_lua_model {

  lua_State* L;

  void* (*custom_allocator)(void*, void*, size_t, size_t) ;
  void* custom_allocator_instance;

  uint32_t stdo_len;
  uint32_t stdi_len;

  void* busy_semaphore;

  void (*busy_semaphore_lock_fn)(void*);
  void (*busy_semaphore_release_fn)(void*);

  uint32_t stde_len;

  char stdo[GRID_LUA_STDO_LENGTH];
  char stdi[GRID_LUA_STDI_LENGTH];

  char stde[GRID_LUA_STDE_LENGTH];

  uint32_t dostring_count;

  uint8_t target_memory_usage_kilobytes;
};

extern struct grid_lua_model grid_lua_state;

void grid_lua_init(struct grid_lua_model* lua, void* (*custom_allocator)(void*, void*, size_t, size_t), void* custom_allocator_instance);
void grid_lua_deinit(struct grid_lua_model* lua);

void grid_lua_semaphore_init(struct grid_lua_model* lua, void* lua_busy_semaphore, void (*lock_fn)(void*), void (*release_fn)(void*));
void grid_lua_semaphore_lock(struct grid_lua_model* lua);
void grid_lua_semaphore_release(struct grid_lua_model* lua);

void grid_lua_set_memory_target(struct grid_lua_model* lua, uint8_t target_kilobytes);
uint8_t grid_lua_get_memory_target(struct grid_lua_model* lua);

void grid_lua_clear_stdi(struct grid_lua_model* lua);
void grid_lua_clear_stdo(struct grid_lua_model* lua);
void grid_lua_clear_stde(struct grid_lua_model* lua);

char* grid_lua_get_output_string(struct grid_lua_model* lua);
char* grid_lua_get_error_string(struct grid_lua_model* lua);

uint32_t grid_lua_dostring(struct grid_lua_model* lua, char* code);

void grid_lua_gc_try_collect(struct grid_lua_model* lua);
void grid_lua_gc_collect(struct grid_lua_model* lua);

void grid_lua_debug_memory_stats(struct grid_lua_model* lua, char* message);

/*static*/ int grid_lua_panic(lua_State* L);

int grid_lua_vm_register_functions(struct grid_lua_model* lua, struct luaL_Reg* lua_lib);

void grid_lua_ui_init(struct grid_lua_model* lua, struct grid_ui_model* ui);

void grid_lua_start_vm(struct grid_lua_model* lua);

void grid_lua_stop_vm(struct grid_lua_model* lua);

#endif
