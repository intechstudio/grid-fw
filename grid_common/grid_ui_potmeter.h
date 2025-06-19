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

// Potmeter init function
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
  \
  "}}"
// clang-format on

#define GRID_ACTIONSTRING_POTMETER_INIT                                                                                                                                                                \
  "<?lua --[[@l]] local "                                                                                                                                                                              \
  "num,val,red,gre,blu=self:ind(),self:pva(),glr(),glg(),glb()--[[@glc]] "                                                                                                                             \
  "glc(num,1,red,gre,blu)--[[@glp]] glp(num,1,val) ?>"

// new dynamic midi based on x y and activepage
#define GRID_ACTIONSTRING_POTMETER_POTMETER                                                                                                                                                            \
  "<?lua --[[@l]] local "                                                                                                                                                                              \
  "num,val,ch,cc=self:ind(),self:pva(),(gmy()*4+gpc())%16,(32+gmx()*16+self:"                                                                                                                          \
  "ind())%128--[[@gms]] gms(ch,176,cc,val)--[[@glp]] glp(num,1,val) ?>"

#endif /* GRID_UI_POTMETER_H */
