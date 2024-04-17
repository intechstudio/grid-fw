#pragma once

#ifndef GRID_UI_SYSTEM_H_INCLUDED
#define GRID_UI_SYSTEM_H_INCLUDED

#include "grid_ui.h"
#include <stdint.h>

void grid_ui_element_system_init(struct grid_ui_element* ele);

// System init function
#define GRID_LUA_SYS_META_init                                                                                                                                                                         \
  "system_meta = { __index = { \
  \
  " GRID_LUA_FNC_A_INIT_short " = function (self) print('undefined action') end, \
  " GRID_LUA_FNC_A_MAPMODE_short " = function (self) print('undefined action') end,\
  " GRID_LUA_FNC_A_MIDIRX_short " = function (self) print('undefined action') end,\
  " GRID_LUA_FNC_A_TIMER_short " = function (self) print('undefined action') end,\
  \
  gtt = function (self,a) " GRID_LUA_FNC_G_TIMER_START_short "(self.index,a) end,\
  gtp = function (self) " GRID_LUA_FNC_G_TIMER_STOP_short "(self.index) end,\
  get = function (self,a) " GRID_LUA_FNC_G_EVENT_TRIGGER_short "(self.index,a) end\
    }}"

#define GRID_ACTIONSTRING_SYSTEM_INIT "<?lua --[[@cb]] --[[page init]] ?>"
#define GRID_ACTIONSTRING_SYSTEM_MAPMODE "<?lua --[[@cb]] gpl(gpn()) ?>"
#define GRID_ACTIONSTRING_SYSTEM_MIDIRX                                                                                                                                                                \
  "<?lua --[[@l]] local "                                                                                                                                                                              \
  "ch,cmd,param1,param2=midi.ch,midi.cmd,midi.p1,midi.p2 ?>"

#define GRID_ACTIONSTRING_SYSTEM_TIMER "<?lua --[[@cb]] print('tick') ?>"

#endif
