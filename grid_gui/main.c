// Copyright 2011 The Emscripten Authors.  All rights reserved.
// Emscripten is available under two separate licenses, the MIT license and the
// University of Illinois/NCSA Open Source License.  Both these licenses can be
// found in the LICENSE file.

#include "grid_font.h"
#include "grid_gui.h"
#include "grid_lua.h"
#include "grid_lua_api_gui.h"

#include "tinyalloc/tinyalloc.h"

#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

void grid_platform_delay_ms(uint32_t delay_milliseconds) { return; }
void grid_platform_printf(char const* fmt, ...) {

  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}

const size_t LUA_MEM_SIZE = 200000;
static uint8_t LUA_MEM[LUA_MEM_SIZE];

void* allocator(void* ud, void* ptr, size_t osize, size_t nsize) {
  // Free
  if (nsize == 0) {
    ta_free(ptr);
    return NULL;
  }
  // Realloc
  else if (ptr != NULL && nsize > osize) {
    void* new_ptr = ta_alloc(nsize);
    if (new_ptr == NULL) {
      printf("Out of memory 1\n");
    }
    memcpy(new_ptr, ptr, osize);
    ta_free(ptr);
    return new_ptr;
  }
  // Malloc
  else if (ptr == NULL && nsize > 0) {
    void* new_ptr = ta_alloc(nsize);
    if (new_ptr == NULL) {
      printf("Out of memory 2\n");
    }
    return new_ptr;
  } else {
    return ptr;
  }
}

#define ARENA_SIZE (100 * 1024) // 100 KB

struct ArenaAllocator {
  size_t size;
  size_t offset;
  uint8_t memory[ARENA_SIZE];
};

struct ArenaAllocator inst = {0};

void arena_init(struct ArenaAllocator* arena) {
  arena->size = ARENA_SIZE;
  arena->offset = 0;
}

void* arena_malloc(struct ArenaAllocator* arena, size_t size) {
  if (arena->offset + size > arena->size) {
    printf("Out of memory\n");
    return NULL; // Out of memory
  }
  // printf("Offset: %ld\n", (arena->offset)/1000);
  void* ptr = arena->memory + arena->offset;
  arena->offset += size;
  return ptr;
}

void arena_reset(struct ArenaAllocator* arena) { arena->offset = 0; }

void arena_free(struct ArenaAllocator* arena, void* ptr) {
  // This simple allocator does not support freeing individual allocations
  // It only supports resetting the entire arena.
}

static void* custom_lua_allocator(void* ud, void* ptr, size_t osize, size_t nsize) {
  struct ArenaAllocator* arena = (struct ArenaAllocator*)ud;

  if (nsize == 0) {
    // Lua requests to free the memory
    arena_free(arena, ptr);
    return NULL;
  } else if (ptr == NULL) {
    // Lua requests to allocate new memory
    return arena_malloc(arena, nsize);
  } else {
    // Lua requests to reallocate memory
    // Simple implementation: allocate new block and do not free old block
    void* new_ptr = arena_malloc(arena, nsize);
    if (new_ptr && osize > 0) {
      // Copy old data to new block
      memcpy(new_ptr, ptr, osize < nsize ? osize : nsize);
    }
    return new_ptr;
  }
}

uint8_t config_changed = 0;
char grid_lua_init_script[1024] = {0};
char grid_lua_loop_script[1024] = {0};

#include <SDL/SDL.h>
#include <stdio.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>

EM_JS(void, captureInput, (), {
  console.log('hello world!');
  // JavaScript code

  // document.addEventListener('keydown', function(event) {
  //   // Pass the key code to the C function

  //   console.log('!');
  //   Module.ccall('handleInput', 'void', ['number'], [event.keyCode]);
  // });

  Module.doNotCaptureKeyboard = true;

  window.addEventListener('keydown',
                          function(event){
                              // event.stopImmediatePropagation();
                          },
                          true);

  window.addEventListener('keyup',
                          function(event){
                              // event.stopImmediatePropagation();
                          },
                          true);

  var button = document.querySelector('#loadScriptButton');

  // Add an event listener to the button
  button.addEventListener('click', function(){

                                   });
});

