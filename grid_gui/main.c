// Copyright 2011 The Emscripten Authors.  All rights reserved.
// Emscripten is available under two separate licenses, the MIT license and the
// University of Illinois/NCSA Open Source License.  Both these licenses can be
// found in the LICENSE file.

#include <stdio.h>
#include <SDL/SDL.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>


EM_JS(void, captureInput, (), {
  console.log('hello world!');
// JavaScript code

      document.addEventListener('keydown', function(event) {
          // Pass the key code to the C function

          console.log('!');
          Module.ccall('handleInput', 'void', ['number'], [event.keyCode]);
      });
  
});

#endif

uint16_t loopcounter = 0;
SDL_Surface *screen = NULL;


uint8_t quit = 0;

uint8_t* frame_buffer = NULL;
uint16_t screen_width = 320;
uint16_t screen_height = 240;


void loop(void){
      char c;

  loopcounter++;

  if (SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);

  for (int i = 0; i < screen_width; i++) {
    for (int j = 0; j < screen_height; j++) {
      
      
      uint32_t index = (j * screen_width + i)*4;

      frame_buffer[index] = i/2;
      frame_buffer[index+1] = j;
      frame_buffer[index+2] = 255-i/2;
      frame_buffer[index+3] = 255-i/2;
    }
  }
  if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);

  SDL_Flip(screen);

  printf("loop %d\n", loopcounter);

}

void draw_rectangle(int x, int y, int width, int height) {



}

void EMSCRIPTEN_KEEPALIVE handleInput(int keyCode) {
    // Print the captured key code
    printf("Key pressed: %d\n", keyCode);
    loopcounter = keyCode;
}

int main(int argc, char** argv) {
  printf("hello, world!\n");

  emscripten_run_script("captureInput();");

  SDL_Init(SDL_INIT_VIDEO);
  screen = SDL_SetVideoMode(screen_width, screen_height, 16, SDL_SWSURFACE);

  frame_buffer = ((uint8_t*)screen->pixels);

  emscripten_set_main_loop(loop, 30, 1);


  printf("you should see a smoothly-colored square - no sharp lines but the square borders!\n");
  printf("and here is some text that should be HTML-friendly: amp: |&| double-quote: |\"| quote: |'| less-than, greater-than, html-like tags: |<cheez></cheez>|\nanother line.\n");

  //SDL_Quit();

  return 0;
}