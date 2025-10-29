#include "grid_lua.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "grid_protocol.h"
#include "grid_transport.h"

struct grid_lua_model grid_lua_state;

void grid_lua_init(struct grid_lua_model* lua, void* (*custom_allocator)(void*, void*, size_t, size_t), void* custom_allocator_instance) {

  lua->stdo_len = GRID_LUA_STDO_LENGTH;
  lua->stdi_len = GRID_LUA_STDI_LENGTH;
  lua->stde_len = GRID_LUA_STDE_LENGTH;

  lua->custom_allocator = custom_allocator;
  lua->custom_allocator_instance = custom_allocator_instance;

  lua->busy_semaphore = NULL;
  lua->busy_semaphore_lock_fn = NULL;
  lua->busy_semaphore_release_fn = NULL;

  grid_lua_clear_stdo(lua);
  grid_lua_clear_stdi(lua);
  grid_lua_clear_stde(lua);

  lua->dostring_count = 0;

  lua->L = NULL;

  lua->target_memory_usage_kilobytes = 70; // 70kb
}

void grid_lua_semaphore_init(struct grid_lua_model* lua, void* lua_busy_semaphore, void (*lock_fn)(void*), void (*release_fn)(void*)) {

  lua->busy_semaphore = lua_busy_semaphore;
  lua->busy_semaphore_lock_fn = lock_fn;
  lua->busy_semaphore_release_fn = release_fn;
}

bool grid_lua_semaphore_lock(struct grid_lua_model* lua) {

  assert(lua->L);

  if (lua->busy_semaphore == NULL) {
    return false;
  }

  if (lua->busy_semaphore_lock_fn == NULL) {
    return false;
  }

  lua->busy_semaphore_lock_fn(lua->busy_semaphore);

  return true;
}

bool grid_lua_semaphore_release(struct grid_lua_model* lua) {

  assert(lua->L);

  if (lua->busy_semaphore == NULL) {
    return false;
  }

  if (lua->busy_semaphore_release_fn == NULL) {
    return false;
  }

  lua->busy_semaphore_release_fn(lua->busy_semaphore);

  return true;
}

void grid_lua_deinit(struct grid_lua_model* lua) {}

void grid_lua_pre_init(struct grid_lua_model* lua) {

  grid_lua_dostring(lua, "init_simple_color() "
                         "init_simple_midi() "
                         "init_auto_value() ");
  grid_lua_clear_stdo(lua);
}

void grid_lua_post_init(struct grid_lua_model* lua) {

  grid_lua_dostring(lua, "ele[#ele]:post_init_cb() "
                         "for i = 0, #ele-1 do ele[i]:post_init_cb() end");
  grid_lua_clear_stdo(lua);
}

void grid_lua_clear_stdi(struct grid_lua_model* lua) { memset(lua->stdi, 0, lua->stdi_len); }

void grid_lua_clear_stdo(struct grid_lua_model* lua) { memset(lua->stdo, 0, lua->stdo_len); }

void grid_lua_clear_stde(struct grid_lua_model* lua) { memset(lua->stde, 0, lua->stde_len); }

int grid_lua_append_stdo(struct grid_lua_model* lua, char* str) {

  int curr = strnlen(lua->stdo, lua->stdo_len);

  int add = strlen(str);

  int remain = lua->stdo_len - 1 - curr;

  if (add > remain) {
    return 1;
  }

  strcat(lua->stdo, str);

  return 0;
}

int grid_lua_append_stde(struct grid_lua_model* lua, char* str) {

  int curr = strnlen(lua->stde, lua->stde_len);

  int add = strlen(str);

  int remain = lua->stde_len - 1 - curr;

  if (add > remain) {
    return 1;
  }

  strcat(lua->stde, str);

  return 0;
}

char* grid_lua_get_output_string(struct grid_lua_model* lua) { return lua->stdo; }

char* grid_lua_get_error_string(struct grid_lua_model* lua) { return lua->stde; }

