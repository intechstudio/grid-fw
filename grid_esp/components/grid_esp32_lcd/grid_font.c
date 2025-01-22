#include "grid_font.h"
#include "grid_gui.h"

struct grid_font_model grid_font_state = {0};

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

extern const char generated_fonts_interdisplay_regular_ttf[];
extern const int generated_fonts_interdisplay_regular_ttf_len;

extern const char generated_fonts_spacemono_regular_ttf[];
extern const int generated_fonts_spacemono_regular_ttf_len;

extern const char generated_fonts_spacemono_bold_ttf[];
extern const int generated_fonts_spacemono_bold_ttf_len;

struct grid_font_table {
  char name[30];
  unsigned char* data;
  int* size
};

struct grid_font_table font_list[] = {{.name = "interdisplay_regular", .data = (unsigned char*)generated_fonts_interdisplay_regular_ttf, .size = &generated_fonts_interdisplay_regular_ttf_len},
                                      {.name = "spacemono_regular", .data = (unsigned char*)generated_fonts_spacemono_regular_ttf, .size = &generated_fonts_spacemono_regular_ttf_len},
                                      {.name = "spacemono_bold", .data = (unsigned char*)generated_fonts_spacemono_bold_ttf, .size = &generated_fonts_spacemono_bold_ttf_len}};

int grid_font_init(struct grid_font_model* font) {

  struct grid_font_table* selected_font = &font_list[1];

  printf("%s font size: %d\n", selected_font->name, *selected_font->size);
  font->font_handle = malloc(sizeof(stbtt_fontinfo));

  stbtt_InitFont(font->font_handle, selected_font->data, stbtt_GetFontOffsetForIndex(selected_font->data, 0));
  printf("stbtt_InitFont\n");

  font->initialized = 1;
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

      grid_gui_draw_pixel(gui, x + i + xoff, y + j + yoff + size / 2, grid_gui_color_apply_alpha(color, bitmap[j * w + i]));
    }

    // putchar('\n');
  }

  *cursor_jump += w + spaceing;
  stbtt_FreeBitmap(bitmap, NULL);

  return 0;
}

// ThreeByteType is needed because internal framebuffer is 3 bytes per pixel
typedef struct __attribute__((packed)) {
  uint8_t byte1;
  uint8_t byte2;
  uint8_t byte3;
} ThreeByteType;

ThreeByteType BG_COLOR = {0, 0, 0};

#define blit32_MACRO_INLINE
#define blit_pixel ThreeByteType
#define blit_background BG_COLOR
#include "blit32.h"

int grid_font_draw_string_fast(struct grid_gui_model* gui, uint16_t x, uint16_t y, char* str, grid_color_t color) {

  unsigned char* buffer = gui->framebuffer;

  uint8_t r = grid_gui_color_to_red(color);
  uint8_t g = grid_gui_color_to_green(color);
  uint8_t b = grid_gui_color_to_blue(color);

  ThreeByteType three_byte_color = {r, g, b};
  int ret = blit32_TextExplicit(buffer, three_byte_color, 4, 320, 240, 1, x, y, str);

  return 0;
}
