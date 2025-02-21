#include "grid_gui.h"
#include "grid_font.h"
#include <stdio.h>

struct grid_gui_model grid_gui_states[2] = {0};

bool grid_gui_index_active(size_t index) {

  if (index >= 2) {
    return false;
  }

  return grid_gui_active(&grid_gui_states[index]);
}

grid_color_t grid_gui_color_from_rgb(uint8_t r, uint8_t g, uint8_t b) { return (r << 24) | (g << 16) | (b << 8) | 255; }
grid_color_t grid_gui_color_from_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { return (r << 24) | (g << 16) | (b << 8) | a; }

grid_color_t grid_gui_color_apply_alpha(grid_color_t color, uint8_t alpha) {

  grid_color_t old_alpha = color & 0xff;
  grid_color_t col_rgb = color ^ old_alpha;
  return col_rgb | ((alpha * old_alpha) / 255);
}

int grid_gui_init(struct grid_gui_model* gui, void* screen_handle, uint8_t* buffer, uint32_t size, uint32_t width, uint32_t height) {

  assert(size);

  if (size != width * height * GRID_GUI_BYTES_PPX) {
    return 1;
  }

  gui->screen_handle = screen_handle;
  gui->buffer = buffer;
  gui->size = size;
  gui->width = width;
  gui->height = height;
  gui->delta = 0;
  gui->swap = 0;

  assert(grid_swsr_malloc(&gui->swsr, 1024) == 0);

  return 0;
}

bool grid_gui_active(struct grid_gui_model* gui) { return gui->size != 0; }

int grid_gui_clear(struct grid_gui_model* gui, grid_color_t color) {

  for (uint16_t i = 0; i < gui->height; ++i) {
    grid_gui_draw_horizontal_line(gui, 0, i, gui->width - 1, color);
  }

  return 0;
}

int grid_gui_queue_push(struct grid_gui_model* gui, grid_gui_draw_handler_t handler, size_t size) {

  // The destination must be able to receive the entire packet
  if (!grid_swsr_writable(&gui->swsr, size)) {
    return 1;
  }

  // Address of packet handler function
  grid_swsr_write(&gui->swsr, &handler, sizeof(grid_gui_draw_handler_t));

  // Packet size
  grid_swsr_write(&gui->swsr, &size, sizeof(size_t));

  return 0;
}

int grid_gui_queue_step(struct grid_gui_model* gui) {

  // To process a packet, at least the header must be readable
  if (!grid_swsr_readable(&gui->swsr, GRID_GUI_CALL_HEADER_SIZE)) {
    return 0;
  }

  // Address of packet handler function
  grid_gui_draw_handler_t handler = NULL;
  assert(grid_swsr_readable(&gui->swsr, sizeof(grid_gui_draw_handler_t)));
  grid_swsr_read(&gui->swsr, &handler, sizeof(grid_gui_draw_handler_t));
  assert(handler);

  // Packet size
  size_t size = 0;
  assert(grid_swsr_readable(&gui->swsr, sizeof(size_t)));
  grid_swsr_read(&gui->swsr, &size, sizeof(size_t));
  assert(size >= GRID_GUI_CALL_HEADER_SIZE);

  // After reading the header, the bytes of the body should become readable
  size_t body = size - GRID_GUI_CALL_HEADER_SIZE;
  while (!grid_swsr_readable(&gui->swsr, body)) {
  }

  handler(gui, &gui->swsr);

  return 0;
}

void grid_gui_swap_set(struct grid_gui_model* gui, bool x) { gui->swap = x; }

bool grid_gui_swap_get(struct grid_gui_model* gui) { return gui->swap; }

int grid_gui_draw_pixel(struct grid_gui_model* gui, uint16_t x, uint16_t y, grid_color_t color) {

  if (x >= gui->width || y >= gui->height) {
    return 1;
  }

  struct grid_rgba_t c = grid_unpack_rgba(color);

  uint8_t* pixel = gui->buffer + (gui->height * x + y) * GRID_GUI_BYTES_PPX;

  uint8_t inv_alpha = 255 - c.a;
  pixel[0] = (c.r * c.a + pixel[0] * inv_alpha) / 255;
  pixel[1] = (c.g * c.a + pixel[1] * inv_alpha) / 255;
  pixel[2] = (c.b * c.a + pixel[2] * inv_alpha) / 255;

  return 0;
}