uint32_t grid_lua_dostring_unsafe(struct grid_lua_model* lua, const char* code) {

  assert(lua->L);

  lua->dostring_count++;

  uint32_t ret = 1;

  if (luaL_loadstring(lua->L, code) == LUA_OK) {

    if ((lua_pcall(lua->L, 0, LUA_MULTRET, 0)) != LUA_OK) {

      grid_lua_clear_stde(lua);
      stpncpy(lua->stde, lua_tostring(lua->L, -1), lua->stde_len - 1);
      ret = 0;
    }

    // Pop everything from the stack
    lua_pop(lua->L, lua_gettop(lua->L));

  } else {

    grid_lua_clear_stde(lua);
    stpncpy(lua->stde, lua_tostring(lua->L, -1), lua->stde_len - 1);
    ret = 0;

    // Pop error message from the stack
    lua_pop(lua->L, 1);
  }

  return ret;
}

uint32_t grid_lua_dostring(struct grid_lua_model* lua, const char* code) {

  grid_lua_semaphore_lock(lua);

  uint32_t ret = grid_lua_dostring_unsafe(lua, code);

  grid_lua_gc_step_unsafe(lua);

  grid_lua_semaphore_release(lua);

  return ret;
}

bool grid_lua_do_event(struct grid_lua_model* lua, uint8_t index, const char* function_name) {

  bool ret = false;

  grid_lua_semaphore_lock(lua);

  // Attempt to get element table
  if (lua_getglobal(lua->L, "ele") != LUA_TTABLE) {
    goto grid_lua_do_event_cleanup;
  }

  // Push element index
  lua_pushinteger(lua->L, index);

  // Attempt to index the element table
  if (lua_gettable(lua->L, -2) != LUA_TTABLE) {
    goto grid_lua_do_event_cleanup;
  }

  // Push event name
  lua_pushstring(lua->L, function_name);

  // Attempt to index the element
  if (lua_gettable(lua->L, -2) != LUA_TFUNCTION) {
    goto grid_lua_do_event_cleanup;
  }

  // Remove element table from stack
  lua_remove(lua->L, -3);

  // Move the event function below the element
  lua_insert(lua->L, -2);

  // Invoke event function, passing the element as self
  if (lua_pcall(lua->L, 1, LUA_MULTRET, 0) != LUA_OK) {
    grid_lua_clear_stde(lua);
    grid_lua_append_stde(lua, lua_tostring(lua->L, -1));
    goto grid_lua_do_event_cleanup;
  }

  ret = true;

grid_lua_do_event_cleanup:

  lua_pop(lua->L, lua_gettop(lua->L));
  grid_lua_semaphore_release(lua);
  return ret;
}

void grid_lua_decode_clear_results(struct grid_lua_model* lua) {

  grid_lua_semaphore_lock(lua);

  // Get lengths of decode result tables
  lua_getglobal(lua->L, GRID_LUA_DECODE_RESULT_MIDI);
  if (lua_type(lua->L, -1) != LUA_TTABLE) {
    goto grid_lua_decode_clear_results_cleanup;
  }
  size_t midi_len = lua_rawlen(lua->L, -1);

  lua_getglobal(lua->L, GRID_LUA_DECODE_RESULT_SYSEX);
  if (lua_type(lua->L, -1) != LUA_TTABLE) {
    goto grid_lua_decode_clear_results_cleanup;
  }
  size_t sysex_len = lua_rawlen(lua->L, -1);

  lua_getglobal(lua->L, GRID_LUA_DECODE_RESULT_EVIEW);
  if (lua_type(lua->L, -1) != LUA_TTABLE) {
    goto grid_lua_decode_clear_results_cleanup;
  }
  size_t eview_len = lua_rawlen(lua->L, -1);

  // If the tables are empty, there are no results to be cleared
  if (midi_len == 0 && sysex_len == 0 && eview_len == 0) {
    goto grid_lua_decode_clear_results_cleanup;
  }

  if (lua_getglobal(lua->L, GRID_LUA_DECODE_CLEARER) != LUA_TFUNCTION) {
    goto grid_lua_decode_clear_results_cleanup;
  }

  // Invoke decode result clearing function
  if (lua_pcall(lua->L, 0, 0, 0) != LUA_OK) {
    grid_lua_clear_stde(lua);
    grid_lua_append_stde(lua, lua_tostring(lua->L, -1));
    goto grid_lua_decode_clear_results_cleanup;
  }

grid_lua_decode_clear_results_cleanup:

  lua_pop(lua->L, lua_gettop(lua->L));
  grid_lua_semaphore_release(lua);
}