void EMSCRIPTEN_KEEPALIVE handleInput(int keyCode) {
  // Print the captured key code
  printf("Key pressed: %d\n", keyCode);
  // loopcounter = keyCode;
}

void EMSCRIPTEN_KEEPALIVE loadScript(char* setup, char* loop) {
  // Print the captured key code
  strcpy(grid_lua_init_script, setup);
  strcpy(grid_lua_loop_script, loop);

  printf("loadScript len %d |||||| %d\n", strlen(grid_lua_init_script), strlen(grid_lua_loop_script));

  config_changed = 1;
  // loopcounter = keyCode;
}
#endif

struct grid_vlcd_model {

  SDL_Surface* screen;
};

struct grid_vlcd_model grid_vlcd_state;

uint16_t loopcounter = 0;

uint8_t quit = 0;

void draw_screen(struct grid_gui_model* gui) {

  SDL_Surface* screen = ((struct grid_vlcd_model*)gui->screen_handle)->screen;

  if (SDL_MUSTLOCK(screen))
    SDL_LockSurface(screen);

  uint8_t* dest = screen->pixels;
  uint8_t* src = gui->buffer;

  for (int x = 0; x < gui->width; ++x) {
    for (int y = 0; y < gui->height; ++y) {
      uint32_t index_in = (x * gui->height + y) * 3;
      uint32_t index_out = (y * gui->width + x) * 4;
      dest[index_out + 0] = src[index_in + 0];
      dest[index_out + 1] = src[index_in + 1];
      dest[index_out + 2] = src[index_in + 2];
      dest[index_out + 3] = 255;
    }
  }

  if (SDL_MUSTLOCK(screen))
    SDL_UnlockSurface(screen);

  SDL_Flip(screen);
}

void loop(void) {

  if (config_changed) {
    config_changed = 0;
    grid_lua_dostring(&grid_lua_state, grid_lua_init_script);
  }

  grid_lua_dostring(&grid_lua_state, grid_lua_loop_script);

  // lua_pushinteger(grid_lua_state.L, 0);
  // l_grid_gui_draw_demo(grid_lua_state.L);

  char buffer[1024] = {0};
  snprintf(buffer, 1024, "loopcounter %d", loopcounter);

  loopcounter++;

  draw_screen(&grid_gui_states[0]);

  // printf("loop %d\n", loopcounter);
}

uint8_t framebuffer[320 * 240 * 3] = {0};

int main(int argc, char** argv) {

  arena_init(&inst);

  ta_init(LUA_MEM, LUA_MEM + LUA_MEM_SIZE - 1, 2048, 16, 4);

  grid_lua_init(&grid_lua_state, allocator, &inst);

  grid_lua_start_vm(&grid_lua_state);
  grid_lua_vm_register_functions(&grid_lua_state, grid_lua_api_gui_lib_reference);

  struct grid_gui_model* gui = &grid_gui_states[0];
  struct grid_vlcd_model* vlcd = &grid_vlcd_state;

  grid_font_init(&grid_font_state);
  grid_gui_init(gui, vlcd, framebuffer, sizeof(framebuffer), 320, 240);

  printf("hello, world!\n");

  emscripten_run_script("captureInput();");

  SDL_Init(SDL_INIT_VIDEO);
  vlcd->screen = SDL_SetVideoMode(gui->width, gui->height, 32, SDL_SWSURFACE);

  emscripten_set_main_loop(loop, 30, 1);

  printf("you should see a smoothly-colored square - no sharp lines but the square borders!\n");
  printf("and here is some text that should be HTML-friendly: amp: |&| double-quote: |\"| quote: |'| less-than, greater-than, html-like tags: |<cheez></cheez>|\nanother line.\n");

  // SDL_Quit();

  return 0;
}
