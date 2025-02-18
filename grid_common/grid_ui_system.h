#pragma once

#ifndef GRID_UI_SYSTEM_H_INCLUDED
#define GRID_UI_SYSTEM_H_INCLUDED

#include "grid_ui.h"
#include <stdint.h>

void grid_ui_element_system_init(struct grid_ui_element* ele);

void grid_ui_element_system_template_parameter_init(struct grid_ui_template_buffer* buf);

#define GRID_LUA_FNC_S_ELEMENT_INDEX_index 0
#define GRID_LUA_FNC_S_ELEMENT_INDEX_short "ind"
#define GRID_LUA_FNC_S_ELEMENT_INDEX_human "element_index"

#define GRID_LUA_FNC_S_LIST_length 1

// System init function
// clang-format off
#define GRID_LUA_SYS_META_init                                                                                                                                                                         \
  "system_meta = { __index = {" \
  \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_S_ELEMENT_INDEX_short, GRID_LUA_FNC_S_ELEMENT_INDEX_index) "," \
  \
  GRID_LUA_FNC_ASSIGN_META_UNDEF(GRID_LUA_FNC_A_INIT_short) "," \
  GRID_LUA_FNC_ASSIGN_META_UNDEF(GRID_LUA_FNC_A_TIMER_short) "," \
  GRID_LUA_FNC_ASSIGN_META_UNDEF(GRID_LUA_FNC_A_MAPMODE_short) "," \
  GRID_LUA_FNC_ASSIGN_META_UNDEF(GRID_LUA_FNC_A_MIDIRX_short) "," \
  \
  GRID_LUA_FNC_ASSIGN_META_PAR1("gtt", GRID_LUA_FNC_G_TIMER_START_short) "," \
  GRID_LUA_FNC_ASSIGN_META_PAR0("gtp", GRID_LUA_FNC_G_TIMER_STOP_short) "," \
  GRID_LUA_FNC_ASSIGN_META_PAR1("get", GRID_LUA_FNC_G_EVENT_TRIGGER_short) "," \
  \
  "}}"
// clang-format on

#define GRID_ACTIONSTRING_SYSTEM_INIT "<?lua --[[@cb]] --[[page init]] ?>"
#define GRID_ACTIONSTRING_SYSTEM_MAPMODE "<?lua --[[@cb]] gpl(gpn()) ?>"
#define GRID_ACTIONSTRING_SYSTEM_MIDIRX                                                                                                                                                                \
  "<?lua --[[@l]] local "                                                                                                                                                                              \
  "ch,cmd,param1,param2=midi.ch,midi.cmd,midi.p1,midi.p2 ?>"

#define GRID_ACTIONSTRING_SYSTEM_TIMER "<?lua --[[@cb]] print('tick') ?>"

#endif
