#include "grid_lua_api_gui.h"

#include <stdatomic.h>

#include "grid_platform.h"

// File-bytes buffers (grid_image_file_handle::data) live in PSRAM on target,
// since these can now be script-held and long-lived rather than transient -
// DRAM is the scarcer resource. The wasm simulator has no PSRAM/heap_caps
// concept (or LittleFS, guarded separately below), so it falls back to plain
// malloc/free - this macro pair exists only to keep every call site below
// platform-agnostic, mirroring grid_image.c's identical STBI_MALLOC pattern.
#ifndef __EMSCRIPTEN__
#include "esp_heap_caps.h"
#define GRID_IMAGE_FILE_MALLOC(sz) heap_caps_malloc(sz, MALLOC_CAP_SPIRAM)
#define GRID_IMAGE_FILE_FREE(p) heap_caps_free(p)
#else
#define GRID_IMAGE_FILE_MALLOC(sz) malloc(sz)
#define GRID_IMAGE_FILE_FREE(p) free(p)
#endif

/* grid_lua_api_gui.c

The API functions defined in this file are built according to a specific
approach, and new functions should also follow this approach to fit into the
system as a whole.

Each API function should parse the arguments it received through a lua state
first, and compute the total serialized size of a packet that encodes those
parameters. Then, after checking that the call queue of the target GUI has
enough space to hold the entire packet, it should first push its header.

The 8-byte header consists of a handler address and the total packet size.

The handler function should deserialize the parameters from a call queue, and
invoke the GUI function that actually reifies the effect provided by the API.

Each function should have a read-write formatter defined for it, which takes
the parameters representing a call, and deserializes or serializes them to a
binary single-writer, single-reader circular buffer.
*/

enum {
  FORMATTER_READ = 0,
  FORMATTER_WRITE = 1,
};

size_t ggdsw_size() { return GRID_GUI_CALL_HEADER_SIZE; }

void ggdsw_handler(struct grid_gui_model* gui, struct grid_swsr_t* swsr) { grid_gui_swap_set(gui, true); }

int l_grid_gui_draw_swap(lua_State* L) {

  int screen_index = luaL_checknumber(L, 1);
  if (!grid_gui_index_active(screen_index)) {
    return 1;
  }

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  size_t bytes = ggdsw_size();
  if (grid_gui_queue_push(gui, ggdsw_handler, bytes) != 0) {
    return 1;
  }

  return 0;
}

size_t ggdpx_size() { return GRID_GUI_CALL_HEADER_SIZE + sizeof(uint16_t) * 2 + sizeof(uint8_t) * 3; }

void ggdpx_formatter(struct grid_swsr_t* swsr, bool dir, uint16_t* x, uint16_t* y, uint8_t* r, uint8_t* g, uint8_t* b) {

  void (*access)(struct grid_swsr_t*, void*, int) = dir ? grid_swsr_write : grid_swsr_read;

  access(swsr, x, sizeof(uint16_t));
  access(swsr, y, sizeof(uint16_t));

  access(swsr, r, sizeof(uint8_t));
  access(swsr, g, sizeof(uint8_t));
  access(swsr, b, sizeof(uint8_t));
}

void ggdpx_handler(struct grid_gui_model* gui, struct grid_swsr_t* swsr) {

  uint16_t x, y;
  uint8_t r, g, b;

  ggdpx_formatter(swsr, FORMATTER_READ, &x, &y, &r, &g, &b);

  grid_gui_draw_pixel(gui, x, y, grid_gui_color_from_rgb(r, g, b));
}

int l_grid_gui_draw_pixel(lua_State* L) {

  int screen_index = luaL_checknumber(L, 1);
  if (!grid_gui_index_active(screen_index)) {
    return 1;
  }

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  uint16_t x = luaL_checknumber(L, 2);
  uint16_t y = luaL_checknumber(L, 3);

  luaL_checktype(L, 4, LUA_TTABLE);
  lua_rawgeti(L, 4, 1);
  uint8_t r = luaL_checknumber(L, -1);
  lua_rawgeti(L, 4, 2);
  uint8_t g = luaL_checknumber(L, -1);
  lua_rawgeti(L, 4, 3);
  uint8_t b = luaL_checknumber(L, -1);

  size_t bytes = ggdpx_size();
  if (grid_gui_queue_push(gui, ggdpx_handler, bytes) != 0) {
    return 1;
  }

  ggdpx_formatter(&gui->swsr, FORMATTER_WRITE, &x, &y, &r, &g, &b);

  return 0;
}

size_t ggdl_size() { return GRID_GUI_CALL_HEADER_SIZE + sizeof(uint16_t) * 4 + sizeof(uint8_t) * 3; }

void ggdl_formatter(struct grid_swsr_t* swsr, bool dir, uint16_t* x1, uint16_t* y1, uint16_t* x2, uint16_t* y2, uint8_t* r, uint8_t* g, uint8_t* b) {

  void (*access)(struct grid_swsr_t*, void*, int) = dir ? grid_swsr_write : grid_swsr_read;

  access(swsr, x1, sizeof(uint16_t));
  access(swsr, y1, sizeof(uint16_t));
  access(swsr, x2, sizeof(uint16_t));
  access(swsr, y2, sizeof(uint16_t));

  access(swsr, r, sizeof(uint8_t));
  access(swsr, g, sizeof(uint8_t));
  access(swsr, b, sizeof(uint8_t));
}

void ggdl_handler(struct grid_gui_model* gui, struct grid_swsr_t* swsr) {

  uint16_t x1, y1, x2, y2;
  uint8_t r, g, b;

  ggdl_formatter(swsr, FORMATTER_READ, &x1, &y1, &x2, &y2, &r, &g, &b);

  grid_gui_draw_line(gui, x1, y1, x2, y2, grid_gui_color_from_rgb(r, g, b));
}

int l_grid_gui_draw_line(lua_State* L) {

  int screen_index = luaL_checknumber(L, 1);
  if (!grid_gui_index_active(screen_index)) {
    return 1;
  }

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  uint16_t x1 = luaL_checknumber(L, 2);
  uint16_t y1 = luaL_checknumber(L, 3);
  uint16_t x2 = luaL_checknumber(L, 4);
  uint16_t y2 = luaL_checknumber(L, 5);

  luaL_checktype(L, 6, LUA_TTABLE);
  lua_rawgeti(L, 6, 1);
  uint8_t r = luaL_checknumber(L, -1);
  lua_rawgeti(L, 6, 2);
  uint8_t g = luaL_checknumber(L, -1);
  lua_rawgeti(L, 6, 3);
  uint8_t b = luaL_checknumber(L, -1);

  size_t bytes = ggdl_size();
  if (grid_gui_queue_push(gui, ggdl_handler, bytes) != 0) {
    return 1;
  }

  ggdl_formatter(&gui->swsr, FORMATTER_WRITE, &x1, &y1, &x2, &y2, &r, &g, &b);

  return 0;
}

