
#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t grid_color_t;

struct grid_gui_model {

  void* screen_handle;
  uint8_t* framebuffer;
  uint32_t framebuffer_size;
  uint8_t bits_per_pixel;
  uint16_t width;
  uint16_t height;
};

extern struct grid_gui_model grid_gui_state;

uint8_t grid_gui_color_to_red(grid_color_t color);
uint8_t grid_gui_color_to_green(grid_color_t color);
uint8_t grid_gui_color_to_blue(grid_color_t color);
uint8_t grid_gui_color_to_alpha(grid_color_t color);
grid_color_t grid_gui_color_from_rgb(uint8_t r, uint8_t g, uint8_t b);
grid_color_t grid_gui_color_from_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
grid_color_t grid_gui_color_apply_alpha(grid_color_t color, uint8_t alpha);

int grid_gui_init(struct grid_gui_model* gui, void* screen_handle, uint8_t* framebuffer, uint32_t framebuffer_size, uint8_t bits_per_pixel, uint16_t width, uint16_t height);

int grid_gui_draw_pixel(struct grid_gui_model* gui, uint16_t x, uint16_t y, grid_color_t color);

int grid_gui_draw_rectangle(struct grid_gui_model* gui, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, grid_color_t color);

int grid_gui_draw_rectangle_filled(struct grid_gui_model* gui, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, grid_color_t color);

void grid_gui_draw_demo(struct grid_gui_model* gui, uint8_t loopcounter);

#ifdef __cplusplus
}
#endif
