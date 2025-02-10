#pragma once

#ifndef GRID_UI_BUTTON_H_INCLUDED
#define GRID_UI_BUTTON_H_INCLUDED

#include "grid_ui.h"
#include <stdint.h>

void grid_ui_element_button_init(struct grid_ui_element* ele);
void grid_ui_element_button_template_parameter_init(struct grid_ui_template_buffer* buf);

void grid_ui_element_button_event_clear_cb(struct grid_ui_event* eve);
void grid_ui_element_button_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new);

void grid_ui_button_update_trigger(struct grid_ui_element* ele, uint64_t* button_last_real_time, uint8_t old_button_value, uint8_t new_button_value);
void grid_ui_button_store_input(uint8_t input_channel, uint64_t* last_real_time, uint16_t value, uint8_t adc_bit_depth);

#define GRID_LUA_FNC_B_ELEMENT_INDEX_index 0
#define GRID_LUA_FNC_B_ELEMENT_INDEX_short "ind"
#define GRID_LUA_FNC_B_ELEMENT_INDEX_human "element_index"

#define GRID_LUA_FNC_B_BUTTON_NUMBER_index 1
#define GRID_LUA_FNC_B_BUTTON_NUMBER_short "bnu"
#define GRID_LUA_FNC_B_BUTTON_NUMBER_human "button_number"

#define GRID_LUA_FNC_B_BUTTON_VALUE_index 2
#define GRID_LUA_FNC_B_BUTTON_VALUE_short "bva"
#define GRID_LUA_FNC_B_BUTTON_VALUE_human "button_value"

#define GRID_LUA_FNC_B_BUTTON_MIN_index 3
#define GRID_LUA_FNC_B_BUTTON_MIN_short "bmi"
#define GRID_LUA_FNC_B_BUTTON_MIN_human "button_min"

#define GRID_LUA_FNC_B_BUTTON_MAX_index 4
#define GRID_LUA_FNC_B_BUTTON_MAX_short "bma"
#define GRID_LUA_FNC_B_BUTTON_MAX_human "button_max"

#define GRID_LUA_FNC_B_BUTTON_MODE_index 5
#define GRID_LUA_FNC_B_BUTTON_MODE_short "bmo"
#define GRID_LUA_FNC_B_BUTTON_MODE_human "button_mode"

#define GRID_LUA_FNC_B_BUTTON_ELAPSED_index 6
#define GRID_LUA_FNC_B_BUTTON_ELAPSED_short "bel"
#define GRID_LUA_FNC_B_BUTTON_ELAPSED_human "button_elapsed_time"

#define GRID_LUA_FNC_B_BUTTON_STATE_index 7
#define GRID_LUA_FNC_B_BUTTON_STATE_short "bst"
#define GRID_LUA_FNC_B_BUTTON_STATE_human "button_state"

#define GRID_LUA_FNC_B_BUTTON_STEP_short "bstp"
#define GRID_LUA_FNC_B_BUTTON_STEP_human "button_step"

// Button parameters
#define GRID_LUA_FNC_B_LIST_length 8

// ========================= BUTTON =========================== //

// Button init function
// clang-format off
#define GRID_LUA_B_META_init                                                                                                                                                                           \
  "button_meta = { __index = {" \
   \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_B_ELEMENT_INDEX_short, GRID_LUA_FNC_B_ELEMENT_INDEX_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_B_BUTTON_NUMBER_short, GRID_LUA_FNC_B_BUTTON_NUMBER_index) "," \
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

#endif