enum {
  GRID_GUI_STYLE_RECTANGLE_BASE = 0,
  GRID_GUI_STYLE_RECTANGLE_FILLED,
  GRID_GUI_STYLE_RECTANGLE_ROUNDED,
  GRID_GUI_STYLE_RECTANGLE_ROUNDED_FILLED,
};

size_t ggdr_size() { return GRID_GUI_CALL_HEADER_SIZE + sizeof(uint16_t) * 4 + sizeof(uint8_t) * 3; }

void ggdr_formatter(struct grid_swsr_t* swsr, bool dir, uint16_t* x1, uint16_t* y1, uint16_t* x2, uint16_t* y2, uint8_t* r, uint8_t* g, uint8_t* b) {

  void (*access)(struct grid_swsr_t*, void*, int) = dir ? grid_swsr_write : grid_swsr_read;

  access(swsr, x1, sizeof(uint16_t));
  access(swsr, y1, sizeof(uint16_t));
  access(swsr, x2, sizeof(uint16_t));
  access(swsr, y2, sizeof(uint16_t));

  access(swsr, r, sizeof(uint8_t));
  access(swsr, g, sizeof(uint8_t));
  access(swsr, b, sizeof(uint8_t));
}

void ggdr_handler(struct grid_gui_model* gui, struct grid_swsr_t* swsr) {

  uint16_t x1, y1, x2, y2;
  uint8_t r, g, b;

  ggdr_formatter(swsr, FORMATTER_READ, &x1, &y1, &x2, &y2, &r, &g, &b);

  grid_gui_draw_rectangle(gui, x1, y1, x2, y2, grid_gui_color_from_rgb(r, g, b));
}

void ggdrf_handler(struct grid_gui_model* gui, struct grid_swsr_t* swsr) {

  uint16_t x1, y1, x2, y2;
  uint8_t r, g, b;

  ggdr_formatter(swsr, FORMATTER_READ, &x1, &y1, &x2, &y2, &r, &g, &b);

  grid_gui_draw_rectangle_filled(gui, x1, y1, x2, y2, grid_gui_color_from_rgb(r, g, b));
}

int l_grid_gui_draw_rectangle_style(lua_State* L, int style) {

  int screen_index = luaL_checknumber(L, 1);
  if (!grid_gui_index_active(screen_index)) {
    return 1;
  }

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  uint16_t x1 = luaL_checknumber(L, 2);
  uint16_t y1 = luaL_checknumber(L, 3);
  uint16_t x2 = luaL_checknumber(L, 4);
  uint16_t y2 = luaL_checknumber(L, 5);

  luaL_checktype(L, 6, LUA_TTABLE);
  lua_rawgeti(L, 6, 1);
  uint8_t r = luaL_checknumber(L, -1);
  lua_rawgeti(L, 6, 2);
  uint8_t g = luaL_checknumber(L, -1);
  lua_rawgeti(L, 6, 3);
  uint8_t b = luaL_checknumber(L, -1);

  grid_gui_draw_handler_t handler = NULL;
  switch (style) {
  case GRID_GUI_STYLE_RECTANGLE_BASE: {
    handler = ggdr_handler;
  } break;
  case GRID_GUI_STYLE_RECTANGLE_FILLED: {
    handler = ggdrf_handler;
  } break;
  }

  assert(handler);

  size_t bytes = ggdr_size();
  if (grid_gui_queue_push(gui, handler, bytes) != 0) {
    return 1;
  }

  ggdr_formatter(&gui->swsr, FORMATTER_WRITE, &x1, &y1, &x2, &y2, &r, &g, &b);

  return 0;
}

int l_grid_gui_draw_rectangle(lua_State* L) { return l_grid_gui_draw_rectangle_style(L, GRID_GUI_STYLE_RECTANGLE_BASE); }

int l_grid_gui_draw_rectangle_filled(lua_State* L) { return l_grid_gui_draw_rectangle_style(L, GRID_GUI_STYLE_RECTANGLE_FILLED); }

size_t ggdrr_size() { return GRID_GUI_CALL_HEADER_SIZE + sizeof(uint16_t) * 5 + sizeof(uint8_t) * 3; }

void ggdrr_formatter(struct grid_swsr_t* swsr, bool dir, uint16_t* x1, uint16_t* y1, uint16_t* x2, uint16_t* y2, uint16_t* rad, uint8_t* r, uint8_t* g, uint8_t* b) {

  void (*access)(struct grid_swsr_t*, void*, int) = dir ? grid_swsr_write : grid_swsr_read;

  access(swsr, x1, sizeof(uint16_t));
  access(swsr, y1, sizeof(uint16_t));
  access(swsr, x2, sizeof(uint16_t));
  access(swsr, y2, sizeof(uint16_t));
  access(swsr, rad, sizeof(uint16_t));

  access(swsr, r, sizeof(uint8_t));
  access(swsr, g, sizeof(uint8_t));
  access(swsr, b, sizeof(uint8_t));
}

void ggdrr_handler(struct grid_gui_model* gui, struct grid_swsr_t* swsr) {

  uint16_t x1, y1, x2, y2, rad;
  uint8_t r, g, b;

  ggdrr_formatter(swsr, FORMATTER_READ, &x1, &y1, &x2, &y2, &rad, &r, &g, &b);

  grid_gui_draw_rectangle_rounded(gui, x1, y1, x2, y2, rad, grid_gui_color_from_rgb(r, g, b));
}

void ggdrrf_handler(struct grid_gui_model* gui, struct grid_swsr_t* swsr) {

  uint16_t x1, y1, x2, y2, rad;
  uint8_t r, g, b;

  ggdrr_formatter(swsr, FORMATTER_READ, &x1, &y1, &x2, &y2, &rad, &r, &g, &b);

  grid_gui_draw_rectangle_rounded_filled(gui, x1, y1, x2, y2, rad, grid_gui_color_from_rgb(r, g, b));
}

