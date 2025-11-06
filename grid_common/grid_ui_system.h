#ifndef GRID_UI_SYSTEM_H
#define GRID_UI_SYSTEM_H

#include <stdint.h>

#include "grid_protocol.h"
#include "grid_ui.h"

void grid_ui_element_system_init(struct grid_ui_element* ele);

void grid_ui_element_system_template_parameter_init(struct grid_ui_template_buffer* buf);

// System init function
// clang-format off
#define GRID_LUA_SYS_META_init                                                                                                                                                                         \
  "system_meta = { __index = {" \
  \
  "type = 'system', "\
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_S_ELEMENT_INDEX_short, GRID_LUA_FNC_S_ELEMENT_INDEX_index) "," \
  \
  GRID_LUA_FNC_ASSIGN_META_UNDEF(GRID_LUA_FNC_A_INIT_short) "," \
  GRID_LUA_FNC_ASSIGN_META_UNDEF(GRID_LUA_FNC_A_TIMER_short) "," \
  GRID_LUA_FNC_ASSIGN_META_UNDEF(GRID_LUA_FNC_A_MAPMODE_short) "," \
  GRID_LUA_FNC_ASSIGN_META_UNDEF(GRID_LUA_FNC_A_MIDIRX_short) "," \
  "post_init_cb = function (self) " \
  "self:"GRID_LUA_FNC_A_INIT_short"() " \
  "end," \
  \
  GRID_LUA_FNC_ASSIGN_META_PAR1("gtt", GRID_LUA_FNC_G_TIMER_START_short) "," \
  GRID_LUA_FNC_ASSIGN_META_PAR0("gtp", GRID_LUA_FNC_G_TIMER_STOP_short) "," \
  GRID_LUA_FNC_ASSIGN_META_PAR1("get", GRID_LUA_FNC_G_EVENT_TRIGGER_short) "," \
  GRID_LUA_FNC_ASSIGN_META_PAR1_RET("gen", GRID_LUA_FNC_G_ELEMENTNAME_short) "," \
  GRID_LUA_FNC_ASSIGN_META_PAR1("gsen", GRID_LUA_FNC_G_ELEMENTNAME_SET_short) "," \
  GRID_LUA_FNC_ASSIGN_META_PAR0_RET("ggen", GRID_LUA_FNC_G_ELEMENTNAME_GET_short) "," \
  \
  "}}"

#define GRID_ACTIONSTRING_SYSTEM_INIT "<?lua --[[@cb]] --[[page init]] ?>"

#define GRID_ACTIONSTRING_SYSTEM_MAPMODE "<?lua --[[@cb]] gpl(gpn()) ?>"

#define GRID_ACTIONSTRING_SYSTEM_MIDIRX \
  "<?lua --[[@l]] local ch,cmd,param1,param2=" \
  "midi.ch,midi.cmd,midi.p1,midi.p2 ?>"

#define GRID_ACTIONSTRING_SYSTEM_TIMER "<?lua --[[@cb]] print('tick') ?>"

// clang-format on

#endif /* GRID_UI_SYSTEM_H */
