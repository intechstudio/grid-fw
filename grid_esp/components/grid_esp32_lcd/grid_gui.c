#include "grid_gui.h"
#include "grid_font.h"
#include <stdio.h>
struct grid_gui_model grid_gui_state = {0};

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
  gui->screen_handle = screen_handle;
  gui->framebuffer = framebuffer;
  gui->framebuffer_size = framebuffer_size;
  gui->bits_per_pixel = bits_per_pixel;
  gui->width = width;
  gui->height = height;
  gui->framebuffer_changed_flag = 0;
  return 0;
}

int grid_gui_draw_pixel(struct grid_gui_model* gui, uint16_t x, uint16_t y, grid_color_t color) {

  gui->framebuffer_changed_flag = 1;

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
  } else if (gui->bits_per_pixel == 1) {

    uint8_t* pixel = gui->framebuffer + ((gui->width * y + x) * 1) / 8;
    uint8_t offset = ((gui->width * y + x) * 1) % 8;

    if (alpha < 128) {
      return 0;
    } else {
      uint8_t value = ((r / 3 + g / 3 + b / 3) > 40);
      pixel[0] = (pixel[0] & ~(1 << offset)) | (value << offset);
      return 0;
    }
  }

  return 0;
}

int grid_gui_draw_line(struct grid_gui_model* gui, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, grid_color_t color) {

  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;

  while (1) {
    grid_gui_draw_pixel(gui, x0, y0, color); // Draw the pixel

    if (x0 == x1 && y0 == y1)
      break;
    int e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y0 += sy;
    }
  }

  return 0;
}

int grid_gui_draw_polygon(struct grid_gui_model* gui, uint16_t* x_points, uint16_t* y_points, size_t num_points, grid_color_t color) {
  if (num_points < 2)
    return 1; // Need at least 2 points to draw a line

  for (size_t i = 0; i < num_points - 1; i++) {
    grid_gui_draw_line(gui, x_points[i], y_points[i], x_points[i + 1], y_points[i + 1], color);
  }

  // Draw line from the last point to the first point to close the polygon
  grid_gui_draw_line(gui, x_points[num_points - 1], y_points[num_points - 1], x_points[0], y_points[0], color);

  return 0;
}

int grid_gui_draw_horizontal_line(struct grid_gui_model* gui, uint16_t x0, uint16_t y, uint16_t x1, grid_color_t color) {
  if (x0 > x1) {
    uint16_t temp = x0;
    x0 = x1;
    x1 = temp;
  }
  for (uint16_t x = x0; x <= x1; ++x) {
    grid_gui_draw_pixel(gui, x, y, color);
  }

  return 0;
}

#include <limits.h>

struct edge_desc {
  uint16_t y_max;
  float x_min;
  float inv_slope;
  struct edge_desc* next;
};

int grid_gui_draw_polygon_filled(struct grid_gui_model* gui, uint16_t* x_points, uint16_t* y_points, size_t num_points, grid_color_t color) {

  if (num_points < 3)
    return 1; // Need at least 3 points to form a polygon

  // Find the bounding box of the polygon
  uint16_t min_y = USHRT_MAX;
  uint16_t max_y = 0;
  for (size_t i = 0; i < num_points; ++i) {
    if (y_points[i] < min_y)
      min_y = y_points[i];
    if (y_points[i] > max_y)
      max_y = y_points[i];
  }

  // Initialize the edge table
  struct edge_desc* edge_table[max_y + 1];
  for (uint16_t i = 0; i <= max_y; ++i) {
    edge_table[i] = NULL;
  }

  // Populate the edge table
  for (size_t i = 0; i < num_points; ++i) {
    size_t j = (i + 1) % num_points;
    uint16_t x0 = x_points[i], y0 = y_points[i];
    uint16_t x1 = x_points[j], y1 = y_points[j];

    if (y0 == y1)
      continue; // Ignore horizontal edges

    if (y0 > y1) {
      uint16_t temp_x = x0, temp_y = y0;
      x0 = x1;
      y0 = y1;
      x1 = temp_x;
      y1 = temp_y;
    }

    struct edge_desc* edge = (struct edge_desc*)malloc(sizeof(struct edge_desc));
    edge->y_max = y1;
    edge->x_min = (float)x0;
    edge->inv_slope = (float)(x1 - x0) / (y1 - y0);
    edge->next = edge_table[y0];
    edge_table[y0] = edge;
  }

  // Initialize the active edge list
  struct edge_desc* active_edge_list = NULL;

  // Scanline fill algorithm
  for (uint16_t y = min_y; y <= max_y; ++y) {
    // Add edges from the edge table to the active edge list
    struct edge_desc* edge = edge_table[y];
    while (edge) {
      struct edge_desc* next_edge = edge->next;
      edge->next = active_edge_list;
      active_edge_list = edge;
      edge = next_edge;
    }

    // Remove edges from the active edge list where y = y_max
    struct edge_desc** current_edge = &active_edge_list;
    while (*current_edge) {
      if ((*current_edge)->y_max <= y) {
        struct edge_desc* edge_to_remove = *current_edge;
        *current_edge = edge_to_remove->next;
        free(edge_to_remove);
      } else {
        current_edge = &((*current_edge)->next);
      }
    }

    // Sort the active edge list by x_min
    for (struct edge_desc* edge_i = active_edge_list; edge_i != NULL; edge_i = edge_i->next) {
      for (struct edge_desc* edge_j = edge_i->next; edge_j != NULL; edge_j = edge_j->next) {
        if (edge_i->x_min > edge_j->x_min) {
          float temp_x_min = edge_i->x_min;
          float temp_inv_slope = edge_i->inv_slope;
          uint16_t temp_y_max = edge_i->y_max;

          edge_i->x_min = edge_j->x_min;
          edge_i->inv_slope = edge_j->inv_slope;
          edge_i->y_max = edge_j->y_max;

          edge_j->x_min = temp_x_min;
          edge_j->inv_slope = temp_inv_slope;
          edge_j->y_max = temp_y_max;
        }
      }
    }

    // Draw horizontal lines between pairs of intersections
    for (struct edge_desc* edge_i = active_edge_list; edge_i && edge_i->next; edge_i = edge_i->next->next) {
      if (edge_i->next) {
        grid_gui_draw_horizontal_line(gui, (uint16_t)edge_i->x_min, y, (uint16_t)edge_i->next->x_min, color);
      }
    }

    // Update x_min for the next scanline
    for (struct edge_desc* edge = active_edge_list; edge; edge = edge->next) {
      edge->x_min += edge->inv_slope;
    }
  }

  // Free remaining edges in the active edge list
  while (active_edge_list) {
    struct edge_desc* edge = active_edge_list;
    active_edge_list = edge->next;
    free(edge);
  }

  return 0;
}