int l_grid_gui_draw_rectangle_rounded_style(lua_State* L, int style) {

  int screen_index = luaL_checknumber(L, 1);
  if (!grid_gui_index_active(screen_index)) {
    return 1;
  }

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  uint16_t x1 = luaL_checknumber(L, 2);
  uint16_t y1 = luaL_checknumber(L, 3);
  uint16_t x2 = luaL_checknumber(L, 4);
  uint16_t y2 = luaL_checknumber(L, 5);
  uint16_t rad = luaL_checknumber(L, 6);

  luaL_checktype(L, 7, LUA_TTABLE);
  lua_rawgeti(L, 7, 1);
  uint8_t r = luaL_checknumber(L, -1);
  lua_rawgeti(L, 7, 2);
  uint8_t g = luaL_checknumber(L, -1);
  lua_rawgeti(L, 7, 3);
  uint8_t b = luaL_checknumber(L, -1);

  grid_gui_draw_handler_t handler = NULL;
  switch (style) {
  case GRID_GUI_STYLE_RECTANGLE_ROUNDED: {
    handler = ggdrr_handler;
  } break;
  case GRID_GUI_STYLE_RECTANGLE_ROUNDED_FILLED: {
    handler = ggdrrf_handler;
  } break;
  }

  assert(handler);

  size_t bytes = ggdrr_size();
  if (grid_gui_queue_push(gui, handler, bytes) != 0) {
    return 1;
  }

  ggdrr_formatter(&gui->swsr, FORMATTER_WRITE, &x1, &y1, &x2, &y2, &rad, &r, &g, &b);

  return 0;
}

int l_grid_gui_draw_rectangle_rounded(lua_State* L) { return l_grid_gui_draw_rectangle_rounded_style(L, GRID_GUI_STYLE_RECTANGLE_ROUNDED); }

int l_grid_gui_draw_rectangle_rounded_filled(lua_State* L) { return l_grid_gui_draw_rectangle_rounded_style(L, GRID_GUI_STYLE_RECTANGLE_ROUNDED_FILLED); }

enum {
  GRID_GUI_STYLE_POLYGON_BASE = 0,
  GRID_GUI_STYLE_POLYGON_FILLED,
};

size_t ggdpo_size(size_t points_count) { return GRID_GUI_CALL_HEADER_SIZE + sizeof(size_t) + sizeof(uint16_t) * 2 * points_count + sizeof(uint8_t) * 3; }

void ggdpo_formatter(struct grid_swsr_t* swsr, bool dir, size_t points, uint16_t* xs, uint16_t* ys, uint8_t* r, uint8_t* g, uint8_t* b) {

  void (*access)(struct grid_swsr_t*, void*, int) = dir ? grid_swsr_write : grid_swsr_read;

  if (dir) {
    access(swsr, &points, sizeof(size_t));
  }

  access(swsr, xs, sizeof(uint16_t) * points);
  access(swsr, ys, sizeof(uint16_t) * points);

  access(swsr, r, sizeof(uint8_t));
  access(swsr, g, sizeof(uint8_t));
  access(swsr, b, sizeof(uint8_t));
}

void ggdpo_handler(struct grid_gui_model* gui, struct grid_swsr_t* swsr) {

  size_t points;

  grid_swsr_read(swsr, &points, sizeof(size_t));

  uint16_t xs[points];
  uint16_t ys[points];
  uint8_t r, g, b;

  ggdpo_formatter(swsr, FORMATTER_READ, points, xs, ys, &r, &g, &b);

  grid_gui_draw_polygon(gui, xs, ys, points, grid_gui_color_from_rgb(r, g, b));
}

void ggdpf_handler(struct grid_gui_model* gui, struct grid_swsr_t* swsr) {

  size_t points;

  grid_swsr_read(swsr, &points, sizeof(size_t));

  uint16_t xs[points];
  uint16_t ys[points];
  uint8_t r, g, b;

  ggdpo_formatter(swsr, FORMATTER_READ, points, xs, ys, &r, &g, &b);

  grid_gui_draw_polygon_filled(gui, xs, ys, points, grid_gui_color_from_rgb(r, g, b));
}

int l_grid_gui_draw_polygon_style(lua_State* L, int style) {

  int screen_index = luaL_checknumber(L, 1);
  if (!grid_gui_index_active(screen_index)) {
    return 1;
  }

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  luaL_checktype(L, 2, LUA_TTABLE);

  size_t points = lua_rawlen(L, 2);

  uint16_t xs[points];
  uint16_t ys[points];

  for (size_t i = 0; i < points; i++) {

    lua_pushnumber(L, i + 1);
    lua_gettable(L, 2);
    xs[i] = lua_tonumber(L, -1);
    lua_pop(L, 1);
  }

  for (size_t i = 0; i < points; i++) {

    lua_pushnumber(L, i + 1);
    lua_gettable(L, 3);
    ys[i] = lua_tonumber(L, -1);
    lua_pop(L, 1);
  }

  // Default color is black
  uint8_t r = 0, g = 0, b = 0;
  if (lua_gettop(L) >= 4) {
    luaL_checktype(L, 4, LUA_TTABLE);
    lua_rawgeti(L, 4, 1);
    r = luaL_checknumber(L, -1);
    lua_rawgeti(L, 4, 2);
    g = luaL_checknumber(L, -1);
    lua_rawgeti(L, 4, 3);
    b = luaL_checknumber(L, -1);
  }

  grid_gui_draw_handler_t handler = NULL;
  switch (style) {
  case GRID_GUI_STYLE_POLYGON_BASE: {
    handler = ggdpo_handler;
  } break;
  case GRID_GUI_STYLE_POLYGON_FILLED: {
    handler = ggdpf_handler;
  } break;
  }

  assert(handler);

  size_t bytes = ggdpo_size(points);
  if (grid_gui_queue_push(gui, handler, bytes) != 0) {
    return 1;
  }

  ggdpo_formatter(&gui->swsr, FORMATTER_WRITE, points, xs, ys, &r, &g, &b);

  return 0;
}

int l_grid_gui_draw_polygon(lua_State* L) { return l_grid_gui_draw_polygon_style(L, GRID_GUI_STYLE_POLYGON_BASE); }

int l_grid_gui_draw_polygon_filled(lua_State* L) { return l_grid_gui_draw_polygon_style(L, GRID_GUI_STYLE_POLYGON_FILLED); }

enum {
  GRID_GUI_STYLE_TEXT_BASE = 0,
  GRID_GUI_STYLE_TEXT_FAST,
};

size_t ggdt_size(size_t length) { return GRID_GUI_CALL_HEADER_SIZE + sizeof(size_t) + sizeof(char) * length + sizeof(uint16_t) * 3 + sizeof(uint8_t) * 3; }

