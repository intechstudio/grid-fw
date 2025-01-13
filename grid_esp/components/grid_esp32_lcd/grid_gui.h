
#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void grid_platform_printf(char const* fmt, ...);

#define GRID_GUI_BYTES_PPX 3

typedef uint32_t grid_color_t;

grid_color_t grid_gui_color_from_rgb(uint8_t r, uint8_t g, uint8_t b);
grid_color_t grid_gui_color_from_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
grid_color_t grid_gui_color_apply_alpha(grid_color_t color, uint8_t alpha);

struct grid_rgba_t {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
};

#define grid_unpack_rgba(color) (struct grid_rgba_t){ \
  .r = ((color) >> 24) & 0xff, \
  .g = ((color) >> 16) & 0xff, \
  .b = ((color) >>  8) & 0xff, \
  .a = ((color) >>  0) & 0xff, \
}

enum grid_gui_colmod_t {
  COLMOD_RGB888 = 0,
};

#define COLMOD_RGB888_BYTES 3


struct grid_gui_model {
  void* screen_handle;
  uint8_t* buffer;
  uint32_t size;
  uint32_t width;
  uint32_t height;
  uint8_t delta;
};

extern struct grid_gui_model grid_gui_state;

int grid_gui_pack_colmod(struct grid_gui_model* gui, uint32_t x, uint32_t y, uint32_t pixels, uint8_t* dest, enum grid_gui_colmod_t colmod);

int grid_gui_init(struct grid_gui_model* gui, void* screen_handle, uint8_t* buffer, uint32_t size, uint32_t width, uint32_t height);

int grid_gui_draw_pixel(struct grid_gui_model* gui, uint16_t x, uint16_t y, grid_color_t color);
int grid_gui_draw_array(struct grid_gui_model* gui, uint16_t x, uint16_t y, uint16_t xs, grid_color_t* colors);
int grid_gui_draw_matrix(struct grid_gui_model* gui, uint16_t x, uint16_t y, uint16_t xs, uint16_t ys, grid_color_t* colors);

int grid_gui_draw_line(struct grid_gui_model* gui, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, grid_color_t color);
int grid_gui_draw_horizontal_line(struct grid_gui_model* gui, uint16_t x0, uint16_t y, uint16_t x1, grid_color_t color);

int grid_gui_draw_polygon(struct grid_gui_model* gui, uint16_t* x_points, uint16_t* y_points, size_t num_points, grid_color_t color);
int grid_gui_draw_polygon_filled(struct grid_gui_model* gui, uint16_t* x_points, uint16_t* y_points, size_t num_points, grid_color_t color);

int grid_gui_draw_rectangle(struct grid_gui_model* gui, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, grid_color_t color);
int grid_gui_draw_rectangle_rounded(struct grid_gui_model* gui, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t radius, grid_color_t color);
int grid_gui_draw_rectangle_rounded_filled(struct grid_gui_model* gui, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t radius, grid_color_t color);

int grid_gui_draw_rectangle_filled(struct grid_gui_model* gui, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, grid_color_t color);

void grid_gui_draw_demo(struct grid_gui_model* gui, uint8_t counter);
void grid_gui_draw_demo_matrix(struct grid_gui_model* gui, uint8_t counter, grid_color_t* matrix);

#ifdef __cplusplus
}
#endif
