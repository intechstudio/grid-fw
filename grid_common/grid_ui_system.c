#include "grid_ui_system.h"

#include <stdint.h>
#include "grid_ui.h"
#include "grid_ain.h"
#include "grid_lua_api.h"
#include "grid_protocol.h"
#include <stdlib.h>
#include <string.h>

extern uint8_t grid_platform_get_adc_bit_depth();

extern void grid_platform_printf(char const* fmt, ...);


const char grid_ui_system_init_actionstring[] = GRID_ACTIONSTRING_PAGE_INIT;
const char grid_ui_system_mapmodechange_actionstring[] = GRID_ACTIONSTRING_MAPMODE_CHANGE;
const char grid_ui_system_midirx_actionstring[] = GRID_ACTIONSTRING_MIDIRX;
const char grid_ui_system_timer_actionstring[] = GRID_ACTIONSTRING_TIMER;

void grid_ui_element_system_init(struct grid_ui_element* ele){
   
  ele->type = GRID_UI_ELEMENT_SYSTEM;

  ele->event_list_length = 4;

  ele->event_list = malloc(ele->event_list_length * sizeof(struct grid_ui_event));
  grid_ui_event_init(ele, 0, GRID_UI_EVENT_INIT, GRID_LUA_FNC_ACTION_INIT_short, grid_ui_system_init_actionstring);                   // Element Initialization Event
  grid_ui_event_init(ele, 1, GRID_UI_EVENT_MAPMODE_CHANGE, GRID_LUA_FNC_ACTION_MAPMODE_short, grid_ui_system_mapmodechange_actionstring); // Mapmode change
  grid_ui_event_init(ele, 2, GRID_UI_EVENT_MIDIRX, GRID_LUA_FNC_ACTION_MIDIRX_short, grid_ui_system_midirx_actionstring);         // Midi Receive
  grid_ui_event_init(ele, 3, GRID_UI_EVENT_TIMER, GRID_LUA_FNC_ACTION_TIMER_short, grid_ui_system_timer_actionstring);

  ele->template_initializer = NULL;
  ele->template_parameter_list_length = 0;

  ele->event_clear_cb = NULL;
  ele->page_change_cb = NULL;

}