void ggdt_formatter(struct grid_swsr_t* swsr, bool dir, size_t length, char* str, uint16_t* fontsize, uint16_t* x, uint16_t* y, uint8_t* r, uint8_t* g, uint8_t* b) {

  void (*access)(struct grid_swsr_t*, void*, int) = dir ? grid_swsr_write : grid_swsr_read;

  if (dir) {
    access(swsr, &length, sizeof(size_t));
  }

  access(swsr, str, sizeof(char) * length);

  access(swsr, fontsize, sizeof(uint16_t));

  access(swsr, x, sizeof(uint16_t));
  access(swsr, y, sizeof(uint16_t));

  access(swsr, r, sizeof(uint8_t));
  access(swsr, g, sizeof(uint8_t));
  access(swsr, b, sizeof(uint8_t));
}

void ggdt_handler(struct grid_gui_model* gui, struct grid_swsr_t* swsr) {

  size_t length;

  grid_swsr_read(swsr, &length, sizeof(size_t));

  char str[length + 1];
  uint16_t fontsize;
  uint16_t x;
  uint16_t y;
  uint8_t r, g, b;

  ggdt_formatter(swsr, FORMATTER_READ, length, str, &fontsize, &x, &y, &r, &g, &b);

  str[length] = '\0';

  if (grid_font_state.initialized) {

    int cursor = 0;
    grid_font_draw_string(&grid_font_state, gui, x, y, fontsize, str, &cursor, grid_gui_color_from_rgb(r, g, b));
  }
}

void ggdft_handler(struct grid_gui_model* gui, struct grid_swsr_t* swsr) {

  size_t length;

  grid_swsr_read(swsr, &length, sizeof(size_t));

  char str[length + 1];
  uint16_t fontsize;
  uint16_t x;
  uint16_t y;
  uint8_t r, g, b;

  ggdt_formatter(swsr, FORMATTER_READ, length, str, &fontsize, &x, &y, &r, &g, &b);

  str[length] = '\0';

  int cursor = 0;
  grid_font_draw_string_fast(&grid_font_state, gui, x, y, fontsize, str, &cursor, grid_gui_color_from_rgb(r, g, b));
}

int l_grid_gui_draw_text_style(lua_State* L, int style) {

  int screen_index = luaL_checknumber(L, 1);
  if (!grid_gui_index_active(screen_index)) {
    return 1;
  }

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  const char* str = luaL_checkstring(L, 2);
  size_t length = strlen(str);

  uint16_t x = luaL_checknumber(L, 3);
  uint16_t y = luaL_checknumber(L, 4);

  // Default font size is 12
  uint16_t fontsize = 12;
  if (lua_gettop(L) >= 5) {
    fontsize = luaL_checknumber(L, 5);
  }

  // Default color is black
  uint8_t r = 0, g = 0, b = 0;
  if (lua_gettop(L) >= 6) {
    luaL_checktype(L, 6, LUA_TTABLE);
    lua_rawgeti(L, 6, 1);
    r = luaL_checknumber(L, -1);
    lua_rawgeti(L, 6, 2);
    g = luaL_checknumber(L, -1);
    lua_rawgeti(L, 6, 3);
    b = luaL_checknumber(L, -1);
  }

  grid_gui_draw_handler_t handler = NULL;
  switch (style) {
  case GRID_GUI_STYLE_TEXT_BASE: {
    handler = ggdt_handler;
  } break;
  case GRID_GUI_STYLE_TEXT_FAST: {
    handler = ggdft_handler;
  } break;
  }

  assert(handler);

  size_t bytes = ggdt_size(length);
  if (grid_gui_queue_push(gui, handler, bytes) != 0) {
    return 1;
  }

  ggdt_formatter(&gui->swsr, FORMATTER_WRITE, length, (char*)str, &fontsize, &x, &y, &r, &g, &b);

  return 0;
}

int l_grid_gui_draw_text(lua_State* L) { return l_grid_gui_draw_text_style(L, GRID_GUI_STYLE_TEXT_BASE); }

int l_grid_gui_draw_text_fast(lua_State* L) { return l_grid_gui_draw_text_style(L, GRID_GUI_STYLE_TEXT_FAST); }

size_t ggdaf_size() { return GRID_GUI_CALL_HEADER_SIZE + sizeof(uint16_t) * 4 + sizeof(uint8_t) * 3; }

void ggdaf_formatter(struct grid_swsr_t* swsr, bool dir, uint16_t* x1, uint16_t* y1, uint16_t* x2, uint16_t* y2, uint8_t* r, uint8_t* g, uint8_t* b) {

  void (*access)(struct grid_swsr_t*, void*, int) = dir ? grid_swsr_write : grid_swsr_read;

  access(swsr, x1, sizeof(uint16_t));
  access(swsr, y1, sizeof(uint16_t));
  access(swsr, x2, sizeof(uint16_t));
  access(swsr, y2, sizeof(uint16_t));

  access(swsr, r, sizeof(uint8_t));
  access(swsr, g, sizeof(uint8_t));
  access(swsr, b, sizeof(uint8_t));
}

void ggdaf_handler(struct grid_gui_model* gui, struct grid_swsr_t* swsr) {

  uint16_t x1, y1, x2, y2;
  uint8_t r, g, b;

  ggdaf_formatter(swsr, FORMATTER_READ, &x1, &y1, &x2, &y2, &r, &g, &b);

  grid_gui_draw_area_filled(gui, x1, y1, x2, y2, grid_gui_color_from_rgb(r, g, b));
}

int l_grid_gui_draw_area_filled(lua_State* L) {

  int screen_index = luaL_checknumber(L, 1);
  if (!grid_gui_index_active(screen_index)) {
    return 1;
  }

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  uint16_t x1 = luaL_checknumber(L, 2);
  uint16_t y1 = luaL_checknumber(L, 3);
  uint16_t x2 = luaL_checknumber(L, 4);
  uint16_t y2 = luaL_checknumber(L, 5);

  luaL_checktype(L, 6, LUA_TTABLE);
  lua_rawgeti(L, 6, 1);
  uint8_t r = luaL_checknumber(L, -1);
  lua_rawgeti(L, 6, 2);
  uint8_t g = luaL_checknumber(L, -1);
  lua_rawgeti(L, 6, 3);
  uint8_t b = luaL_checknumber(L, -1);

  size_t bytes = ggdaf_size();
  if (grid_gui_queue_push(gui, ggdaf_handler, bytes) != 0) {
    return 1;
  }

  ggdaf_formatter(&gui->swsr, FORMATTER_WRITE, &x1, &y1, &x2, &y2, &r, &g, &b);

  return 0;
}

size_t ggdim_size() { return GRID_GUI_CALL_HEADER_SIZE + sizeof(uint8_t) + sizeof(uint16_t) * 2; }

