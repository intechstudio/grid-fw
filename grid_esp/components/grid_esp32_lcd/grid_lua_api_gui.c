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

int l_grid_gui_draw_rectangle(lua_State* L) {

  int screen_index = luaL_checknumber(L, 1);

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  int x1 = luaL_checknumber(L, 2);
  int y1 = luaL_checknumber(L, 3);
  int x2 = luaL_checknumber(L, 4);
  int y2 = luaL_checknumber(L, 5);

  luaL_checktype(L, 6, LUA_TTABLE);
  lua_rawgeti(L, 6, 1);
  int r = luaL_checknumber(L, -1);
  lua_rawgeti(L, 6, 2);
  int g = luaL_checknumber(L, -1);
  lua_rawgeti(L, 6, 3);
  int b = luaL_checknumber(L, -1);

  grid_gui_draw_rectangle(gui, x1, y1, x2, y2, grid_gui_color_from_rgb(r, g, b));

  return 0;
}

int l_grid_gui_draw_rectangle_filled(lua_State* L) {

  int screen_index = luaL_checknumber(L, 1);

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  int x1 = luaL_checknumber(L, 2);
  int y1 = luaL_checknumber(L, 3);
  int x2 = luaL_checknumber(L, 4);
  int y2 = luaL_checknumber(L, 5);

  luaL_checktype(L, 6, LUA_TTABLE);
  lua_rawgeti(L, 6, 1);
  int r = luaL_checknumber(L, -1);
  lua_rawgeti(L, 6, 2);
  int g = luaL_checknumber(L, -1);
  lua_rawgeti(L, 6, 3);
  int b = luaL_checknumber(L, -1);

  grid_gui_draw_rectangle_filled(gui, x1, y1, x2, y2, grid_gui_color_from_rgb(r, g, b));

  return 0;
}

int l_grid_gui_draw_rectangle_rounded(lua_State* L) {

  int screen_index = luaL_checknumber(L, 1);

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  int x1 = luaL_checknumber(L, 2);
  int y1 = luaL_checknumber(L, 3);
  int x2 = luaL_checknumber(L, 4);
  int y2 = luaL_checknumber(L, 5);
  int radius = luaL_checknumber(L, 6);

  luaL_checktype(L, 7, LUA_TTABLE);
  lua_rawgeti(L, 7, 1);
  int r = luaL_checknumber(L, -1);
  lua_rawgeti(L, 7, 2);
  int g = luaL_checknumber(L, -1);
  lua_rawgeti(L, 7, 3);
  int b = luaL_checknumber(L, -1);

  grid_gui_draw_rectangle_rounded(gui, x1, y1, x2, y2, radius, grid_gui_color_from_rgb(r, g, b));

  return 0;
}

int l_grid_gui_draw_rectangle_rounded_filled(lua_State* L) {

  int screen_index = luaL_checknumber(L, 1);

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  int x1 = luaL_checknumber(L, 2);
  int y1 = luaL_checknumber(L, 3);
  int x2 = luaL_checknumber(L, 4);
  int y2 = luaL_checknumber(L, 5);
  int radius = luaL_checknumber(L, 6);

  luaL_checktype(L, 7, LUA_TTABLE);
  lua_rawgeti(L, 7, 1);
  int r = luaL_checknumber(L, -1);
  lua_rawgeti(L, 7, 2);
  int g = luaL_checknumber(L, -1);
  lua_rawgeti(L, 7, 3);
  int b = luaL_checknumber(L, -1);

  grid_gui_draw_rectangle_rounded_filled(gui, x1, y1, x2, y2, radius, grid_gui_color_from_rgb(r, g, b));

  return 0;
}

