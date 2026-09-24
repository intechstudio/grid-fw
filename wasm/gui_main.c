// Browser front-end for the VSN1 module simulator (see sim_core.h). Owns
// the SDL canvas and keyboard capture; all module/Lua/LED/LCD logic lives
// in sim_core.c, shared with the headless (Node/browser-library) entry
// point in headless_main.c.

#include <SDL/SDL.h>
#include <stdio.h>

#include "sim_core.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// Maps a subset of the keyboard onto VSN1L's inputs. This is a first,
// minimal mapping to make the simulator interactively testable in a
// browser tab - revisit as real usage shows what's actually convenient.
//
//   1-8         -> buttons 0-7
//   q w e r     -> buttons 9-12
//   ArrowLeft   -> endless (element 8) rotate -1
//   ArrowRight  -> endless (element 8) rotate +1
//   Space       -> endless (element 8) push button

static const int KEYCODE_1 = 49;
static const int KEYCODE_8 = 56;
static const int KEYCODE_Q = 81;
static const int KEYCODE_W = 87;
static const int KEYCODE_E = 69;
static const int KEYCODE_R = 82;
static const int KEYCODE_LEFT = 37;
static const int KEYCODE_RIGHT = 39;
static const int KEYCODE_SPACE = 32;

void EMSCRIPTEN_KEEPALIVE grid_sim_gui_handle_key(int key_code, int down) {

  if (key_code >= KEYCODE_1 && key_code <= KEYCODE_8) {
    grid_sim_input_button((uint8_t)(key_code - KEYCODE_1), down);
    return;
  }

  uint8_t extra_button = 0xff;
  if (key_code == KEYCODE_Q) {
    extra_button = 9;
  } else if (key_code == KEYCODE_W) {
    extra_button = 10;
  } else if (key_code == KEYCODE_E) {
    extra_button = 11;
  } else if (key_code == KEYCODE_R) {
    extra_button = 12;
  }
  if (extra_button != 0xff) {
    grid_sim_input_button(extra_button, down);
    return;
  }

  if (key_code == KEYCODE_SPACE) {
    grid_sim_input_endless_button(GRID_SIM_ENDLESS_ELEMENT, down);
    return;
  }

  // Rotation only makes sense as a one-shot step on keydown.
  if (!down) {
    return;
  }

  if (key_code == KEYCODE_LEFT) {
    grid_sim_input_endless_rotate(GRID_SIM_ENDLESS_ELEMENT, -1);
  } else if (key_code == KEYCODE_RIGHT) {
    grid_sim_input_endless_rotate(GRID_SIM_ENDLESS_ELEMENT, 1);
  }
}

// Left over from the pre-simulator raw-Lua playground shell
// (wasm/shell_minimal.html's "Load Script" button) - kept as an inert stub
// purely so that leftover control doesn't throw a JS error if clicked.
void EMSCRIPTEN_KEEPALIVE loadScript(char* setup, char* loop) {}

#ifdef __EMSCRIPTEN__
EM_JS(void, grid_sim_gui_capture_input, (), {
  document.addEventListener('keydown', function(event) {
    Module.ccall('grid_sim_gui_handle_key', 'void', [ 'number', 'number' ], [ event.keyCode, 1 ]);
  });
  document.addEventListener('keyup', function(event) {
    Module.ccall('grid_sim_gui_handle_key', 'void', [ 'number', 'number' ], [ event.keyCode, 0 ]);
  });
});
#endif

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

#ifdef __EMSCRIPTEN__
  grid_sim_gui_capture_input();
#endif

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