void grid_lua_decode_process_results(struct grid_lua_model* lua) {

  grid_lua_semaphore_lock(lua);

  // Get lengths of decode result tables and leave them on the stack
  lua_getglobal(lua->L, GRID_LUA_DECODE_RESULT_MIDI);
  if (lua_type(lua->L, -1) != LUA_TTABLE) {
    goto grid_lua_decode_process_results_cleanup;
  }
  size_t midi_len = lua_rawlen(lua->L, -1);

  lua_getglobal(lua->L, GRID_LUA_DECODE_RESULT_SYSEX);
  if (lua_type(lua->L, -1) != LUA_TTABLE) {
    goto grid_lua_decode_process_results_cleanup;
  }
  size_t sysex_len = lua_rawlen(lua->L, -1);

  lua_getglobal(lua->L, GRID_LUA_DECODE_RESULT_EVIEW);
  if (lua_type(lua->L, -1) != LUA_TTABLE) {
    goto grid_lua_decode_process_results_cleanup;
  }
  size_t eview_len = lua_rawlen(lua->L, -1);

  // If the tables are empty, there are no results to be processed
  if (midi_len == 0 && sysex_len == 0 && eview_len == 0) {
    goto grid_lua_decode_process_results_cleanup;
  }

  if (lua_getglobal(lua->L, GRID_LUA_DECODE_PROCESSOR) != LUA_TFUNCTION) {
    goto grid_lua_decode_process_results_cleanup;
  }

  // Move the processor function below the result tables
  lua_insert(lua->L, -4);

  // Invoke decode result processor function
  if (lua_pcall(lua->L, 3, 0, 0) != LUA_OK) {
    grid_lua_clear_stde(lua);
    grid_lua_append_stde(lua, lua_tostring(lua->L, -1));
    goto grid_lua_decode_process_results_cleanup;
  }

grid_lua_decode_process_results_cleanup:

  lua_pop(lua->L, lua_gettop(lua->L));
  grid_lua_gc_step_unsafe(lua);
  grid_lua_semaphore_release(lua);
}

void grid_lua_broadcast_stdo(struct grid_lua_model* lua) {

  char* str = grid_lua_get_output_string(lua);

  if (str[0] == '\0') {
    return;
  }

  struct grid_msg msg;
  uint8_t xy = GRID_PARAMETER_GLOBAL_POSITION;
  grid_msg_init_brc(&grid_msg_state, &msg, xy, xy);

  grid_msg_nprintf(&msg, "%s", str);
  grid_lua_clear_stdo(&grid_lua_state);

  if (grid_msg_close_brc(&grid_msg_state, &msg) >= 0) {
    grid_transport_send_msg_to_all(&grid_transport_state, &msg);
  }
}

void grid_lua_broadcast_stde(struct grid_lua_model* lua) {

  char* str = grid_lua_get_error_string(lua);

  if (str[0] == '\0') {
    return;
  }

  grid_port_debug_printf("LUA not OK! MSG: %s", str);
  grid_platform_printf("LUA not OK! MSG: %s\n", str);

  grid_lua_clear_stde(lua);
}

void grid_lua_set_memory_target(struct grid_lua_model* lua, uint8_t target_kilobytes) { lua->target_memory_usage_kilobytes = target_kilobytes; }