void ggdim_formatter(struct grid_swsr_t* swsr, bool dir, uint8_t* image_id, uint16_t* x, uint16_t* y) {

  void (*access)(struct grid_swsr_t*, void*, int) = dir ? grid_swsr_write : grid_swsr_read;

  access(swsr, image_id, sizeof(uint8_t));
  access(swsr, x, sizeof(uint16_t));
  access(swsr, y, sizeof(uint16_t));
}

void ggdim_handler(struct grid_gui_model* gui, struct grid_swsr_t* swsr) {

  uint8_t image_id;
  uint16_t x, y;

  ggdim_formatter(swsr, FORMATTER_READ, &image_id, &x, &y);

  grid_image_draw(gui, image_id, x, y);
}

/* ---- Loaded/decoded image handles ----

grid_image_file_handle / grid_image_decoded_handle are plain C heap structs,
NOT the Lua userdata objects themselves. This indirection matters: a queued
draw command in a screen's swsr ring buffer holds a raw pointer to one of
these, and that pointer must stay valid independent of when Lua's GC decides
to collect the (unrelated-lifetime) Lua wrapper object - if the refcount
lived directly inside Lua-managed userdata memory, Lua reclaiming that
userdata (which happens right after __gc returns, regardless of what __gc
decided about the refcount) would leave the queued command holding a
dangling pointer. So each Lua userdata is just a thin wrapper holding one
pointer ("impl") to the real, independently-heap-allocated control struct.

Refcounting rule, consistent across every release site below: a queued draw
command holds one reference (taken before grid_gui_queue_push, released by
its handler after the last read of the buffer); Lua's own live reference is
released either by explicit :free() (strict - only succeeds, and frees
immediately, when it is the sole reference) or by __gc (unconditional -
whichever release, __gc's or a handler's, happens to be the one that brings
the count to zero performs the actual free; neither side needs to know
which). heap_caps_malloc/free are safe to call from either task (ESP-IDF's
allocator has its own locking) - unlike LittleFS, which is why all the
actual file reading below happens only on the calling (Lua/main) task. */

struct grid_image_file_handle {
  unsigned char* data; // NULL once freed
  size_t size;
  _Atomic int refcount;
};

struct grid_image_file_handle_ud {
  struct grid_image_file_handle* impl; // NULL once this Lua reference is released
};

struct grid_image_decoded_handle {
  unsigned char* pixels; // NULL once freed
  int w, h;
  _Atomic int refcount;
};

struct grid_image_decoded_handle_ud {
  struct grid_image_decoded_handle* impl;
};

#define GRID_IMAGE_FILE_HANDLE_MT "grid_image_file_handle"
#define GRID_IMAGE_DECODED_HANDLE_MT "grid_image_decoded_handle"

// Unconditional release, shared by __gc and every queued-draw handler below:
// decrement, and free only if this decrement is the one that reached zero.
static void grid_image_file_handle_release(struct grid_image_file_handle* impl) {
  if (atomic_fetch_sub(&impl->refcount, 1) == 1) {
    GRID_IMAGE_FILE_FREE(impl->data);
    free(impl);
  }
}

static void grid_image_decoded_handle_release(struct grid_image_decoded_handle* impl) {
  if (atomic_fetch_sub(&impl->refcount, 1) == 1) {
    grid_image_free_pixels(impl->pixels);
    free(impl);
  }
}

static int l_grid_image_file_handle_free(lua_State* L) {

  struct grid_image_file_handle_ud* ud = (struct grid_image_file_handle_ud*)luaL_checkudata(L, 1, GRID_IMAGE_FILE_HANDLE_MT);

  if (ud->impl == NULL) {
    lua_pushnil(L);
    lua_pushstring(L, "handle already freed");
    return 2;
  }

  // Strict: only succeeds (and frees immediately) if this is the sole
  // reference. Rejects rather than deferring, so a script gets an
  // unambiguous, synchronous answer - consistent with grid_gui_queue_push
  // already failing fast elsewhere in this API instead of blocking/deferring.
  int expected = 1;
  if (atomic_compare_exchange_strong(&ud->impl->refcount, &expected, 0)) {
    GRID_IMAGE_FILE_FREE(ud->impl->data);
    free(ud->impl);
    ud->impl = NULL;
    lua_pushboolean(L, true);
    return 1;
  }

  lua_pushnil(L);
  lua_pushfstring(L, "still in use (%d pending)", expected - 1);
  return 2;
}

static int l_grid_image_file_handle_gc(lua_State* L) {

  struct grid_image_file_handle_ud* ud = (struct grid_image_file_handle_ud*)luaL_checkudata(L, 1, GRID_IMAGE_FILE_HANDLE_MT);

  // Deliberately NOT the same logic as :free() above: __gc has no caller to
  // report "still in use" to, and the Lua-side object is destroyed after
  // this returns regardless of what we do - so unlike :free(), this must
  // unconditionally release Lua's own share. If a queued draw still holds
  // its own share, this decrement won't be the one that reaches zero, and
  // that queued draw's own eventual release will be.
  if (ud->impl != NULL) {
    grid_image_file_handle_release(ud->impl);
    ud->impl = NULL;
  }

  return 0;
}

static void grid_image_file_handle_ensure_metatable(lua_State* L) {

  if (luaL_newmetatable(L, GRID_IMAGE_FILE_HANDLE_MT)) {
    lua_pushcfunction(L, l_grid_image_file_handle_free);
    lua_setfield(L, -2, "free");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, l_grid_image_file_handle_gc);
    lua_setfield(L, -2, "__gc");
  }
  lua_pop(L, 1);
}

static int l_grid_image_decoded_handle_free(lua_State* L) {

  struct grid_image_decoded_handle_ud* ud = (struct grid_image_decoded_handle_ud*)luaL_checkudata(L, 1, GRID_IMAGE_DECODED_HANDLE_MT);

  if (ud->impl == NULL) {
    lua_pushnil(L);
    lua_pushstring(L, "handle already freed");
    return 2;
  }

  int expected = 1;
  if (atomic_compare_exchange_strong(&ud->impl->refcount, &expected, 0)) {
    grid_image_free_pixels(ud->impl->pixels);
    free(ud->impl);
    ud->impl = NULL;
    lua_pushboolean(L, true);
    return 1;
  }

  lua_pushnil(L);
  lua_pushfstring(L, "still in use (%d pending)", expected - 1);
  return 2;
}

static int l_grid_image_decoded_handle_gc(lua_State* L) {

  struct grid_image_decoded_handle_ud* ud = (struct grid_image_decoded_handle_ud*)luaL_checkudata(L, 1, GRID_IMAGE_DECODED_HANDLE_MT);

  if (ud->impl != NULL) {
    grid_image_decoded_handle_release(ud->impl);
    ud->impl = NULL;
  }

  return 0;
}

