// Copyright 2011 The Emscripten Authors.  All rights reserved.
// Emscripten is available under two separate licenses, the MIT license and the
// University of Illinois/NCSA Open Source License.  Both these licenses can be
// found in the LICENSE file.

#include "../grid_esp/components/grid_esp32_lcd/grid_gui.h"
#include <SDL/SDL.h>
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

EM_JS(void, captureInput, (), {
  console.log('hello world!');
  // JavaScript code

  document.addEventListener(
      'keydown', function(event) {
        // Pass the key code to the C function

        console.log('!');
        Module.ccall('handleInput', 'void', ['number'], [event.keyCode]);
      });
});

void EMSCRIPTEN_KEEPALIVE handleInput(int keyCode) {
  // Print the captured key code
  printf("Key pressed: %d\n", keyCode);
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

  for (int i = 0; i < gui->width; i++) {
    for (int j = 0; j < gui->height; j++) {

      uint32_t index_in_buffer = (j * gui->width + i) * 3;
      uint32_t index_out_buffer = (j * gui->width + i) * 4;
      ;
      ((uint8_t*)screen->pixels)[index_out_buffer] = gui->framebuffer[index_in_buffer];
      ((uint8_t*)screen->pixels)[index_out_buffer + 1] = gui->framebuffer[index_in_buffer + 1];
      ((uint8_t*)screen->pixels)[index_out_buffer + 2] = gui->framebuffer[index_in_buffer + 2];
      ((uint8_t*)screen->pixels)[index_out_buffer + 3] = 255;
    }
  }
  if (SDL_MUSTLOCK(screen))
    SDL_UnlockSurface(screen);

  SDL_Flip(screen);
}

void loop(void) {

  loopcounter++;
  grid_gui_draw_demo(&grid_gui_state, loopcounter);

  grid_gui_draw_rectangle(&grid_gui_state, 10, 10, 20, 20, grid_gui_color_from_rgb(0, 0, 255));

  draw_screen(&grid_gui_state);

  printf("loop %d\n", loopcounter);
}

uint8_t framebuffer[320 * 240 * 3] = {0};

int main(int argc, char** argv) {

  struct grid_gui_model* gui = &grid_gui_state;
  struct grid_vlcd_model* vlcd = &grid_vlcd_state;

  grid_gui_init(gui, vlcd, framebuffer, sizeof(framebuffer), 24, 320, 240);

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
