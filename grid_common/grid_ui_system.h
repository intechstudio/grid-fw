#ifndef GRID_UI_SYSTEM_H
#define GRID_UI_SYSTEM_H

#include <stdint.h>

#include "grid_protocol.h"
#include "grid_ui.h"

void grid_ui_element_system_init(struct grid_ui_element* ele);

void grid_ui_element_system_template_parameter_init(struct grid_ui_template_buffer* buf);

// clang-format off

#define GRID_LUA_S_TYPE "System"

extern const luaL_Reg GRID_LUA_S_INDEX_META[];

#define GRID_LUA_S_META_init \
  GRID_LUA_S_TYPE " = { __index = {" \
  \
  "type = 'system', "\
  \
  "post_init_cb = function (self) " \
  "self:"GRID_LUA_FNC_A_INIT_short"() " \
  "end," \
  \
  GRID_LUA_FNC_ASSIGN_META_PAR1_RET("gen", GRID_LUA_FNC_G_ELEMENTNAME_short) "," \
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