static void grid_image_decoded_handle_ensure_metatable(lua_State* L) {

  if (luaL_newmetatable(L, GRID_IMAGE_DECODED_HANDLE_MT)) {
    lua_pushcfunction(L, l_grid_image_decoded_handle_free);
    lua_setfield(L, -2, "free");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, l_grid_image_decoded_handle_gc);
    lua_setfield(L, -2, "__gc");
  }
  lua_pop(L, 1);
}

#ifndef __EMSCRIPTEN__
// Internal, silent (printf-diagnostic only, no Lua error) file load used by
// the ldim(path,...) backward-compat sugar path below - mirrors the old
// grid_image_draw_from_file's diagnostics. load_file() (the Lua-visible,
// nil+err-reporting version) is a separate, small duplication of this rather
// than sharing it, since the two have genuinely different error-reporting
// needs (silent+printf here; Lua-visible there).
static struct grid_image_file_handle* grid_image_file_handle_create_silent(const char* path) {

  void* statbuf;
  if (grid_platform_stat(path, &statbuf) != 0) {
    grid_platform_printf("grid_image_file_handle_create_silent: stat failed for \"%s\"\n", path);
    return NULL;
  }

  size_t size = grid_platform_file_info_size(statbuf);
  if (size == 0) {
    grid_platform_printf("grid_image_file_handle_create_silent: \"%s\" is empty\n", path);
    return NULL;
  }

  unsigned char* data = GRID_IMAGE_FILE_MALLOC(size);
  if (data == NULL) {
    grid_platform_printf("grid_image_file_handle_create_silent: malloc(%u) failed for \"%s\"\n", (unsigned int)size, path);
    return NULL;
  }

  void* file = grid_platform_fopen(path, "r");
  if (file == NULL) {
    grid_platform_printf("grid_image_file_handle_create_silent: fopen failed for \"%s\"\n", path);
    GRID_IMAGE_FILE_FREE(data);
    return NULL;
  }

  bool ok = grid_platform_fread(data, size, 1, file) == 1;
  grid_platform_fclose(file);

  if (!ok) {
    grid_platform_printf("grid_image_file_handle_create_silent: fread failed for \"%s\"\n", path);
    GRID_IMAGE_FILE_FREE(data);
    return NULL;
  }

  struct grid_image_file_handle* impl = malloc(sizeof(struct grid_image_file_handle));
  if (impl == NULL) {
    GRID_IMAGE_FILE_FREE(data);
    return NULL;
  }

  impl->data = data;
  impl->size = size;
  atomic_init(&impl->refcount, 1);

  return impl;
}
#endif

/* ---- load_file / decode_image_from_file ---- */

#ifndef __EMSCRIPTEN__
int l_grid_load_file(lua_State* L) {

  const char* path = luaL_checkstring(L, 1);

  void* statbuf;
  if (grid_platform_stat(path, &statbuf) != 0) {
    lua_pushnil(L);
    lua_pushfstring(L, "failed to stat file: %s", path);
    return 2;
  }

  size_t size = grid_platform_file_info_size(statbuf);
  if (size == 0) {
    lua_pushnil(L);
    lua_pushfstring(L, "empty file: %s", path);
    return 2;
  }

  unsigned char* data = GRID_IMAGE_FILE_MALLOC(size);
  if (data == NULL) {
    lua_pushnil(L);
    lua_pushstring(L, "out of memory");
    return 2;
  }

  void* file = grid_platform_fopen(path, "r");
  if (file == NULL) {
    GRID_IMAGE_FILE_FREE(data);
    lua_pushnil(L);
    lua_pushfstring(L, "failed to open file: %s", path);
    return 2;
  }

  bool ok = grid_platform_fread(data, size, 1, file) == 1;
  grid_platform_fclose(file);

  if (!ok) {
    GRID_IMAGE_FILE_FREE(data);
    lua_pushnil(L);
    lua_pushfstring(L, "failed to read file: %s", path);
    return 2;
  }

  struct grid_image_file_handle* impl = malloc(sizeof(struct grid_image_file_handle));
  if (impl == NULL) {
    GRID_IMAGE_FILE_FREE(data);
    lua_pushnil(L);
    lua_pushstring(L, "out of memory");
    return 2;
  }

  impl->data = data;
  impl->size = size;
  atomic_init(&impl->refcount, 1);

  grid_image_file_handle_ensure_metatable(L);

  struct grid_image_file_handle_ud* ud = (struct grid_image_file_handle_ud*)lua_newuserdatauv(L, sizeof(struct grid_image_file_handle_ud), 0);
  ud->impl = impl;
  luaL_setmetatable(L, GRID_IMAGE_FILE_HANDLE_MT);

  return 1;
}
#else
// No LittleFS in the wasm simulator.
int l_grid_load_file(lua_State* L) {
  lua_pushnil(L);
  lua_pushstring(L, "no filesystem in the wasm simulator");
  return 2;
}
#endif

int l_grid_decode_image_from_file(lua_State* L) {

  struct grid_image_file_handle_ud* ud = (struct grid_image_file_handle_ud*)luaL_checkudata(L, 1, GRID_IMAGE_FILE_HANDLE_MT);

  if (ud->impl == NULL) {
    lua_pushnil(L);
    lua_pushstring(L, "file handle already freed");
    return 2;
  }

  int w, h;
  unsigned char* pixels = grid_image_decode(ud->impl->data, (int)ud->impl->size, &w, &h);
  if (pixels == NULL) {
    const char* reason = grid_image_last_error();
    lua_pushnil(L);
    lua_pushfstring(L, "decode failed: %s", reason ? reason : "");
    return 2;
  }

  struct grid_image_decoded_handle* impl = malloc(sizeof(struct grid_image_decoded_handle));
  if (impl == NULL) {
    grid_image_free_pixels(pixels);
    lua_pushnil(L);
    lua_pushstring(L, "out of memory");
    return 2;
  }

  impl->pixels = pixels;
  impl->w = w;
  impl->h = h;
  atomic_init(&impl->refcount, 1);

  grid_image_decoded_handle_ensure_metatable(L);

  struct grid_image_decoded_handle_ud* out = (struct grid_image_decoded_handle_ud*)lua_newuserdatauv(L, sizeof(struct grid_image_decoded_handle_ud), 0);
  out->impl = impl;
  luaL_setmetatable(L, GRID_IMAGE_DECODED_HANDLE_MT);

  return 1;
}

/* ---- draw_decode_image: queued decode+blit from a loaded file handle ---- */

size_t ggddi_size() { return GRID_GUI_CALL_HEADER_SIZE + sizeof(void*) + sizeof(uint16_t) * 2; }

