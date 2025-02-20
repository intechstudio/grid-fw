#include "grid_lua.h"

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
void grid_lua_semaphore_lock(struct grid_lua_model* lua) {

  if (lua->L == NULL) {
    grid_platform_printf("LUA model not initialized\n");
    return;
  }

  if (lua->busy_semaphore == NULL) {
    return;
  }

  if (lua->busy_semaphore_lock_fn == NULL) {
    return;
  }

  lua->busy_semaphore_lock_fn(lua->busy_semaphore);
}
void grid_lua_semaphore_release(struct grid_lua_model* lua) {

  if (lua->busy_semaphore == NULL) {
    return;
  }

  if (lua->busy_semaphore_lock_fn == NULL) {
    return;
  }

  lua->busy_semaphore_release_fn(lua->busy_semaphore);
}

void grid_lua_deinit(struct grid_lua_model* lua) {}

void grid_lua_clear_stdi(struct grid_lua_model* lua) {

  for (uint32_t i = 0; i < lua->stdi_len; i++) {
    lua->stdi[i] = 0;
  }
}

void grid_lua_clear_stdo(struct grid_lua_model* lua) {

  for (uint32_t i = 0; i < lua->stdo_len; i++) {
    lua->stdo[i] = 0;
  }
}

void grid_lua_clear_stde(struct grid_lua_model* lua) {

  for (uint32_t i = 0; i < lua->stde_len; i++) {
    lua->stde[i] = 0;
  }
}

char* grid_lua_get_output_string(struct grid_lua_model* lua) { return lua->stdo; }

char* grid_lua_get_error_string(struct grid_lua_model* lua) { return lua->stde; }

uint32_t grid_lua_dostring(struct grid_lua_model* lua, const char* code) {

  grid_lua_semaphore_lock(lua);

  lua->dostring_count++;

  uint32_t is_ok = 1;

  if (luaL_loadstring(lua->L, code) == LUA_OK) {

    if ((lua_pcall(lua->L, 0, LUA_MULTRET, 0)) == LUA_OK) {
      // If it was executed successfully we
      // remove the code from the stack
    } else {
      // grid_platform_printf("LUA not OK: %s \r\n", code);
      grid_lua_clear_stde(lua);
      stpncpy(lua->stde, lua_tostring(lua->L, -1), lua->stde_len - 1);
      is_ok = 0;
    }

    lua_pop(lua->L, lua_gettop(lua->L));
  } else {
    // grid_platform_printf("LUA not OK:  %s\r\n", code);
    // grid_port_debug_printf("LUA not OK 2");

    grid_lua_clear_stde(lua);
    stpncpy(lua->stde, lua_tostring(lua->L, -1), lua->stde_len - 1);
    is_ok = 0;

    lua_pop(lua->L, 1); // Remove error message from the stack
  }

  grid_lua_semaphore_release(lua);
  grid_lua_gc_try_collect(lua);

  return is_ok;
}

void grid_lua_set_memory_target(struct grid_lua_model* lua, uint8_t target_kilobytes) { lua->target_memory_usage_kilobytes = target_kilobytes; }

uint8_t grid_lua_get_memory_target(struct grid_lua_model* lua) { return lua->target_memory_usage_kilobytes; }

void grid_lua_gc_try_collect(struct grid_lua_model* lua) {

  if (lua->L == NULL) {
    grid_platform_printf("LUA model not initialized\n");
    return;
  }

  grid_lua_semaphore_lock(lua);

  uint8_t target_kilobytes = grid_lua_get_memory_target(lua);

  if (lua_gc(lua->L, LUA_GCCOUNT) > target_kilobytes) {

    lua_gc(lua->L, LUA_GCCOLLECT);

    // char message[10] = {0};
    // sprintf(message, "gc %dkb", target_kilobytes);
    // grid_lua_debug_memory_stats(lua, message);
    lua->dostring_count = 0;
  }

  grid_lua_semaphore_release(lua);
}

void grid_lua_gc_collect(struct grid_lua_model* lua) {
  grid_lua_semaphore_lock(lua);
  lua_gc(lua->L, LUA_GCCOLLECT);
  grid_lua_semaphore_release(lua);
}

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

/* ====================  MODULE SPECIFIC INITIALIZERS  ====================*/

int grid_lua_vm_register_functions(struct grid_lua_model* lua, struct luaL_Reg* lua_lib) {

  grid_lua_semaphore_lock(lua);

  lua_getglobal(lua->L, "_G");
  luaL_setfuncs(lua->L, lua_lib, 0);
  lua_pop(lua->L, 1);

  grid_lua_semaphore_release(lua);
  return 0;
}

void grid_lua_ui_init(struct grid_lua_model* lua, lua_ui_init_callback_t callback) {

  if (callback == NULL) {

    grid_platform_printf("LUA UI INIT FAILED: callback not registered\r\n");
    return;
  }

  callback(lua);

  // grid_lua_debug_memory_stats(lua, "Ui init");
}

void grid_lua_start_vm(struct grid_lua_model* lua) {

  grid_platform_printf("START VM\n");

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

  grid_lua_semaphore_release(lua);
  grid_lua_dostring(lua, GRID_LUA_FNC_G_LOOKUP_source);
  grid_lua_dostring(lua, GRID_LUA_FNC_G_LIMIT_source);
  grid_lua_dostring(lua, GRID_LUA_FNC_G_ELEMENTNAME_source);
  grid_lua_dostring(lua, GRID_LUA_FNC_G_MAPSAT_source);
  grid_lua_dostring(lua, GRID_LUA_FNC_G_SIGN_source);
  grid_lua_dostring(lua, GRID_LUA_FNC_G_SEGCALC_source);
  grid_lua_dostring(lua, "midi_fifo = {}");
  grid_lua_dostring(lua, "midi_fifo_highwater = 0");
  grid_lua_dostring(lua, "midi_fifo_retriggercount = 0");
  grid_lua_dostring(lua, "midi = {}");
  grid_lua_dostring(lua, "midi.send_packet = function "
                         "(self,ch,cmd,p1,p2) " GRID_LUA_FNC_G_MIDI_SEND_short "(ch,cmd,p1,p2) end");

  grid_lua_dostring(lua, "mouse = {}");
  grid_lua_dostring(lua, "mouse.send_axis_move = function "
                         "(self,p,a) " GRID_LUA_FNC_G_MOUSEMOVE_SEND_short "(p,a) end");
  grid_lua_dostring(lua, "mouse.send_button_change = function "
                         "(self,s,b) " GRID_LUA_FNC_G_MOUSEBUTTON_SEND_short "(s,b) end");

  grid_lua_dostring(lua, "keyboard = {}");
  grid_lua_dostring(lua, "keyboard.send_macro = function "
                         "(self,...) " GRID_LUA_FNC_G_KEYBOARD_SEND_short "(...) end");

  grid_lua_semaphore_lock(lua);
  grid_lua_semaphore_release(lua);
}

void grid_lua_stop_vm(struct grid_lua_model* lua) {

  grid_lua_semaphore_lock(lua);
  grid_platform_printf("STOP VM\n");

  if (lua->L != NULL) {

    lua_close(lua->L);
    lua->L = NULL;
  }
}