int l_grid_gui_draw_polygon(lua_State* L) {

  int screen_index = luaL_checknumber(L, 1);

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  // The C function that will be called from Lua
  // Check that the first argument is a table
  if (!lua_istable(L, 2)) {
    return luaL_error(L, "Expected a table as the argument");
  }

  // Get the length of the table
  size_t num_points = lua_rawlen(L, 2);
  uint16_t x_points[num_points];
  uint16_t y_points[num_points];

  // Iterate over the table and print the numbers
  for (size_t i = 0; i < num_points; i++) {
    // Push the index onto the stack
    lua_pushnumber(L, i + 1);

    // Get the value at the given index in the table
    lua_gettable(L, 2);

    // Get the number
    x_points[i] = lua_tonumber(L, -1);

    // printf("x[%d]=%d", i, x_points[i]);
    //  Pop the value off the stack
    lua_pop(L, 1);
  }

  // Iterate over the table and print the numbers
  for (size_t i = 0; i < num_points; i++) {
    // Push the index onto the stack
    lua_pushnumber(L, i + 1);

    // Get the value at the given index in the table
    lua_gettable(L, 3);

    // Get the number
    y_points[i] = lua_tonumber(L, -1);
    // printf("x[%d]=%d", i, y_points[i]);

    // Pop the value off the stack
    lua_pop(L, 1);
  }

  int r = 0, g = 0, b = 0; // Default color: black
  if (lua_gettop(L) >= 4) {
    luaL_checktype(L, 4, LUA_TTABLE);
    lua_rawgeti(L, 4, 1);
    r = luaL_checknumber(L, -1);
    lua_rawgeti(L, 4, 2);
    g = luaL_checknumber(L, -1);
    lua_rawgeti(L, 4, 3);
    b = luaL_checknumber(L, -1);
  }

  grid_gui_draw_polygon(gui, x_points, y_points, num_points, grid_gui_color_from_rgb(r, g, b));

  return 0; // Number of return values
}

int l_grid_gui_draw_polygon_filled(lua_State* L) {

  int screen_index = luaL_checknumber(L, 1);

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  // The C function that will be called from Lua
  // Check that the first argument is a table
  if (!lua_istable(L, 2)) {
    return luaL_error(L, "Expected a table as the argument");
  }

  // Get the length of the table
  size_t num_points = lua_rawlen(L, 2);
  uint16_t x_points[num_points];
  uint16_t y_points[num_points];

  // Iterate over the table and print the numbers
  for (size_t i = 0; i < num_points; i++) {
    // Push the index onto the stack
    lua_pushnumber(L, i + 1);

    // Get the value at the given index in the table
    lua_gettable(L, 2);

    // Get the number
    x_points[i] = lua_tonumber(L, -1);

    // printf("x[%d]=%d", i, x_points[i]);
    //  Pop the value off the stack
    lua_pop(L, 1);
  }

  // Iterate over the table and print the numbers
  for (size_t i = 0; i < num_points; i++) {
    // Push the index onto the stack
    lua_pushnumber(L, i + 1);

    // Get the value at the given index in the table
    lua_gettable(L, 3);

    // Get the number
    y_points[i] = lua_tonumber(L, -1);
    // printf("x[%d]=%d", i, y_points[i]);

    // Pop the value off the stack
    lua_pop(L, 1);
  }

  int r = 0, g = 0, b = 0; // Default color: black
  if (lua_gettop(L) >= 4) {
    luaL_checktype(L, 4, LUA_TTABLE);
    lua_rawgeti(L, 4, 1);
    r = luaL_checknumber(L, -1);
    lua_rawgeti(L, 4, 2);
    g = luaL_checknumber(L, -1);
    lua_rawgeti(L, 4, 3);
    b = luaL_checknumber(L, -1);
  }

  grid_gui_draw_polygon_filled(gui, x_points, y_points, num_points, grid_gui_color_from_rgb(r, g, b));

  return 0; // Number of return values
}

// Function to draw text
int l_grid_gui_draw_text(lua_State* L) {

  int screen_index = luaL_checknumber(L, 1);

  struct grid_gui_model* gui = &grid_gui_states[screen_index];

  const char* text = luaL_checkstring(L, 2);
  int x = luaL_checknumber(L, 3);
  int y = luaL_checknumber(L, 4);
  // Optional font_size parameter
  int font_size = 12; // Default font size
  if (lua_gettop(L) >= 5) {
    font_size = luaL_checknumber(L, 5);
  }
  // Optional color parameter
  int r = 0, g = 0, b = 0; // Default color: black
  if (lua_gettop(L) >= 6) {
    luaL_checktype(L, 6, LUA_TTABLE);
    lua_rawgeti(L, 6, 1);
    r = luaL_checknumber(L, -1);
    lua_rawgeti(L, 6, 2);
    g = luaL_checknumber(L, -1);
    lua_rawgeti(L, 6, 3);
    b = luaL_checknumber(L, -1);
  }
  // Draw the text at (x, y) with optional font size and color
  // grid_platform_printf("Drawing text: \"%s\" at (%d, %d) with font size %d and color R=%d, G=%d, B=%d\n", text, x, y, font_size, r, g, b);
  int cursor = 0;

  if (grid_font_state.initialized) {
    grid_font_draw_string(&grid_font_state, gui, x, y, font_size, text, &cursor, grid_gui_color_from_rgb(r, g, b));
  } else {
    // grid_platform_printf("NOT INITIALIZED\n");
  }

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