int grid_gui_draw_polygon_filled3(struct grid_gui_model* gui, uint16_t* x_points, uint16_t* y_points, size_t num_points, grid_color_t color) {
  if (num_points < 3)
    return 1; // Need at least 3 points to form a polygon

  // Find the bounding box of the polygon
  uint16_t min_y = USHRT_MAX;
  uint16_t max_y = 0;
  for (size_t i = 0; i < num_points; ++i) {
    if (y_points[i] < min_y)
      min_y = y_points[i];
    if (y_points[i] > max_y)
      max_y = y_points[i];
  }

  // Initialize the edge table
  struct edge_desc* edge_table[max_y + 1];
  for (uint16_t i = 0; i <= max_y; ++i) {
    edge_table[i] = NULL;
  }

  // Populate the edge table
  for (size_t i = 0; i < num_points; ++i) {
    size_t j = (i + 1) % num_points;
    uint16_t x0 = x_points[i], y0 = y_points[i];
    uint16_t x1 = x_points[j], y1 = y_points[j];

    if (y0 == y1)
      continue; // Ignore horizontal edges

    if (y0 > y1) {
      uint16_t temp_x = x0, temp_y = y0;
      x0 = x1;
      y0 = y1;
      x1 = temp_x;
      y1 = temp_y;
    }

    struct edge_desc* edge = (struct edge_desc*)malloc(sizeof(struct edge_desc));
    edge->y_max = y1;
    edge->x_min = x0;
    edge->inv_slope = (float)(x1 - x0) / (y1 - y0);

    edge->next = edge_table[y0];
    edge_table[y0] = edge;
  }

  // Initialize the active edge list
  struct edge_desc* active_edge_list = NULL;

  // Scanline fill algorithm
  for (uint16_t y = min_y; y <= max_y; ++y) {
    // Add edges from the edge table to the active edge list
    struct edge_desc* edge = edge_table[y];
    while (edge) {
      struct edge_desc* next_edge = edge->next;
      edge->next = active_edge_list;
      active_edge_list = edge;
      edge = next_edge;
    }

    // Remove edges from the active edge list where y = y_max
    struct edge_desc** current_edge = &active_edge_list;
    while (*current_edge) {
      if ((*current_edge)->y_max == y) {
        struct edge_desc* edge_to_remove = *current_edge;
        *current_edge = edge_to_remove->next;
        free(edge_to_remove);
      } else {
        current_edge = &((*current_edge)->next);
      }
    }

    // Sort the active edge list by x_min
    for (struct edge_desc* edge_i = active_edge_list; edge_i != NULL; edge_i = edge_i->next) {
      for (struct edge_desc* edge_j = edge_i->next; edge_j != NULL; edge_j = edge_j->next) {
        if (edge_i->x_min > edge_j->x_min) {
          float temp_x_min = edge_i->x_min;
          float temp_inv_slope = edge_i->inv_slope;
          uint16_t temp_y_max = edge_i->y_max;

          edge_i->x_min = edge_j->x_min;
          edge_i->inv_slope = edge_j->inv_slope;
          edge_i->y_max = edge_j->y_max;

          edge_j->x_min = temp_x_min;
          edge_j->inv_slope = temp_inv_slope;
          edge_j->y_max = temp_y_max;
        }
      }
    }

    // Draw horizontal lines between pairs of intersections
    for (struct edge_desc* edge_i = active_edge_list; edge_i && edge_i->next; edge_i = edge_i->next->next) {
      if (edge_i->next) {
        grid_gui_draw_horizontal_line(gui, (uint16_t)edge_i->x_min, y, (uint16_t)edge_i->next->x_min, color);
      }
    }

    // Update x_min for the next scanline
    for (struct edge_desc* edge = active_edge_list; edge; edge = edge->next) {
      edge->x_min += edge->inv_slope;
    }
  }

  // Free remaining edges in the active edge list
  while (active_edge_list) {
    struct edge_desc* edge = active_edge_list;
    active_edge_list = edge->next;
    free(edge);
  }

  return 0;
}

