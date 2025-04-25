#ifndef GRID_LUA_H
#define GRID_LUA_H

#include <stdint.h>

#include "lua-5.4.3/src/lauxlib.h"
#include "lua-5.4.3/src/lua.h"
#include "lua-5.4.3/src/lualib.h"

#include "lua_src/lua_source_collection.h"

extern void grid_platform_printf(char const* fmt, ...);
extern void grid_platform_delay_ms(uint32_t delay_milliseconds);

#define GRID_LUA_STDO_LENGTH 100
#define GRID_LUA_STDI_LENGTH 100

#define GRID_LUA_STDE_LENGTH 400

struct grid_lua_model {

  lua_State* L;

  void* (*custom_allocator)(void*, void*, size_t, size_t);
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

typedef void (*lua_ui_init_callback_t)(struct grid_lua_model*);

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

uint32_t grid_lua_dostring(struct grid_lua_model* lua, const char* code);

void grid_lua_gc_try_collect(struct grid_lua_model* lua);
void grid_lua_gc_collect(struct grid_lua_model* lua);

void grid_lua_debug_memory_stats(struct grid_lua_model* lua, char* message);

/*static*/ int grid_lua_panic(lua_State* L);

int grid_lua_vm_register_functions(struct grid_lua_model* lua, const struct luaL_Reg* lua_lib);

void grid_lua_ui_init(struct grid_lua_model* lua, lua_ui_init_callback_t callback);

void grid_lua_start_vm(struct grid_lua_model* lua);

void grid_lua_stop_vm(struct grid_lua_model* lua);

// clang-format off

// Double stringize trick
#define XSTRINGIZE(s) STRINGIZE(s)
#define STRINGIZE(s) #s

#define GRID_LUA_FNC_ASSIGN_META_GTV(key, index) \
  key " = function (self, a) " \
  "return gtv(self.index, " XSTRINGIZE(index) ", a) end"

#define GRID_LUA_FNC_ASSIGN_META_UNDEF(key) \
  key " = function (self) print('undefined action') end"

#define GRID_LUA_FNC_ASSIGN_META_PAR0(key, val) \
  key " = function (self) " val "(self.index) end"

#define GRID_LUA_FNC_ASSIGN_META_PAR1(key, val) \
  key " = function (self, a) " val "(self.index, a) end"

#define GRID_LUA_FNC_ASSIGN_META_PAR1_RET(key, val) \
  key " = function (self, a) return " val "(self.index, a) end"

// clang-format on

#endif /* GRID_LUA_H */
