#include "grid_ui_lcd.h"

#include <stdlib.h>
#include <string.h>

#include "grid_ain.h"
#include "grid_lua_api.h"

const luaL_Reg GRID_LUA_L_INDEX_META[] = {{GRID_LUA_FNC_L_ELEMENT_INDEX_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_L_ELEMENT_INDEX_index)},
                                          {GRID_LUA_FNC_L_SCREEN_INDEX_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_L_SCREEN_INDEX_index)},
                                          {GRID_LUA_FNC_L_SCREEN_WIDTH_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_L_SCREEN_WIDTH_index)},
                                          {GRID_LUA_FNC_L_SCREEN_HEIGHT_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_L_SCREEN_HEIGHT_index)},
                                          {GRID_LUA_FNC_G_TIMER_START_short, XAFTERX(GRID_LUA_FNC_META_PAR1_NAME, gtt)},
                                          {GRID_LUA_FNC_G_TIMER_STOP_short, XAFTERX(GRID_LUA_FNC_META_PAR0_NAME, gtp)},
                                          {GRID_LUA_FNC_G_EVENT_TRIGGER_short, XAFTERX(GRID_LUA_FNC_META_PAR0_NAME, get)},
                                          {GRID_LUA_FNC_G_ELEMENTNAME_SET_short, XAFTERX(GRID_LUA_FNC_META_PAR1_NAME, gsen)},
                                          {GRID_LUA_FNC_G_ELEMENTNAME_GET_short, XAFTERX(GRID_LUA_FNC_META_PAR0_NAME, ggen)},
                                          {NULL, NULL}};

const char grid_ui_lcd_init_actionstring[] = GRID_ACTIONSTRING_LCD_INIT;
const char grid_ui_lcd_draw_actionstring[] = GRID_ACTIONSTRING_LCD_DRAW;

void grid_ui_element_lcd_init(struct grid_ui_element* ele, template_init_t initializer) {

  ele->type = GRID_PARAMETER_ELEMENT_LCD;

  grid_ui_element_malloc_events(ele, 2);

  grid_ui_event_init(ele, 0, GRID_PARAMETER_EVENT_INIT, GRID_LUA_FNC_A_INIT_short, grid_ui_lcd_init_actionstring); // Element Initialization Event
  grid_ui_event_init(ele, 1, GRID_PARAMETER_EVENT_DRAW, GRID_LUA_FNC_A_DRAW_short, grid_ui_lcd_draw_actionstring);

  template_init_t init_default = grid_ui_element_lcd_template_parameter_init;
  template_init_t init_actual = initializer ? initializer : init_default;
  ele->template_initializer = init_actual;
  ele->template_parameter_list_length = GRID_LUA_FNC_L_LIST_length;

  ele->event_clear_cb = NULL;
  ele->page_change_cb = NULL;
}

void grid_ui_element_lcd_template_parameter_init(struct grid_ui_template_buffer* buf) {

  // printf("template parameter init\r\n");

  uint8_t element_index = buf->parent->index;
  int32_t* template_parameter_list = buf->template_parameter_list;

  template_parameter_list[GRID_LUA_FNC_L_ELEMENT_INDEX_index] = element_index;
  template_parameter_list[GRID_LUA_FNC_L_SCREEN_INDEX_index] = 0;
  template_parameter_list[GRID_LUA_FNC_L_SCREEN_WIDTH_index] = 0;
  template_parameter_list[GRID_LUA_FNC_L_SCREEN_HEIGHT_index] = 0;
}