uint8_t grid_lua_get_memory_target(struct grid_lua_model* lua) { return lua->target_memory_usage_kilobytes; }

void grid_lua_gc_full_unsafe(struct grid_lua_model* lua) {

  assert(lua->L);

  lua_gc(lua->L, LUA_GCCOLLECT);
}

void grid_lua_gc_step_unsafe(struct grid_lua_model* lua) {

  assert(lua->L);

  uint8_t target_kilobytes = grid_lua_get_memory_target(lua);

  if (lua_gc(lua->L, LUA_GCCOUNT) > target_kilobytes) {

    lua_gc(lua->L, LUA_GCSTEP, 1);

    // char message[10] = {0};
    // sprintf(message, "gc %dkb", target_kilobytes);
    // grid_lua_debug_memory_stats(lua, message);
    lua->dostring_count = 0;
  }
}

uint8_t grid_lua_gc_count_unsafe(struct grid_lua_model* lua) { return lua_gc(lua->L, LUA_GCCOUNT); }

void grid_lua_debug_memory_stats(struct grid_lua_model* lua, char* message) {

  uint32_t memusage = lua_gc(grid_lua_state.L, LUA_GCCOUNT) * 1024 + lua_gc(grid_lua_state.L, LUA_GCCOUNTB);
  grid_platform_printf("LUA mem usage: %d(%s)\r\n", memusage, message);
}

/*static*/ int grid_lua_panic(lua_State* L) {

  while (1) {

    grid_platform_printf("LUA PANIC\r\n");
    grid_platform_delay_ms(1000);
  }

  return 1;
}

int grid_lua_register_functions_unsafe(struct grid_lua_model* lua, const struct luaL_Reg* lua_lib) {

  assert(lua->L);

  lua_getglobal(lua->L, "_G");
  luaL_setfuncs(lua->L, lua_lib, 0);
  lua_pop(lua->L, 1);

  return 0;
}

void grid_lua_ui_init_unsafe(struct grid_lua_model* lua, lua_ui_init_callback_t callback) {

  assert(lua->L);

  if (callback == NULL) {

    grid_platform_printf("LUA UI INIT FAILED: callback not registered\r\n");
    return;
  }

  callback(lua);

  grid_lua_gc_full_unsafe(lua);

  // grid_lua_debug_memory_stats(lua, "Ui init");
}