void ggddi_formatter(struct grid_swsr_t* swsr, bool dir, struct grid_image_file_handle** impl, uint16_t* x, uint16_t* y) {

  void (*access)(struct grid_swsr_t*, void*, int) = dir ? grid_swsr_write : grid_swsr_read;

  access(swsr, impl, sizeof(void*));
  access(swsr, x, sizeof(uint16_t));
  access(swsr, y, sizeof(uint16_t));
}

void ggddi_handler(struct grid_gui_model* gui, struct grid_swsr_t* swsr) {

  struct grid_image_file_handle* impl;
  uint16_t x, y;

  ggddi_formatter(swsr, FORMATTER_READ, &impl, &x, &y);

  int w, h;
  unsigned char* pixels = grid_image_decode(impl->data, (int)impl->size, &w, &h);
  if (pixels != NULL) {
    grid_image_blit_pixels(gui, pixels, w, h, x, y);
    grid_image_free_pixels(pixels);
  }

  // The compressed bytes aren't needed past decode, so the reference is
  // released here rather than waiting for the whole handler to finish.
  grid_image_file_handle_release(impl);
}

int l_grid_gui_draw_decode_image(lua_State* L) {

  int screen_index = luaL_checknumber(L, 1);
  if (!grid_gui_index_active(screen_index)) {
    lua_pushnil(L);
    lua_pushstring(L, "inactive screen");
    return 2;
  }

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  struct grid_image_file_handle_ud* ud = (struct grid_image_file_handle_ud*)luaL_checkudata(L, 2, GRID_IMAGE_FILE_HANDLE_MT);
  if (ud->impl == NULL) {
    lua_pushnil(L);
    lua_pushstring(L, "file handle already freed");
    return 2;
  }

  uint16_t x = luaL_checknumber(L, 3);
  uint16_t y = luaL_checknumber(L, 4);

  atomic_fetch_add(&ud->impl->refcount, 1);

  size_t bytes = ggddi_size();
  if (grid_gui_queue_push(gui, ggddi_handler, bytes) != 0) {
    atomic_fetch_sub(&ud->impl->refcount, 1); // undo: never the decrement that reaches zero, Lua's own share is still live
    lua_pushnil(L);
    lua_pushstring(L, "draw queue full");
    return 2;
  }

  struct grid_image_file_handle* impl = ud->impl;
  ggddi_formatter(&gui->swsr, FORMATTER_WRITE, &impl, &x, &y);

  lua_pushboolean(L, true);
  return 1;
}

/* ---- draw_blit_image: queued blit-only from an already-decoded handle ---- */

size_t ggdbi_size() { return GRID_GUI_CALL_HEADER_SIZE + sizeof(void*) + sizeof(uint16_t) * 2; }

void ggdbi_formatter(struct grid_swsr_t* swsr, bool dir, struct grid_image_decoded_handle** impl, uint16_t* x, uint16_t* y) {

  void (*access)(struct grid_swsr_t*, void*, int) = dir ? grid_swsr_write : grid_swsr_read;

  access(swsr, impl, sizeof(void*));
  access(swsr, x, sizeof(uint16_t));
  access(swsr, y, sizeof(uint16_t));
}

void ggdbi_handler(struct grid_gui_model* gui, struct grid_swsr_t* swsr) {

  struct grid_image_decoded_handle* impl;
  uint16_t x, y;

  ggdbi_formatter(swsr, FORMATTER_READ, &impl, &x, &y);

  grid_image_blit_pixels(gui, impl->pixels, impl->w, impl->h, x, y);

  grid_image_decoded_handle_release(impl);
}

int l_grid_gui_draw_blit_image(lua_State* L) {

  int screen_index = luaL_checknumber(L, 1);
  if (!grid_gui_index_active(screen_index)) {
    lua_pushnil(L);
    lua_pushstring(L, "inactive screen");
    return 2;
  }

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  struct grid_image_decoded_handle_ud* ud = (struct grid_image_decoded_handle_ud*)luaL_checkudata(L, 2, GRID_IMAGE_DECODED_HANDLE_MT);
  if (ud->impl == NULL) {
    lua_pushnil(L);
    lua_pushstring(L, "image handle already freed");
    return 2;
  }

  uint16_t x = luaL_checknumber(L, 3);
  uint16_t y = luaL_checknumber(L, 4);

  atomic_fetch_add(&ud->impl->refcount, 1);

  size_t bytes = ggdbi_size();
  if (grid_gui_queue_push(gui, ggdbi_handler, bytes) != 0) {
    atomic_fetch_sub(&ud->impl->refcount, 1);
    lua_pushnil(L);
    lua_pushstring(L, "draw queue full");
    return 2;
  }

  struct grid_image_decoded_handle* impl = ud->impl;
  ggdbi_formatter(&gui->swsr, FORMATTER_WRITE, &impl, &x, &y);

  lua_pushboolean(L, true);
  return 1;
}

// gui_draw_image(screen_index, image_id_or_path, x, y): a string second
// argument is a LittleFS path, a number is a predefined embedded image id.
// The string-path case is sugar over load_file+draw_decode_image, kept for
// backward compatibility with the original one-shot call shape: there is no
// Lua-visible handle here to retry :free() on, so cleanup uses the same
// unconditional release __gc uses (see grid_image_file_handle_release above)
// rather than :free()'s strict one - this is exactly what would happen if a
// script created a local handle and let it go out of scope right after
// queuing the draw.
int l_grid_gui_draw_image(lua_State* L) {

  int screen_index = luaL_checknumber(L, 1);
  if (!grid_gui_index_active(screen_index)) {
    return 1;
  }

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  if (lua_type(L, 2) == LUA_TSTRING) {

#ifndef __EMSCRIPTEN__
    const char* path = lua_tostring(L, 2);
    uint16_t x = luaL_checknumber(L, 3);
    uint16_t y = luaL_checknumber(L, 4);

    struct grid_image_file_handle* impl = grid_image_file_handle_create_silent(path);
    if (impl == NULL) {
      return 1;
    }

    size_t bytes = ggddi_size();
    if (grid_gui_queue_push(gui, ggddi_handler, bytes) != 0) {
      grid_image_file_handle_release(impl); // never touched by anything else yet - this call frees it
      return 1;
    }

    ggddi_formatter(&gui->swsr, FORMATTER_WRITE, &impl, &x, &y);
#endif

    return 0;
  }

  uint8_t image_id = luaL_checknumber(L, 2);
  uint16_t x = luaL_checknumber(L, 3);
  uint16_t y = luaL_checknumber(L, 4);

  size_t bytes = ggdim_size();
  if (grid_gui_queue_push(gui, ggdim_handler, bytes) != 0) {
    return 1;
  }

  ggdim_formatter(&gui->swsr, FORMATTER_WRITE, &image_id, &x, &y);

  return 0;
}

