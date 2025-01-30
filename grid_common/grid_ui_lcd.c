#include "grid_ui_lcd.h"

#include "grid_ain.h"
#include "grid_lua_api.h"
#include "grid_protocol.h"
#include "grid_ui.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern uint8_t grid_platform_get_adc_bit_depth();

extern void grid_platform_printf(char const* fmt, ...);

const char grid_ui_lcd_init_actionstring[] = GRID_ACTIONSTRING_LCD_INIT;

void grid_ui_element_lcd_init(struct grid_ui_element* ele) {

  ele->type = GRID_PARAMETER_ELEMENT_LCD;

  ele->event_list_length = 1;

  ele->event_list = malloc(ele->event_list_length * sizeof(struct grid_ui_event));
  grid_ui_event_init(ele, 0, GRID_PARAMETER_EVENT_INIT, GRID_LUA_FNC_A_INIT_short, grid_ui_lcd_init_actionstring); // Element Initialization Event

  ele->template_initializer = &grid_ui_element_lcd_template_parameter_init;
  ele->template_parameter_list_length = GRID_LUA_FNC_L_LIST_length;

  ele->event_clear_cb = NULL;
  ele->page_change_cb = NULL;
}

void grid_ui_element_lcd_template_parameter_init(struct grid_ui_template_buffer* buf) {

  // printf("template parameter init\r\n");

  uint8_t element_index = buf->parent->index;
  int32_t* template_parameter_list = buf->template_parameter_list;

  template_parameter_list[GRID_LUA_FNC_L_ELEMENT_INDEX_index] = element_index;
  template_parameter_list[GRID_LUA_FNC_L_SCREEN_INDEX_index] = element_index % 2; // TODO:implement proper logic here
  template_parameter_list[GRID_LUA_FNC_L_SCREEN_WIDTH_index] = 320;
  template_parameter_list[GRID_LUA_FNC_L_SCREEN_HEIGHT_index] = 240;
}