void grid_lua_start_vm(struct grid_lua_model* lua, const struct luaL_Reg* lua_lib, lua_ui_init_callback_t callback) {

  assert(!lua->L);

  grid_platform_printf("grid_lua_start_vm\n");

  // grid_lua_semaphore_lock(lua);

  if (lua->custom_allocator == NULL) {
    lua->L = luaL_newstate();
  } else {
    lua->L = lua_newstate(lua->custom_allocator, lua->custom_allocator_instance);
  }

  lua_atpanic(lua->L, &grid_lua_panic);

  // grid_lua_debug_memory_stats(lua, "Init");

  // luaL_openlibs(lua->L);

  static const luaL_Reg loadedlibs[] = {{LUA_GNAME, luaopen_base},
                                        //{LUA_LOADLIBNAME, luaopen_package},
                                        //{LUA_COLIBNAME, luaopen_coroutine},
                                        {LUA_TABLIBNAME, luaopen_table},
                                        {LUA_IOLIBNAME, luaopen_io},
                                        {LUA_OSLIBNAME, luaopen_os},
                                        {LUA_STRLIBNAME, luaopen_string},
                                        {LUA_MATHLIBNAME, luaopen_math},
                                        //{LUA_UTF8LIBNAME, luaopen_utf8},
                                        {LUA_DBLIBNAME, luaopen_debug},
                                        {NULL, NULL}};

  const luaL_Reg* lib;
  /* "require" functions from 'loadedlibs' and set results to global table */
  for (lib = loadedlibs; lib->func; lib++) {
    luaL_requiref(lua->L, lib->name, lib->func, 1);
    lua_pop(lua->L, 1); /* remove lib */
  }

  // grid_lua_debug_memory_stats(lua, "Openlibs");

  grid_lua_dostring_unsafe(lua, GRID_LUA_FNC_G_DECODE_source);
  grid_lua_dostring_unsafe(lua, GRID_LUA_FNC_G_LOOKUP_source);
  grid_lua_dostring_unsafe(lua, GRID_LUA_FNC_G_LIMIT_source);
  grid_lua_dostring_unsafe(lua, GRID_LUA_FNC_G_ELEMENTNAME_source);
  grid_lua_dostring_unsafe(lua, GRID_LUA_FNC_G_EVENTFNAME_source);
  grid_lua_dostring_unsafe(lua, GRID_LUA_FNC_G_MAPSAT_source);
  grid_lua_dostring_unsafe(lua, GRID_LUA_FNC_G_SIGN_source);
  grid_lua_dostring_unsafe(lua, GRID_LUA_FNC_G_SEGCALC_source);
  grid_lua_dostring_unsafe(lua, "midi_fifo = {}");
  // grid_lua_dostring_unsafe(lua, GRID_LUA_FNC_G_TOML_source);
  grid_lua_dostring_unsafe(lua, GRID_LUA_FNC_G_SIMPLECOLOR_source);
  grid_lua_dostring_unsafe(lua, GRID_LUA_FNC_G_SIMPLEMIDI_source);
  grid_lua_dostring_unsafe(lua, GRID_LUA_FNC_G_AUTOVALUE_source);
  grid_lua_dostring_unsafe(lua, "midi_fifo_highwater = 0");
  grid_lua_dostring_unsafe(lua, "midi_fifo_retriggercount = 0");
  grid_lua_dostring_unsafe(lua, "midi = {}");
  grid_lua_dostring_unsafe(lua, "midi.send_packet = function "
                                "(self,ch,cmd,p1,p2) " GRID_LUA_FNC_G_MIDI_SEND_short "(ch,cmd,p1,p2) end");

  grid_lua_dostring_unsafe(lua, "mouse = {}");
  grid_lua_dostring_unsafe(lua, "mouse.send_axis_move = function "
                                "(self,p,a) " GRID_LUA_FNC_G_MOUSEMOVE_SEND_short "(p,a) end");
  grid_lua_dostring_unsafe(lua, "mouse.send_button_change = function "
                                "(self,s,b) " GRID_LUA_FNC_G_MOUSEBUTTON_SEND_short "(s,b) end");

  grid_lua_dostring_unsafe(lua, "keyboard = {}");
  grid_lua_dostring_unsafe(lua, "keyboard.send_macro = function "
                                "(self,...) " GRID_LUA_FNC_G_KEYBOARD_SEND_short "(...) end");

  grid_lua_gc_full_unsafe(lua);

  grid_lua_register_functions_unsafe(lua, lua_lib);
  grid_lua_ui_init_unsafe(&grid_lua_state, callback);

  lua_newtable(lua->L);
  lua_setglobal(lua->L, GRID_LUA_DECODE_ORDER);
  lua_newtable(lua->L);
  lua_setglobal(lua->L, GRID_LUA_DECODE_RESULT_MIDI);
  lua_newtable(lua->L);
  lua_setglobal(lua->L, GRID_LUA_DECODE_RESULT_SYSEX);
  lua_newtable(lua->L);
  lua_setglobal(lua->L, GRID_LUA_DECODE_RESULT_EVIEW);

  grid_lua_semaphore_release(lua);
}

void grid_lua_stop_vm(struct grid_lua_model* lua) {

  if (lua->L != NULL) {

    grid_platform_printf("grid_lua_stop_vm\n");
    grid_lua_semaphore_lock(lua);
    lua_close(lua->L);
    lua->L = NULL;
  }
}
