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

#define GRID_LUA_P_META_init                                                                                                                                                                           \
  "potmeter_meta = { __index = {" \
  \
  "type = 'potmeter', "\
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_P_ELEMENT_INDEX_short, GRID_LUA_FNC_P_ELEMENT_INDEX_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_P_LED_INDEX_short, GRID_LUA_FNC_P_LED_INDEX_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_P_POTMETER_VALUE_short, GRID_LUA_FNC_P_POTMETER_VALUE_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_P_POTMETER_MIN_short, GRID_LUA_FNC_P_POTMETER_MIN_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_P_POTMETER_MAX_short, GRID_LUA_FNC_P_POTMETER_MAX_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_P_POTMETER_MODE_short, GRID_LUA_FNC_P_POTMETER_MODE_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_P_POTMETER_ELAPSED_short, GRID_LUA_FNC_P_POTMETER_ELAPSED_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_P_POTMETER_STATE_short, GRID_LUA_FNC_P_POTMETER_STATE_index) "," \
  \
  GRID_LUA_FNC_ASSIGN_META_UNDEF(GRID_LUA_FNC_A_INIT_short) "," \
  GRID_LUA_FNC_ASSIGN_META_UNDEF(GRID_LUA_FNC_A_TIMER_short) "," \
  GRID_LUA_FNC_ASSIGN_META_UNDEF(GRID_LUA_FNC_A_POTMETER_short) "," \
  "post_init_cb = function (self) " \
  "self:"GRID_LUA_FNC_A_INIT_short"() " \
  "self:"GRID_LUA_FNC_A_POTMETER_short"() " \
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

#define GRID_ACTIONSTRING_POTMETER_INIT "<?lua --[[@cb]] --[[Potmeter Init]] ?>"

#define GRID_ACTIONSTRING_POTMETER_POTMETER \
  "<?lua --[[@spc]] self:pmo(7) self:pmi(0)  self:pma(127)" \
  "--[[@sglc]] self:glc(-1,{{-1,-1,-1,1}}) self:glp(-1,-1)" \
  "--[[@gms]] self:gms(-1,-1,-1,-1) ?>"

// clang-format on

#endif /* GRID_UI_POTMETER_H */
