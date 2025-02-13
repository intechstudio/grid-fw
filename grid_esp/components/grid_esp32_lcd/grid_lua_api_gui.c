#include "grid_lua_api_gui.h"

extern void grid_platform_printf(char const* fmt, ...);

enum {
  FORMATTER_READ = 0,
  FORMATTER_WRITE = 1,
};

inline size_t ggdsw_size() { return GRID_GUI_DRAW_HEADER_SIZE; }

void ggdsw_handler(struct grid_gui_model* gui, struct grid_swsr_t* swsr) { grid_gui_draw_swap(gui); }

int l_grid_gui_draw_swap(lua_State* L) {

  int screen_index = luaL_checknumber(L, 1);

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  size_t bytes = ggdsw_size();
  if (grid_gui_queue_push(gui, ggdsw_handler, bytes) != 0) {
    return 1;
  }

  return 0;
}

inline size_t ggdpx_size() { return GRID_GUI_DRAW_HEADER_SIZE + sizeof(uint16_t) * 2 + sizeof(uint8_t) * 3; }

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

inline size_t ggdl_size() { return GRID_GUI_DRAW_HEADER_SIZE + sizeof(uint16_t) * 4 + sizeof(uint8_t) * 3; }

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

inline size_t ggdr_size() { return GRID_GUI_DRAW_HEADER_SIZE + sizeof(uint16_t) * 4 + sizeof(uint8_t) * 3; }

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

inline size_t ggdrr_size() { return GRID_GUI_DRAW_HEADER_SIZE + sizeof(uint16_t) * 5 + sizeof(uint8_t) * 3; }

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

inline size_t ggdpo_size(size_t points_count) { return GRID_GUI_DRAW_HEADER_SIZE + sizeof(uint16_t) * 2 * points_count + sizeof(uint8_t) * 3; }

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

  ggdpo_formatter(swsr, FORMATTER_READ, points, &xs, &ys, &r, &g, &b);

  grid_gui_draw_polygon(gui, xs, ys, points, grid_gui_color_from_rgb(r, g, b));
}

void ggdpf_handler(struct grid_gui_model* gui, struct grid_swsr_t* swsr) {

  size_t points;

  grid_swsr_read(swsr, &points, sizeof(size_t));

  uint16_t xs[points];
  uint16_t ys[points];
  uint8_t r, g, b;

  ggdpo_formatter(swsr, FORMATTER_READ, points, &xs, &ys, &r, &g, &b);

  grid_gui_draw_polygon_filled(gui, xs, ys, points, grid_gui_color_from_rgb(r, g, b));
}

int l_grid_gui_draw_polygon_style(lua_State* L, int style) {

  int screen_index = luaL_checknumber(L, 1);

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

inline size_t ggdt_size(size_t length) { return GRID_GUI_DRAW_HEADER_SIZE + sizeof(size_t) + sizeof(char) * length + sizeof(uint16_t) * 3 + sizeof(uint8_t) * 3; }

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

int l_grid_gui_draw_text(lua_State* L) {

  int screen_index = luaL_checknumber(L, 1);

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

  size_t bytes = ggdt_size(length);
  if (grid_gui_queue_push(gui, ggdt_handler, bytes) != 0) {
    return 1;
  }

  ggdt_formatter(&gui->swsr, FORMATTER_WRITE, length, str, &fontsize, &x, &y, &r, &g, &b);

  return 0;
}

inline size_t ggdd_size() { return GRID_GUI_DRAW_HEADER_SIZE + sizeof(uint8_t); }

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

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  uint8_t counter = luaL_checknumber(L, 2);

  size_t bytes = ggdd_size();
  if (grid_gui_queue_push(gui, ggdd_handler, bytes) != 0) {
    return 1;
  }

  ggdd_formatter(&gui->swsr, FORMATTER_WRITE, &counter);

  return 0;
}

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
    {GRID_LUA_FNC_G_GUI_DRAW_TEXT_short, GRID_LUA_FNC_G_GUI_DRAW_TEXT_fnptr},
    {GRID_LUA_FNC_G_GUI_DRAW_DEMO_short, GRID_LUA_FNC_G_GUI_DRAW_DEMO_fnptr},
    {NULL, NULL} /* end of array */
};

struct luaL_Reg* grid_lua_api_gui_lib_reference = grid_lua_api_gui_lib;
