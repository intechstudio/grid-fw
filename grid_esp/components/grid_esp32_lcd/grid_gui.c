#include "grid_gui.h"
#include "grid_font.h"
#include <stdio.h>
struct grid_gui_model grid_gui_state;

uint8_t grid_gui_color_to_red(grid_color_t color) { return (color >> 24) & 0xFF; }
uint8_t grid_gui_color_to_green(grid_color_t color) { return (color >> 16) & 0xFF; }
uint8_t grid_gui_color_to_blue(grid_color_t color) { return (color >> 8) & 0xFF; }
uint8_t grid_gui_color_to_alpha(grid_color_t color) { return (color) & 0xFF; }
grid_color_t grid_gui_color_from_rgb(uint8_t r, uint8_t g, uint8_t b) { return (r << 24) | (g << 16) | (b << 8) | 255; }
grid_color_t grid_gui_color_from_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { return (r << 24) | (g << 16) | (b << 8) | a; }

grid_color_t grid_gui_color_apply_alpha(grid_color_t color, uint8_t alpha) {

  return grid_gui_color_from_rgba(grid_gui_color_to_red(color), grid_gui_color_to_green(color), grid_gui_color_to_blue(color), grid_gui_color_to_alpha(color) * alpha / 255);
}

int grid_gui_init(struct grid_gui_model* gui, void* screen_handle, uint8_t* framebuffer, uint32_t framebuffer_size, uint8_t bits_per_pixel, uint16_t width, uint16_t height) {
  grid_gui_state.screen_handle = screen_handle;
  grid_gui_state.framebuffer = framebuffer;
  grid_gui_state.framebuffer_size = framebuffer_size;
  grid_gui_state.bits_per_pixel = bits_per_pixel;
  grid_gui_state.width = width;
  grid_gui_state.height = height;
  return 0;
}

int grid_gui_draw_pixel(struct grid_gui_model* gui, uint16_t x, uint16_t y, grid_color_t color) {

  uint8_t alpha = grid_gui_color_to_alpha(color);
  uint8_t r = grid_gui_color_to_red(color);
  uint8_t g = grid_gui_color_to_green(color);
  uint8_t b = grid_gui_color_to_blue(color);

  if (x >= gui->width || y >= gui->height) {

    return 1; // out of bounds
  }

  if (gui->bits_per_pixel == 24) {

    uint8_t* pixel = gui->framebuffer + ((gui->width * y + x) * 3);

    uint8_t r_old = pixel[0];
    uint8_t g_old = pixel[1];
    uint8_t b_old = pixel[2];

    uint8_t r_new = r * alpha / 255 + r_old * (255 - alpha) / 255;
    uint8_t g_new = g * alpha / 255 + g_old * (255 - alpha) / 255;
    uint8_t b_new = b * alpha / 255 + b_old * (255 - alpha) / 255;

    pixel[0] = r_new;
    pixel[1] = g_new;
    pixel[2] = b_new;

  } else if (gui->bits_per_pixel == 6) {

    uint8_t* pixel = gui->framebuffer + ((gui->width * y + x) * 1);

    if (alpha == 0) {
      return 0;
    }

    if (alpha == 255) {
      pixel[0] = (r / 64) << 4 | (g / 64) << 2 | (b / 64);
      return 0;
    }

    uint8_t r_old = ((pixel[0] >> 4) & 0b11) * 85;
    uint8_t g_old = ((pixel[0] >> 2) & 0b11) * 85;
    uint8_t b_old = ((pixel[0] >> 0) & 0b11) * 85;

    uint8_t r_new = r * alpha / 255 + r_old * (255 - alpha) / 255;
    uint8_t g_new = g * alpha / 255 + g_old * (255 - alpha) / 255;
    uint8_t b_new = b * alpha / 255 + b_old * (255 - alpha) / 255;

    pixel[0] = (r_new / 64) << 4 | (g_new / 64) << 2 | (b_new / 64);
  }

  return 0;
}

int grid_gui_draw_rectangle_filled(struct grid_gui_model* gui, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, grid_color_t color) {

  if (x1 > x2) {
    uint16_t tmp = x1;
    x1 = x2;
    x2 = tmp;
  }

  if (y1 > y2) {
    uint16_t tmp = y1;
    y1 = y2;
    y2 = tmp;
  }

  for (int i = x1; i <= x2; i++) {
    for (int j = y1; j <= y2; j++) {
      grid_gui_draw_pixel(gui, i, j, color);
    }
  }

  return 0;
}

int grid_gui_draw_rectangle(struct grid_gui_model* gui, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, grid_color_t color) {

  if (x1 > x2) {
    uint16_t tmp = x1;
    x1 = x2;
    x2 = tmp;
  }

  if (y1 > y2) {
    uint16_t tmp = y1;
    y1 = y2;
    y2 = tmp;
  }

  for (int i = x1; i <= x2; i++) {

    grid_gui_draw_pixel(gui, i, y1, color);
    grid_gui_draw_pixel(gui, i, y2, color);
  }

  for (int j = y1; j <= y2; j++) {
    grid_gui_draw_pixel(gui, x1, j, color);
    grid_gui_draw_pixel(gui, x2, j, color);
  }

  return 0;
}

void grid_gui_draw_demo(struct grid_gui_model* gui, uint8_t loopcounter) {

  for (int i = 0; i < 320; i++) {

    for (int j = 0; j < 240; j++) {

      grid_gui_draw_pixel(gui, i, j, grid_gui_color_from_rgb(loopcounter, 0, 0));
    }
  }

  grid_gui_draw_rectangle(&grid_gui_state, 10, 10, 40, 40, grid_gui_color_from_rgb(0, 0, 255));
  grid_gui_draw_rectangle_filled(&grid_gui_state, 100, 100, 140, 140, grid_gui_color_from_rgb(0, 255, 0));

  grid_gui_draw_rectangle_filled(&grid_gui_state, 120, 120, 140, 140, grid_gui_color_from_rgba(255, 0, 0, 70));

  uint16_t x = 10;
  uint16_t y = 100;
  int cursor = 0;

  char temp[10] = {0};
  sprintf(temp, "%d", loopcounter);

  grid_font_draw_string(&grid_font_state, gui, 0, 1, 60, "hello", &cursor, grid_gui_color_from_rgb(0, 0, 255));

  if (loopcounter > 30) {

    int cursor = 0;
    grid_font_draw_character(&grid_font_state, gui, x + cursor, y, 60, '$', &cursor, grid_gui_color_from_rgba(255, 0, 0, 255));
    grid_font_draw_character(&grid_font_state, gui, x + cursor, y, 60, temp[0], &cursor, grid_gui_color_from_rgb(255, 0, 0));
    grid_font_draw_character(&grid_font_state, gui, x + cursor, y, 60, temp[1], &cursor, grid_gui_color_from_rgb(255, 0, 0));
    grid_font_draw_character(&grid_font_state, gui, x + cursor, y, 60, temp[2], &cursor, grid_gui_color_from_rgb(255, 0, 0));
    grid_font_draw_character(&grid_font_state, gui, x + cursor, y, 60, temp[3], &cursor, grid_gui_color_from_rgb(255, 0, 0));
    grid_font_draw_character(&grid_font_state, gui, x + cursor, y, 60, temp[4], &cursor, grid_gui_color_from_rgb(255, 0, 0));
  }
}
