// Browser front-end for the VSN1 module simulator (see sim_core.h). Owns
// the SDL canvas; all module/Lua/LED/LCD logic lives in sim_core.c, shared
// with the headless (Node/browser-library) entry point in headless_main.c.
//
// No keyboard-driven module controls here - buttons/dial are pointer-only
// (wasm/shell_minimal.html), and Module.doNotCaptureKeyboard=true (set
// there) tells emscripten's SDL layer not to intercept keyboard events at
// all, so normal browser text-input behavior (e.g. the send console) is
// never in competition with anything SDL-related on this page.

#include <SDL/SDL.h>
#include <stdio.h>

#include "sim_core.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// Left over from the pre-simulator raw-Lua playground shell
// (wasm/shell_minimal.html's "Load Script" button) - kept as an inert stub
// purely so that leftover control doesn't throw a JS error if clicked.
void EMSCRIPTEN_KEEPALIVE loadScript(char* setup, char* loop) {}

struct grid_vlcd_model {
  SDL_Surface* screen;
};

static struct grid_vlcd_model grid_vlcd_state;

static void draw_screen(void) {

  SDL_Surface* screen = grid_vlcd_state.screen;

  if (SDL_MUSTLOCK(screen)) {
    SDL_LockSurface(screen);
  }

  uint8_t* dest = screen->pixels;
  uint8_t* src = grid_sim_get_lcd_framebuffer();

  for (int x = 0; x < GRID_SIM_LCD_WIDTH; ++x) {
    for (int y = 0; y < GRID_SIM_LCD_HEIGHT; ++y) {
      uint32_t index_in = (x * GRID_SIM_LCD_HEIGHT + y) * GRID_SIM_LCD_BYTES_PER_PIXEL;
      uint32_t index_out = (y * GRID_SIM_LCD_WIDTH + x) * 4;
      dest[index_out + 0] = src[index_in + 0];
      dest[index_out + 1] = src[index_in + 1];
      dest[index_out + 2] = src[index_in + 2];
      dest[index_out + 3] = 255;
    }
  }

  if (SDL_MUSTLOCK(screen)) {
    SDL_UnlockSurface(screen);
  }

  SDL_Flip(screen);
}

static void loop(void) {

  // 30 fps, matching emscripten_set_main_loop below.
  grid_sim_tick(1000 / 30);

  draw_screen();
}

int main(int argc, char** argv) {

  grid_sim_init();

  printf("grid module simulator (VSN1L) ready\n");

  SDL_Init(SDL_INIT_VIDEO);
  grid_vlcd_state.screen = SDL_SetVideoMode(GRID_SIM_LCD_WIDTH, GRID_SIM_LCD_HEIGHT, 32, SDL_SWSURFACE);

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(loop, 30, 1);
#else
  while (1) {
    loop();
  }
#endif

  return 0;
}
