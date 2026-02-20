#ifndef GRID_LUA_H
#define GRID_LUA_H

#include <stdbool.h>
#include <stdint.h>

#include "lua-5.4.3/src/lauxlib.h"
#include "lua-5.4.3/src/lua.h"
#include "lua-5.4.3/src/lualib.h"

#include "lua_src/lua_source_collection.h"

extern void grid_platform_printf(char const* fmt, ...);
extern void grid_platform_delay_ms(uint32_t delay_milliseconds);

#define GRID_LUA_STDO_LENGTH 256
#define GRID_LUA_STDI_LENGTH 100

#define GRID_LUA_STDE_LENGTH 400

struct grid_lua_model {

  lua_State* L;

  void* (*custom_allocator)(void*, void*, size_t, size_t);
  void* custom_allocator_instance;

  void* busy_semaphore;

  void (*busy_semaphore_lock_fn)(void*);
  void (*busy_semaphore_release_fn)(void*);

  uint32_t stdo_len;
  uint32_t stdi_len;
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
void grid_lua_post_init(struct grid_lua_model* lua);
void grid_lua_pre_init(struct grid_lua_model* lua);

void grid_lua_semaphore_init(struct grid_lua_model* lua, void* lua_busy_semaphore, void (*lock_fn)(void*), void (*release_fn)(void*));
bool grid_lua_semaphore_lock(struct grid_lua_model* lua);
bool grid_lua_semaphore_release(struct grid_lua_model* lua);

void grid_lua_set_memory_target(struct grid_lua_model* lua, uint8_t target_kilobytes);
uint8_t grid_lua_get_memory_target(struct grid_lua_model* lua);

void grid_lua_clear_stdi(struct grid_lua_model* lua);
void grid_lua_clear_stdo(struct grid_lua_model* lua);
void grid_lua_clear_stde(struct grid_lua_model* lua);

int grid_lua_append_stdo(struct grid_lua_model* lua, char* str);
int grid_lua_append_stde(struct grid_lua_model* lua, char* str);

char* grid_lua_get_output_string(struct grid_lua_model* lua);
char* grid_lua_get_error_string(struct grid_lua_model* lua);

uint32_t grid_lua_dostring_unsafe(struct grid_lua_model* lua, const char* code);
uint32_t grid_lua_dostring(struct grid_lua_model* lua, const char* code);
bool grid_lua_dostring_begin(struct grid_lua_model* lua, const char* code);
void grid_lua_dostring_end(struct grid_lua_model* lua);
bool grid_lua_do_event(struct grid_lua_model* lua, uint8_t index, const char* function_name);
void grid_lua_decode_clear_results(struct grid_lua_model* lua);
void grid_lua_decode_process_results(struct grid_lua_model* lua);

void grid_lua_broadcast_stdo(struct grid_lua_model* lua);
void grid_lua_broadcast_stde(struct grid_lua_model* lua);

void grid_lua_gc_full_unsafe(struct grid_lua_model* lua);
void grid_lua_gc_step_unsafe(struct grid_lua_model* lua);
uint8_t grid_lua_gc_count_unsafe(struct grid_lua_model* lua);

void grid_lua_debug_memory_stats(struct grid_lua_model* lua, char* message);

/*static*/ int grid_lua_panic(lua_State* L);

int grid_lua_register_functions_unsafe(struct grid_lua_model* lua, const struct luaL_Reg* lua_lib);

void grid_lua_ui_init_unsafe(struct grid_lua_model* lua, lua_ui_init_callback_t callback);

void grid_lua_start_vm(struct grid_lua_model* lua, const struct luaL_Reg* lua_lib, lua_ui_init_callback_t callback);
void grid_lua_stop_vm(struct grid_lua_model* lua);

void grid_lua_dumpstack(lua_State* L);

void grid_lua_register_index_meta_for_type(lua_State* L, const char* type, const luaL_Reg* reg);
void grid_lua_create_element_array(lua_State* L, uint8_t elements);
void grid_lua_register_element(lua_State* L, uint8_t element);
void grid_lua_register_index_meta_for_element(lua_State* L, uint8_t element, const char* type);

// clang-format off

#define XAFTERX(macro, exp) macro(exp)

#define GRID_LUA_FNC_GTV_NAME(idx) grid_lua_gtv_key_to_idx ## idx

#define GRID_LUA_FNC_GTV_DEFI(idx) \
  int XAFTERX(GRID_LUA_FNC_GTV_NAME, idx)(lua_State* L) { \
    if (lua_gettop(L) < 2) { lua_pushnil(L); } /* ele val */ \
    lua_pushstring(L, "index"); /* ele val key */ \
    lua_rawget(L, 1); /* ele val eidx */ \
    lua_pushinteger(L, idx); /* ele val eidx tidx */ \
    lua_rotate(L, 1, -2); /* eidx tidx ele val */ \
    lua_remove(L, -2); /* eidx tidx val */ \
    return l_grid_template_variable(L); \
  }

#define GRID_LUA_FNC_META_NAME(shortname) grid_lua_meta_ ## shortname

#define GRID_LUA_FNC_META_DEFI(idx, fun) \
  int XAFTERX(GRID_LUA_FNC_META_NAME, idx)(lua_State* L) { \
    lua_pushstring(L, "index"); /* ele ... key */ \
    lua_rawget(L, 1); /* ele ... eidx */ \
    lua_replace(L, 1); /* eidx ... */ \
    return fun(L); \
  }

#define GRID_LUA_FNC_DRAW_NAME(shortname) grid_lua_draw_ ## shortname

#define GRID_LUA_FNC_DRAW_DEFI(idx, fun) \
  int XAFTERX(GRID_LUA_FNC_DRAW_NAME, idx)(lua_State* L) { \
    lua_pushstring(L, "index"); /* ele ... key */ \
    lua_rawget(L, 1); /* ele ... eidx */ \
    int32_t screen_idx = grid_ui_element_get_template_parameter( \
        grid_ui_element_find(&grid_ui_state, lua_tointeger(L, -1)), \
        GRID_LUA_FNC_L_SCREEN_INDEX_index \
    ); \
    lua_pop(L, 1); /* ele ... */ \
    lua_pushinteger(L, screen_idx); /* ele ... scr */ \
    lua_replace(L, 1); /* scr ... */ \
    return fun(L); \
  }

#define GRID_LUA_FNC_ASSIGN_META_PAR1_RET(key, val) \
  key " = function (self, a) return " val "(self.index, a) end"

#define GRID_LUA_DECODE_PROCESSOR "_decode_process"
#define GRID_LUA_DECODE_CLEARER "_decode_clear"
#define GRID_LUA_DECODE_ORDER "_decoded_order"
#define GRID_LUA_DECODE_RESULT_MIDI "_decoded_midi"
#define GRID_LUA_DECODE_RESULT_SYSEX "_decoded_sysex"
#define GRID_LUA_DECODE_RESULT_EVIEW "_decoded_eview"

// clang-format on

enum {
  GRID_LUA_DECODE_ORDER_MIDI = 1,
  GRID_LUA_DECODE_ORDER_SYSEX = 2,
  GRID_LUA_DECODE_ORDER_EVIEW = 3,
};

#endif /* GRID_LUA_H */