int grid_gui_draw_array(struct grid_gui_model* gui, uint16_t x, uint16_t y, uint16_t ys, grid_color_t* colors) {

  if (x >= gui->width || y >= gui->height) {
    return 1;
  }

  uint32_t ymin = y;
  uint32_t ymax = y + ys < gui->height ? y + ys : gui->height;
  uint32_t ylen = ymax - ymin;

  uint8_t* pixels = gui->buffer + (gui->height * x + y) * GRID_GUI_BYTES_PPX;

  for (uint32_t i = 0; i < ylen; ++i) {

    struct grid_rgba_t c = grid_unpack_rgba(colors[i]);
    uint8_t* pixel = &pixels[i * GRID_GUI_BYTES_PPX];

    uint8_t inv_alpha = 255 - c.a;
    pixel[0] = (c.r * c.a + pixel[0] * inv_alpha) / 255;
    pixel[1] = (c.g * c.a + pixel[1] * inv_alpha) / 255;
    pixel[2] = (c.b * c.a + pixel[2] * inv_alpha) / 255;
  }

  return 0;
}

int grid_gui_draw_matrix(struct grid_gui_model* gui, uint16_t x, uint16_t y, uint16_t xs, uint16_t ys, grid_color_t* colors) {

  for (uint16_t i = 0; i < xs; ++i) {
    grid_gui_draw_array(gui, x + i, y, ys, &colors[i * ys]);
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
  uint16_t min_y = UINT16_MAX;
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

int grid_gui_draw_rectangle_rounded(struct grid_gui_model* gui, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t radius, grid_color_t color) {

  // Ensure x1 < x2 and y1 < y2
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

  // Calculate width and height
  uint16_t width = x2 - x1;
  uint16_t height = y2 - y1;

  // Limit the radius to half the width or height
  if (radius > width / 2) {
    radius = width / 2;
  }
  if (radius > height / 2) {
    radius = height / 2;
  }

  // Draw the four corners using quarter circles
  for (int i = 0; i <= radius; i++) {
    int j = (int)(sqrt(radius * radius - i * i) + 0.5);

    // Top-left corner
    grid_gui_draw_pixel(gui, x1 + radius - i, y1 + radius - j, color);
    grid_gui_draw_pixel(gui, x1 + radius - j, y1 + radius - i, color);

    // Top-right corner
    grid_gui_draw_pixel(gui, x2 - radius + i, y1 + radius - j, color);
    grid_gui_draw_pixel(gui, x2 - radius + j, y1 + radius - i, color);

    // Bottom-left corner
    grid_gui_draw_pixel(gui, x1 + radius - i, y2 - radius + j, color);
    grid_gui_draw_pixel(gui, x1 + radius - j, y2 - radius + i, color);

    // Bottom-right corner
    grid_gui_draw_pixel(gui, x2 - radius + i, y2 - radius + j, color);
    grid_gui_draw_pixel(gui, x2 - radius + j, y2 - radius + i, color);
  }

  // Draw the four edges (excluding the corners)
  for (int i = x1 + radius; i <= x2 - radius; i++) {
    grid_gui_draw_pixel(gui, i, y1, color);
    grid_gui_draw_pixel(gui, i, y2, color);
  }

  for (int j = y1 + radius; j <= y2 - radius; j++) {
    grid_gui_draw_pixel(gui, x1, j, color);
    grid_gui_draw_pixel(gui, x2, j, color);
  }

  return 0;
}

int grid_gui_draw_rectangle_rounded_filled(struct grid_gui_model* gui, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t radius, grid_color_t color) {

  // Ensure x1 < x2 and y1 < y2
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

  // Calculate width and height
  uint16_t width = x2 - x1;
  uint16_t height = y2 - y1;

  // Limit the radius to half the width or height
  if (radius > width / 2) {
    radius = width / 2;
  }
  if (radius > height / 2) {
    radius = height / 2;
  }

  // Draw the filled rectangle (excluding corners)
  for (int y = y1; y <= y2; y++) {
    for (int x = x1; x <= x2; x++) {
      // Check if the current pixel is inside the rounded corners
      if (x - x1 < radius && y - y1 < radius && (x - x1 - radius) * (x - x1 - radius) + (y - y1 - radius) * (y - y1 - radius) > radius * radius) {
        continue;
      } else if (x - x2 > -radius && y - y1 < radius && (x - x2 + radius) * (x - x2 + radius) + (y - y1 - radius) * (y - y1 - radius) > radius * radius) {
        continue;
      } else if (x - x1 < radius && y - y2 > -radius && (x - x1 - radius) * (x - x1 - radius) + (y - y2 + radius) * (y - y2 + radius) > radius * radius) {
        continue;
      } else if (x - x2 > -radius && y - y2 > -radius && (x - x2 + radius) * (x - x2 + radius) + (y - y2 + radius) * (y - y2 + radius) > radius * radius) {
        continue;
      }
      grid_gui_draw_pixel(gui, x, y, color);
    }
  }

  // // Draw the rounded corners
  // for (int i = 0; i <= radius; i++) {
  //     int j = (int)(sqrt(radius * radius - i * i) + 0.5);

  //     // Top-left corner
  //     for (int yi = y1; yi <= y1 + radius - i; yi++) {
  //         for (int xi = x1; xi <= x1 + radius - j; xi++) {
  //             grid_gui_draw_pixel(gui, xi, yi, color);
  //         }
  //     }

  //     // Top-right corner
  //     for (int yi = y1; yi <= y1 + radius - i; yi++) {
  //         for (int xi = x2 - radius + j; xi <= x2; xi++) {
  //             grid_gui_draw_pixel(gui, xi, yi, color);
  //         }
  //     }

  //     // Bottom-left corner
  //     for (int yi = y2 - radius + j; yi <= y2; yi++) {
  //         for (int xi = x1; xi <= x1 + radius - j; xi++) {
  //             grid_gui_draw_pixel(gui, xi, yi, color);
  //         }
  //     }

  //     // Bottom-right corner
  //     for (int yi = y2 - radius + j; yi <= y2; yi++) {
  //         for (int xi = x2 - radius + j; xi <= x2; xi++) {
  //             grid_gui_draw_pixel(gui, xi, yi, color);
  //         }
  //     }
  // }

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

  for (int j = y1; j <= y2; j++) {
    for (int i = x1; i <= x2; i++) {
      grid_gui_draw_pixel(gui, i, j, color);
    }
  }
  return 0;
}

void grid_gui_draw_demo(struct grid_gui_model* gui, uint8_t counter) {

  for (uint32_t y = 0; y < gui->height; ++y) {
    for (uint32_t x = 0; x < gui->width; ++x) {

      grid_color_t col = ((((uint8_t)(counter * 8 + y * 2)) << 24) | (((uint8_t)(counter * 8 + x * 2)) << 16) | ((0xff) << 0));
      grid_gui_draw_pixel(gui, x, y, col);
    }
  }

  grid_color_t white = grid_gui_color_from_rgb(255, 255, 255);

  grid_gui_draw_line(gui, 160, 120, 40, 40, white);
  grid_gui_draw_rectangle(gui, 10, 10, 10 + 30, 10 + 30, white);

  grid_gui_draw_line(gui, 160, 120, 280, 200, white);
  grid_gui_draw_rectangle_filled(gui, 280, 200, 280 + 30, 200 + 30, white);

  grid_gui_draw_horizontal_line(gui, 160 - 30, 10, 160 + 30, white);
  grid_gui_draw_horizontal_line(gui, 160 - 30, 230, 160 + 30, white);

  grid_gui_draw_rectangle_rounded(gui, 280, 10, 280 + 30, 10 + 30, 10, white);
  grid_gui_draw_rectangle_rounded_filled(gui, 10, 200, 10 + 30, 200 + 30, 10, white);

  uint16_t x_pts[4] = {160 - 45, 160, 160 + 45, 160};
  uint16_t y_pts[4] = {120, 120 + 45, 120, 120 - 45};

  grid_gui_draw_polygon(gui, x_pts, y_pts, 4, white);

  x_pts[0] += 15;
  x_pts[2] -= 15;
  y_pts[1] -= 15;
  y_pts[3] += 15;

  grid_color_t white_half = grid_gui_color_from_rgba(255, 255, 255, 127);
  grid_gui_draw_polygon_filled(gui, x_pts, y_pts, 4, white_half);

  if (grid_font_state.initialized) {

    int cursor = 0;
    grid_font_draw_string(&grid_font_state, gui, 220, 105, 60, "vsnx", &cursor, white);
  }

  uint16_t x = 10;
  uint16_t y = 105;
  int cursor = 0;

  char temp[4] = {0};
  sprintf(temp, "%3hhu", counter);

  struct grid_font_model* font = &grid_font_state;
  grid_font_draw_character(font, gui, x + cursor, y, 60, temp[0], &cursor, white);
  grid_font_draw_character(font, gui, x + cursor, y, 60, temp[1], &cursor, white);
  grid_font_draw_character(font, gui, x + cursor, y, 60, temp[2], &cursor, white);

  // grid_font_draw_string_fast(gui, 0, 0, "hello", white);
}

void grid_gui_draw_demo_matrix(struct grid_gui_model* gui, uint8_t counter, grid_color_t* matrix) {

  for (uint32_t y = 0; y < gui->height; ++y) {
    for (uint32_t x = 0; x < gui->width; ++x) {

      grid_color_t col = (((counter * 8 + y) << 24) | ((counter * 8 + x) << 16) | ((0xff) << 0));
      matrix[y * gui->width + x] = col;
    }
  }

  grid_gui_draw_matrix(gui, 0, 0, 320, 240, matrix);
}

void grid_gui_draw_demo_rgb(struct grid_gui_model* gui, uint8_t counter) {

  grid_color_t colors[3] = {0xff0000ff, 0x00ff00ff, 0x0000ffff};

  for (uint32_t x = 0; x < gui->width; ++x) {
    for (uint32_t y = 0; y < gui->height; ++y) {

      grid_gui_draw_pixel(gui, x, y, colors[counter % 3]);
    }
  }
}
