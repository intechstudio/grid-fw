#ifndef GRID_UI_POTMETER_H
#define GRID_UI_POTMETER_H

#include <stdint.h>

#include "grid_protocol.h"
#include "grid_ui.h"

void grid_ui_element_potmeter_init(struct grid_ui_element* ele);
void grid_ui_element_potmeter_template_parameter_init(struct grid_ui_template_buffer* buf);

void grid_ui_element_potmeter_event_clear_cb(struct grid_ui_event* eve);
void grid_ui_element_potmeter_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new);

void grid_ui_potmeter_store_input(struct grid_ui_element* ele, uint8_t input_channel, uint64_t* last_real_time, uint16_t value, uint8_t adc_bit_depth);

// ========================= POTMETER =========================== //

// clang-format off

#define GRID_LUA_P_TYPE "Potmeter"

extern const luaL_Reg GRID_LUA_P_INDEX_META[];

#define GRID_LUA_P_META_init \
  GRID_LUA_P_TYPE " = { __index = {" \
  \
  "type = 'potmeter', "\
  \
  "post_init_cb = function (self) " \
  "self:"GRID_LUA_FNC_A_INIT_short"() " \
  "self:"GRID_LUA_FNC_A_POTMETER_short"() " \
  "end," \
  \
  GRID_LUA_FNC_ASSIGN_META_PAR1_RET("gen", GRID_LUA_FNC_G_ELEMENTNAME_short) "," \
  \
  "}}"

#define GRID_ACTIONSTRING_POTMETER_INIT "<?lua --[[@cb]] --[[Potmeter Init]] ?>"

#define GRID_ACTIONSTRING_POTMETER_POTMETER \
  "<?lua --[[@spc]] self:pmo(7) self:pmi(0)  self:pma(127)" \
  "--[[@sglc]] self:glc(-1,{{-1,-1,-1,1}}) self:glp(-1,-1)" \
  "--[[@gms]] self:gms(-1,-1,-1,-1) ?>"

// clang-format on

#endif /* GRID_UI_POTMETER_H */
