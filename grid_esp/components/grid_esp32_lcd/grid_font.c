#include "grid_font.h"
#include "grid_gui.h"

struct grid_font_model grid_font_state;

static char memory[57000] = {0};

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef EMSCRIPTEN
#define STBTT_malloc(x, u)                                                                                                                                                                             \
  (/*printf("Allocating %lu bytes to ", (unsigned long)(x)),*/ ({                                                                                                                                      \
    void* ptr = (x > 50000 ? memory : malloc(x));                                                                                                                                                      \
    /*printf(" %p\n", (void*)ptr);    */                                                                                                                                                               \
    ptr;                                                                                                                                                                                               \
  }))

#define STBTT_free(x, u) (/*printf("Freeing %p\n", (void*)(x)), */ (x != memory ? free(x) : 0))
#endif

#define STB_TRUETYPE_IMPLEMENTATION // force following include to generate implementation
#include "stb_truetype.h"

extern const char interdisplay_medium_webfont_ttf[];
extern const int interdisplay_medium_webfont_ttf_len;

unsigned char* ttf_buffer = interdisplay_medium_webfont_ttf;

int grid_font_init(struct grid_font_model* font) {
  printf("Ubuntu font size: %d\n", interdisplay_medium_webfont_ttf_len);
  font->font_handle = malloc(sizeof(stbtt_fontinfo));

  stbtt_InitFont(font->font_handle, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer, 0));
  printf("stbtt_InitFont\n");

  return 0;
}

int grid_font_draw_string(struct grid_font_model* font, struct grid_gui_model* gui, uint16_t x, uint16_t y, int size, char* string, int* cursor_jump, grid_color_t color) {

  if (font == NULL) {
    return 1;
  }

  if (gui == NULL) {
    return 1;
  }

  int cursor = 0;

  for (int i = 0; string[i] != 0; i++) {
    grid_font_draw_character(font, gui, x + cursor, y, size, string[i], &cursor, color);
  }

  *cursor_jump = cursor;
  return 0;
}

int grid_font_draw_character(struct grid_font_model* font, struct grid_gui_model* gui, uint16_t x, uint16_t y, int size, int character, int* cursor_jump, grid_color_t color) {

  if (font == NULL) {
    return 1;
  }

  if (gui == NULL) {
    return 1;
  }

  int spaceing = size / 20;
  int w, h;
  int xoff, yoff;
  unsigned char* bitmap;
  bitmap = stbtt_GetCodepointBitmap(font->font_handle, 0, stbtt_ScaleForPixelHeight(font->font_handle, size), character, &w, &h, &xoff, &yoff);

  for (int j = 0; j < h; ++j) {
    for (int i = 0; i < w; ++i) {
      // putchar(" .:ioVM@"[bitmap[j*w+i]>>5]);

      // grid_gui_draw_pixel(gui, x + i + xoff, y + j + yoff+size/2, grid_gui_color_apply_alpha(color, bitmap[j * w + i]));

      grid_gui_draw_pixel(gui, x + i + xoff, y + j + yoff + size / 2, grid_gui_color_from_rgba(255, 0, 0, bitmap[j * w + i]));
    }

    // putchar('\n');
  }

  *cursor_jump += w + spaceing;
  stbtt_FreeBitmap(bitmap, NULL);

  return 0;
}
