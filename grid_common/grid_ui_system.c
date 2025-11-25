#include "grid_ui_system.h"

#include <stdlib.h>
#include <string.h>

#include "grid_ain.h"
#include "grid_lua_api.h"

const luaL_Reg GRID_LUA_S_INDEX_META[] = {{GRID_LUA_FNC_S_ELEMENT_INDEX_short, XAFTERX(GRID_LUA_FNC_GTV_NAME, GRID_LUA_FNC_S_ELEMENT_INDEX_index)},
                                          {GRID_LUA_FNC_G_TIMER_START_short, XAFTERX(GRID_LUA_FNC_META_PAR1_NAME, gtt)},
                                          {GRID_LUA_FNC_G_TIMER_STOP_short, XAFTERX(GRID_LUA_FNC_META_PAR0_NAME, gtp)},
                                          {GRID_LUA_FNC_G_EVENT_TRIGGER_short, XAFTERX(GRID_LUA_FNC_META_PAR0_NAME, get)},
                                          {GRID_LUA_FNC_G_ELEMENTNAME_SET_short, XAFTERX(GRID_LUA_FNC_META_PAR1_NAME, gsen)},
                                          {GRID_LUA_FNC_G_ELEMENTNAME_GET_short, XAFTERX(GRID_LUA_FNC_META_PAR0_NAME, ggen)},
                                          {NULL, NULL}};

const char grid_ui_system_init_actionstring[] = GRID_ACTIONSTRING_SYSTEM_INIT;
const char grid_ui_system_mapmodechange_actionstring[] = GRID_ACTIONSTRING_SYSTEM_MAPMODE;
const char grid_ui_system_midirx_actionstring[] = GRID_ACTIONSTRING_SYSTEM_MIDIRX;
const char grid_ui_system_timer_actionstring[] = GRID_ACTIONSTRING_SYSTEM_TIMER;

void grid_ui_element_system_init(struct grid_ui_element* ele) {

  ele->type = GRID_PARAMETER_ELEMENT_SYSTEM;

  ele->event_list_length = 4;

  ele->event_list = malloc(ele->event_list_length * sizeof(struct grid_ui_event));
  grid_ui_event_init(ele, 0, GRID_PARAMETER_EVENT_INIT, GRID_LUA_FNC_A_INIT_short, grid_ui_system_init_actionstring);                // Element Initialization Event
  grid_ui_event_init(ele, 1, GRID_PARAMETER_EVENT_MAPMODE, GRID_LUA_FNC_A_MAPMODE_short, grid_ui_system_mapmodechange_actionstring); // Mapmode change
  grid_ui_event_init(ele, 2, GRID_PARAMETER_EVENT_MIDIRX, GRID_LUA_FNC_A_MIDIRX_short, grid_ui_system_midirx_actionstring);          // Midi Receive
  grid_ui_event_init(ele, 3, GRID_PARAMETER_EVENT_TIMER, GRID_LUA_FNC_A_TIMER_short, grid_ui_system_timer_actionstring);

  ele->template_initializer = &grid_ui_element_system_template_parameter_init;
  ele->template_parameter_list_length = GRID_LUA_FNC_S_LIST_length;

  ele->event_clear_cb = NULL;
  ele->page_change_cb = NULL;
}

void grid_ui_element_system_template_parameter_init(struct grid_ui_template_buffer* buf) {

  // printf("template parameter init\r\n");

  uint8_t element_index = buf->parent->index;
  int32_t* template_parameter_list = buf->template_parameter_list;

  template_parameter_list[GRID_LUA_FNC_S_ELEMENT_INDEX_index] = element_index;
}
