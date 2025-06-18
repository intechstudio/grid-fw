#ifndef GRID_UI_BUTTON_H
#define GRID_UI_BUTTON_H

#include <stdint.h>

#include "grid_protocol.h"
#include "grid_ui.h"

struct grid_ui_button_state {
  uint64_t last_real_time;
  double threshold;
  double hysteresis;
  uint32_t full_range;
  uint16_t min_value;
  uint16_t max_value;
  uint16_t trig_lo;
  uint16_t trig_hi;
  uint16_t min_range;
  uint16_t prev_in;
  uint16_t curr_in;
  uint16_t prev_out;
  uint16_t curr_out;
  uint64_t prev_time;
  uint64_t curr_time;
};

void grid_ui_button_state_init(struct grid_ui_button_state* state, uint8_t adc_bit_depth, double threshold, double hysteresis);

void grid_ui_element_button_init(struct grid_ui_element* ele);
void grid_ui_element_button_template_parameter_init(struct grid_ui_template_buffer* buf);

void grid_ui_element_button_event_clear_cb(struct grid_ui_event* eve);
void grid_ui_element_button_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new);

void grid_ui_button_update_trigger(struct grid_ui_element* ele, uint64_t* button_last_real_time, uint8_t old_button_value, uint8_t new_button_value);
void grid_ui_button_store_input(struct grid_ui_element* ele, struct grid_ui_button_state* state, uint16_t value, uint8_t adc_bit_depth);

// ========================= BUTTON =========================== //

// Button init function
// clang-format off
#define GRID_LUA_B_META_init                                                                                                                                                                           \
  "button_meta = { __index = {" \
   \
  "type = 'button', "\
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_B_ELEMENT_INDEX_short, GRID_LUA_FNC_B_ELEMENT_INDEX_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_B_LED_INDEX_short, GRID_LUA_FNC_B_LED_INDEX_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_B_BUTTON_VALUE_short, GRID_LUA_FNC_B_BUTTON_VALUE_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_B_BUTTON_MIN_short, GRID_LUA_FNC_B_BUTTON_MIN_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_B_BUTTON_MAX_short, GRID_LUA_FNC_B_BUTTON_MAX_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_B_BUTTON_MODE_short, GRID_LUA_FNC_B_BUTTON_MODE_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_B_BUTTON_ELAPSED_short, GRID_LUA_FNC_B_BUTTON_ELAPSED_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_B_BUTTON_STATE_short, GRID_LUA_FNC_B_BUTTON_STATE_index) "," \
  \
  GRID_LUA_FNC_ASSIGN_META_UNDEF(GRID_LUA_FNC_A_INIT_short) "," \
  GRID_LUA_FNC_ASSIGN_META_UNDEF(GRID_LUA_FNC_A_BUTTON_short) "," \
  GRID_LUA_FNC_ASSIGN_META_UNDEF(GRID_LUA_FNC_A_TIMER_short) "," \
  "post_init_cb = function (self) self:GRID_LUA_FNC_A_BUTTON_short() end," \
  \
  GRID_LUA_FNC_ASSIGN_META_PAR1("gtt", GRID_LUA_FNC_G_TIMER_START_short) "," \
  GRID_LUA_FNC_ASSIGN_META_PAR0("gtp", GRID_LUA_FNC_G_TIMER_STOP_short) "," \
  GRID_LUA_FNC_ASSIGN_META_PAR1("get", GRID_LUA_FNC_G_EVENT_TRIGGER_short) "," \
  GRID_LUA_FNC_ASSIGN_META_PAR1_RET("gen", GRID_LUA_FNC_G_ELEMENTNAME_short) "," \
  \
  GRID_LUA_FNC_B_BUTTON_STEP_short " =function (self) " \
  "local steps, min, max, value = self:" GRID_LUA_FNC_B_BUTTON_MODE_short "(), self:" GRID_LUA_FNC_B_BUTTON_MIN_short "(), self:" GRID_LUA_FNC_B_BUTTON_MAX_short \
  "(), self:" GRID_LUA_FNC_B_BUTTON_VALUE_short "() " \
  "if steps == 0 then return false end " \
  "return value // ((max - min) // steps) " \
  "end," \
  \
  "}}"
// clang-format on

#define GRID_ACTIONSTRING_BUTTON_INIT                                                                                                                                                                  \
  "<?lua --[[@l]] local "                                                                                                                                                                              \
  "num,val,red,gre,blu=self:ind(),self:bva(),glr(),glg(),glb()--[[@glc]] "                                                                                                                             \
  "glc(num,1,red,gre,blu)--[[@glp]] glp(num,1,val) ?>"

#define GRID_ACTIONSTRING_BUTTON_BUTTON                                                                                                                                                                \
  "<?lua --[[@l]] local "                                                                                                                                                                              \
  "num,val,ch,note=self:ind(),self:bva(),(gmy()*4+gpc())%16,(32+gmx()*16+"                                                                                                                             \
  "self:ind())%128--[[@gms]] gms(ch,144,note,val)--[[@glp]] glp(num,1,val) ?>"

#endif /* GRID_UI_BUTTON_H */
