#pragma once

#ifndef GRID_UI_LCD_H_INCLUDED
#define GRID_UI_LCD_H_INCLUDED

#include "grid_ui.h"
#include <stdint.h>

void grid_ui_element_lcd_init(struct grid_ui_element* ele);

void grid_ui_element_lcd_template_parameter_init(struct grid_ui_template_buffer* buf);

#define GRID_LUA_FNC_L_ELEMENT_INDEX_index 0
#define GRID_LUA_FNC_L_ELEMENT_INDEX_helper "0"
#define GRID_LUA_FNC_L_ELEMENT_INDEX_short "ind"
#define GRID_LUA_FNC_L_ELEMENT_INDEX_human "element_index"

#define GRID_LUA_FNC_L_LIST_length 1

// LCD init function
#define GRID_LUA_LCD_META_init                                                                                                                                                                         \
  "lcd_meta = { __index = { \
  \
  " GRID_LUA_FNC_L_ELEMENT_INDEX_short "=function (self,a) return "                                                                                                                                    \
  "gtv(self.index, " GRID_LUA_FNC_L_ELEMENT_INDEX_helper ", a) end, \
  \
  " GRID_LUA_FNC_A_INIT_short " = function (self) print('undefined action') end, \
  \
  gtt = function (self,a) " GRID_LUA_FNC_G_TIMER_START_short "(self.index,a) end,\
  gtp = function (self) " GRID_LUA_FNC_G_TIMER_STOP_short "(self.index) end,\
  get = function (self,a) " GRID_LUA_FNC_G_EVENT_TRIGGER_short "(self.index,a) end\
    }}"

#define GRID_ACTIONSTRING_LCD_INIT "<?lua --[[@cb]] --[[lcd init]] ?>"

#endif