int grid_gui_draw_polygon_filled2(struct grid_gui_model* gui, uint16_t* x_points, uint16_t* y_points, size_t num_points, grid_color_t color) {
  if (num_points < 3)
    return 1; // Need at least 3 points to form a polygon

  // Find the bounding box of the polygon
  uint16_t min_y = USHRT_MAX;
  uint16_t max_y = 0;
  for (size_t i = 0; i < num_points; ++i) {
    if (y_points[i] < min_y)
      min_y = y_points[i];
    if (y_points[i] > max_y)
      max_y = y_points[i];
  }

  printf("y %d %d\n", min_y, max_y);

  // Scanline fill algorithm
  for (uint16_t y = min_y; y <= max_y; ++y) {
    // Find intersections of the polygon with the scanline
    size_t num_intersections = 0;
    uint16_t intersections[num_points];

    for (size_t i = 0; i < num_points; ++i) {
      size_t j = (i + 1) % num_points;
      uint16_t x0 = x_points[i], y0 = y_points[i];
      uint16_t x1 = x_points[j], y1 = y_points[j];

      if (y0 == y1)
        continue; // Ignore horizontal edges

      if ((y >= y0 && y < y1) || (y >= y1 && y < y0)) {
        // Calculate intersection point
        uint16_t x = x0 + (y - y0) * (x1 - x0) / (y1 - y0);
        intersections[num_intersections++] = x;
      }
    }

    for (size_t i = 0; i < num_intersections; i++) {
      printf("x[%d] %d ", i, intersections[i]);
    }

    printf("\n");

    // // Sort intersections
    // for (size_t i = 0; i < num_intersections - 1; ++i) {
    //     for (size_t j = i + 1; j < num_intersections; ++j) {
    //         if (intersections[i] > intersections[j]) {
    //             uint16_t temp = intersections[i];
    //             intersections[i] = intersections[j];
    //             intersections[j] = temp;
    //         }
    //     }
    // }

    // // Draw horizontal lines between pairs of intersections
    // for (size_t i = 0; i < num_intersections; i += 2) {
    //     if (i + 1 < num_intersections) {
    //         grid_gui_draw_horizontal_line(gui, intersections[i], y, intersections[i + 1], color);
    //     }
    // }
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

void grid_gui_draw_clear(struct grid_gui_model* gui) {

  for (int i = 0; i < 320; i++) {

    for (int j = 0; j < 240; j++) {

      grid_gui_draw_pixel(gui, i, j, grid_gui_color_from_rgb(0, 0, 0));
    }
  }
}

void grid_gui_draw_demo(struct grid_gui_model* gui, uint8_t loopcounter) {

  for (int i = 0; i < 320; i++) {

    for (int j = 0; j < 240; j++) {

      grid_gui_draw_pixel(gui, i, j, grid_gui_color_from_rgb(loopcounter, 0, 0));
    }
  }

  grid_color_t color = grid_gui_color_from_rgb(0, 0, 255); // Example color (white)

  uint16_t x_points[] = {100, 200, 200, 100};
  uint16_t y_points[] = {100, 200, 100, 200};
  size_t num_points = sizeof(x_points) / sizeof(x_points[0]);

  grid_gui_draw_polygon_filled(gui, x_points, y_points, num_points, color);

  grid_gui_draw_rectangle(&grid_gui_state, 10, 10, 40, 40, grid_gui_color_from_rgb(0, 0, 255));
  grid_gui_draw_line(&grid_gui_state, 150, 100, 40, 40, grid_gui_color_from_rgb(0, 0, 255));

  grid_gui_draw_rectangle_filled(&grid_gui_state, 200, 200, 220, 220, grid_gui_color_from_rgb(0, 255, 0));

  grid_gui_draw_rectangle_filled(&grid_gui_state, 120, 120, 140, 140, grid_gui_color_from_rgba(255, 0, 0, 70));

  uint16_t x = 10;
  uint16_t y = 100;
  int cursor = 0;

  char temp[10] = {0};
  sprintf(temp, "%d", loopcounter);

  if (grid_font_state.initialized) {
    grid_font_draw_string(&grid_font_state, gui, 0, 1, 60, "hello", &cursor, grid_gui_color_from_rgb(0, 0, 255));
  }

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