size_t ggdd_size() { return GRID_GUI_CALL_HEADER_SIZE + sizeof(uint8_t); }

void ggdd_formatter(struct grid_swsr_t* swsr, bool dir, uint8_t* counter) {

  void (*access)(struct grid_swsr_t*, void*, int) = dir ? grid_swsr_write : grid_swsr_read;

  access(swsr, counter, sizeof(uint8_t));
}

void ggdd_handler(struct grid_gui_model* gui, struct grid_swsr_t* swsr) {

  uint8_t counter;

  ggdd_formatter(swsr, FORMATTER_READ, &counter);

  grid_gui_draw_demo(gui, counter);
}

int l_grid_gui_draw_demo(lua_State* L) {

  int screen_index = luaL_checknumber(L, 1);
  if (!grid_gui_index_active(screen_index)) {
    return 1;
  }

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  uint8_t counter = luaL_checknumber(L, 2);

  size_t bytes = ggdd_size();
  if (grid_gui_queue_push(gui, ggdd_handler, bytes) != 0) {
    return 1;
  }

  ggdd_formatter(&gui->swsr, FORMATTER_WRITE, &counter);

  return 0;
}

int l_grid_gui_get_render_time(lua_State* L) {

  int screen_index = luaL_checknumber(L, 1);
  if (!grid_gui_index_active(screen_index)) {
    return 0;
  }

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  lua_pushinteger(L, gui->render_time);

  return 1;
}

#include "grid_lua_api.h"

GRID_LUA_FNC_DRAW_DEFI(ldsw, l_grid_gui_draw_swap)
GRID_LUA_FNC_DRAW_DEFI(ldpx, l_grid_gui_draw_pixel)
GRID_LUA_FNC_DRAW_DEFI(ldl, l_grid_gui_draw_line)
GRID_LUA_FNC_DRAW_DEFI(ldr, l_grid_gui_draw_rectangle)
GRID_LUA_FNC_DRAW_DEFI(ldrf, l_grid_gui_draw_rectangle_filled)
GRID_LUA_FNC_DRAW_DEFI(ldrr, l_grid_gui_draw_rectangle_rounded)
GRID_LUA_FNC_DRAW_DEFI(ldrrf, l_grid_gui_draw_rectangle_rounded_filled)
GRID_LUA_FNC_DRAW_DEFI(ldpo, l_grid_gui_draw_polygon)
GRID_LUA_FNC_DRAW_DEFI(ldpof, l_grid_gui_draw_polygon_filled)
GRID_LUA_FNC_DRAW_DEFI(ldt, l_grid_gui_draw_text)
GRID_LUA_FNC_DRAW_DEFI(ldft, l_grid_gui_draw_text_fast)
GRID_LUA_FNC_DRAW_DEFI(ldaf, l_grid_gui_draw_area_filled)
GRID_LUA_FNC_DRAW_DEFI(ldim, l_grid_gui_draw_image)
GRID_LUA_FNC_DRAW_DEFI(lddi, l_grid_gui_draw_decode_image)
GRID_LUA_FNC_DRAW_DEFI(ldbi, l_grid_gui_draw_blit_image)
GRID_LUA_FNC_DRAW_DEFI(ldd, l_grid_gui_draw_demo)
GRID_LUA_FNC_DRAW_DEFI(lgrt, l_grid_gui_get_render_time)

/*static*/ struct luaL_Reg grid_lua_api_gui_lib[] = {
    {GRID_LUA_FNC_G_GUI_DRAW_SWAP_short, GRID_LUA_FNC_G_GUI_DRAW_SWAP_fnptr},
    {GRID_LUA_FNC_G_GUI_DRAW_PIXEL_short, GRID_LUA_FNC_G_GUI_DRAW_PIXEL_fnptr},
    {GRID_LUA_FNC_G_GUI_DRAW_LINE_short, GRID_LUA_FNC_G_GUI_DRAW_LINE_fnptr},
    {GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_short, GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_fnptr},
    {GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_FILLED_short, GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_FILLED_fnptr},
    {GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_ROUNDED_short, GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_ROUNDED_fnptr},
    {GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_ROUNDED_FILLED_short, GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_ROUNDED_FILLED_fnptr},
    {GRID_LUA_FNC_G_GUI_DRAW_POLYGON_short, GRID_LUA_FNC_G_GUI_DRAW_POLYGON_fnptr},
    {GRID_LUA_FNC_G_GUI_DRAW_POLYGON_FILLED_short, GRID_LUA_FNC_G_GUI_DRAW_POLYGON_FILLED_fnptr},
    {GRID_LUA_FNC_G_GUI_DRAW_FASTTEXT_short, GRID_LUA_FNC_G_GUI_DRAW_FASTTEXT_fnptr},
    {GRID_LUA_FNC_G_GUI_DRAW_TEXT_short, GRID_LUA_FNC_G_GUI_DRAW_TEXT_fnptr},
    {GRID_LUA_FNC_G_GUI_DRAW_AREA_FILLED_short, GRID_LUA_FNC_G_GUI_DRAW_AREA_FILLED_fnptr},
    {GRID_LUA_FNC_G_GUI_DRAW_IMAGE_short, GRID_LUA_FNC_G_GUI_DRAW_IMAGE_fnptr},
    {GRID_LUA_FNC_G_LOAD_FILE_short, GRID_LUA_FNC_G_LOAD_FILE_fnptr},
    {GRID_LUA_FNC_G_DECODE_IMAGE_FROM_FILE_short, GRID_LUA_FNC_G_DECODE_IMAGE_FROM_FILE_fnptr},
    {GRID_LUA_FNC_G_GUI_DRAW_DECODE_IMAGE_short, GRID_LUA_FNC_G_GUI_DRAW_DECODE_IMAGE_fnptr},
    {GRID_LUA_FNC_G_GUI_DRAW_BLIT_IMAGE_short, GRID_LUA_FNC_G_GUI_DRAW_BLIT_IMAGE_fnptr},
    {GRID_LUA_FNC_G_GUI_DRAW_DEMO_short, GRID_LUA_FNC_G_GUI_DRAW_DEMO_fnptr},
    {GRID_LUA_FNC_G_GUI_GET_RENDER_TIME_short, GRID_LUA_FNC_G_GUI_GET_RENDER_TIME_fnptr},
    {NULL, NULL} /* end of array */
};

struct luaL_Reg* grid_lua_api_gui_lib_reference = grid_lua_api_gui_lib;
