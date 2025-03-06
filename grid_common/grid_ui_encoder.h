#ifndef GRID_UI_ENCODER_H
#define GRID_UI_ENCODER_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "grid_ain.h"
#include "grid_lua_api.h"
#include "grid_platform.h"
#include "grid_protocol.h"
#include "grid_ui.h"
#include "grid_ui_button.h"
#include "grid_ui_system.h"

#define GRID_UI_ENCODER_INIT_SAMPLES 2

struct grid_ui_encoder_state {
  uint64_t encoder_last_real_time;
  uint64_t button_last_real_time;
  uint8_t last_nibble;
  uint8_t detent;
  int8_t encoder_last_leave_dir;
  uint8_t initial_samples;
};

void grid_ui_encoder_state_init(struct grid_ui_encoder_state* state, uint8_t detent);

void grid_ui_element_encoder_init(struct grid_ui_element* ele);
void grid_ui_element_encoder_template_parameter_init(struct grid_ui_template_buffer* buf);

void grid_ui_element_encoder_event_clear_cb(struct grid_ui_event* eve);
void grid_ui_element_encoder_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new);

int16_t grid_ui_encoder_rotation_delta(uint8_t old_value, uint8_t new_value, uint8_t detent, int8_t* dir_lock);
uint8_t grid_ui_encoder_update_trigger(struct grid_ui_element* ele, uint64_t* encoder_last_real_time, int16_t delta);

void grid_ui_encoder_store_input(struct grid_ui_encoder_state* state, uint8_t input_channel, uint8_t new_value);

// ========================= ENCODER =========================== //

#define GRID_LUA_FNC_E_ELEMENT_INDEX_index 0
#define GRID_LUA_FNC_E_ELEMENT_INDEX_short "ind"
#define GRID_LUA_FNC_E_ELEMENT_INDEX_human "element_index"

#define GRID_LUA_FNC_E_BUTTON_NUMBER_index 1
#define GRID_LUA_FNC_E_BUTTON_NUMBER_short "bnu"
#define GRID_LUA_FNC_E_BUTTON_NUMBER_human "button_number"

#define GRID_LUA_FNC_E_BUTTON_VALUE_index 2
#define GRID_LUA_FNC_E_BUTTON_VALUE_short "bva"
#define GRID_LUA_FNC_E_BUTTON_VALUE_human "button_value"

#define GRID_LUA_FNC_E_BUTTON_MIN_index 3
#define GRID_LUA_FNC_E_BUTTON_MIN_short "bmi"
#define GRID_LUA_FNC_E_BUTTON_MIN_human "button_min"

#define GRID_LUA_FNC_E_BUTTON_MAX_index 4
#define GRID_LUA_FNC_E_BUTTON_MAX_short "bma"
#define GRID_LUA_FNC_E_BUTTON_MAX_human "button_max"

#define GRID_LUA_FNC_E_BUTTON_MODE_index 5
#define GRID_LUA_FNC_E_BUTTON_MODE_short "bmo"
#define GRID_LUA_FNC_E_BUTTON_MODE_human "button_mode"

#define GRID_LUA_FNC_E_BUTTON_ELAPSED_index 6
#define GRID_LUA_FNC_E_BUTTON_ELAPSED_short "bel"
#define GRID_LUA_FNC_E_BUTTON_ELAPSED_human "button_elapsed_time"

#define GRID_LUA_FNC_E_BUTTON_STATE_index 7
#define GRID_LUA_FNC_E_BUTTON_STATE_short "bst"
#define GRID_LUA_FNC_E_BUTTON_STATE_human "button_state"

#define GRID_LUA_FNC_E_ENCODER_NUMBER_index 8
#define GRID_LUA_FNC_E_ENCODER_NUMBER_short "enu"
#define GRID_LUA_FNC_E_ENCODER_NUMBER_human "encoder_number"

#define GRID_LUA_FNC_E_ENCODER_VALUE_index 9
#define GRID_LUA_FNC_E_ENCODER_VALUE_short "eva"
#define GRID_LUA_FNC_E_ENCODER_VALUE_human "encoder_value"

#define GRID_LUA_FNC_E_ENCODER_MIN_index 10
#define GRID_LUA_FNC_E_ENCODER_MIN_short "emi"
#define GRID_LUA_FNC_E_ENCODER_MIN_human "encoder_min"

#define GRID_LUA_FNC_E_ENCODER_MAX_index 11
#define GRID_LUA_FNC_E_ENCODER_MAX_short "ema"
#define GRID_LUA_FNC_E_ENCODER_MAX_human "encoder_max"

#define GRID_LUA_FNC_E_ENCODER_MODE_index 12
#define GRID_LUA_FNC_E_ENCODER_MODE_short "emo"
#define GRID_LUA_FNC_E_ENCODER_MODE_human "encoder_mode"

#define GRID_LUA_FNC_E_ENCODER_ELAPSED_index 13
#define GRID_LUA_FNC_E_ENCODER_ELAPSED_short "eel"
#define GRID_LUA_FNC_E_ENCODER_ELAPSED_human "encoder_elapsed_time"

#define GRID_LUA_FNC_E_ENCODER_STATE_index 14
#define GRID_LUA_FNC_E_ENCODER_STATE_short "est"
#define GRID_LUA_FNC_E_ENCODER_STATE_human "encoder_state"

#define GRID_LUA_FNC_E_ENCODER_VELOCITY_index 15
#define GRID_LUA_FNC_E_ENCODER_VELOCITY_short "ev0"
#define GRID_LUA_FNC_E_ENCODER_VELOCITY_human "encoder_velocity"

#define GRID_LUA_FNC_E_ENCODER_SENSITIVITY_index 16
#define GRID_LUA_FNC_E_ENCODER_SENSITIVITY_short "ese"
#define GRID_LUA_FNC_E_ENCODER_SENSITIVITY_human "encoder_sensitivity"

#define GRID_LUA_FNC_E_BUTTON_STEP_short "bstp"
#define GRID_LUA_FNC_E_BUTTON_STEP_human "button_step"

#define GRID_LUA_FNC_E_LED_COLOR_short "elc"
#define GRID_LUA_FNC_E_LED_COLOR_human "led_color"

// Encoder parameters
#define GRID_LUA_FNC_E_LIST_length 17

// Encoder init function
// clang-format off
#define GRID_LUA_E_META_init                                                                                                                                                                           \
  "encoder_meta = { __index = {" \
  \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_E_ELEMENT_INDEX_short, GRID_LUA_FNC_E_ELEMENT_INDEX_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_E_BUTTON_NUMBER_short, GRID_LUA_FNC_E_BUTTON_NUMBER_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_E_BUTTON_VALUE_short, GRID_LUA_FNC_E_BUTTON_VALUE_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_E_BUTTON_MIN_short, GRID_LUA_FNC_E_BUTTON_MIN_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_E_BUTTON_MAX_short, GRID_LUA_FNC_E_BUTTON_MAX_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_E_BUTTON_MODE_short, GRID_LUA_FNC_E_BUTTON_MODE_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_E_BUTTON_ELAPSED_short, GRID_LUA_FNC_E_BUTTON_ELAPSED_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_E_BUTTON_STATE_short, GRID_LUA_FNC_E_BUTTON_STATE_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_E_ENCODER_NUMBER_short, GRID_LUA_FNC_E_ENCODER_NUMBER_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_E_ENCODER_VALUE_short, GRID_LUA_FNC_E_ENCODER_VALUE_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_E_ENCODER_MIN_short, GRID_LUA_FNC_E_ENCODER_MIN_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_E_ENCODER_MAX_short, GRID_LUA_FNC_E_ENCODER_MAX_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_E_ENCODER_MODE_short, GRID_LUA_FNC_E_ENCODER_MODE_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_E_ENCODER_ELAPSED_short, GRID_LUA_FNC_E_ENCODER_ELAPSED_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_E_ENCODER_STATE_short, GRID_LUA_FNC_E_ENCODER_STATE_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_E_ENCODER_VELOCITY_short, GRID_LUA_FNC_E_ENCODER_VELOCITY_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_E_ENCODER_SENSITIVITY_short, GRID_LUA_FNC_E_ENCODER_SENSITIVITY_index) "," \
  \
  GRID_LUA_FNC_ASSIGN_META_UNDEF(GRID_LUA_FNC_A_INIT_short) "," \
  GRID_LUA_FNC_ASSIGN_META_UNDEF(GRID_LUA_FNC_A_BUTTON_short) "," \
  GRID_LUA_FNC_ASSIGN_META_UNDEF(GRID_LUA_FNC_A_TIMER_short) "," \
  GRID_LUA_FNC_ASSIGN_META_UNDEF(GRID_LUA_FNC_A_ENCODER_short) "," \
  \
  GRID_LUA_FNC_ASSIGN_META_PAR1("gtt", GRID_LUA_FNC_G_TIMER_START_short) "," \
  GRID_LUA_FNC_ASSIGN_META_PAR0("gtp", GRID_LUA_FNC_G_TIMER_STOP_short) "," \
  GRID_LUA_FNC_ASSIGN_META_PAR1("get", GRID_LUA_FNC_G_EVENT_TRIGGER_short) "," \
  GRID_LUA_FNC_ASSIGN_META_PAR1_RET("gen", GRID_LUA_FNC_G_ELEMENTNAME_short) "," \
  \
  GRID_LUA_FNC_E_BUTTON_STEP_short " =function (self) " \
  "local steps, min, max, value = self:" GRID_LUA_FNC_B_BUTTON_MODE_short "(), self:" GRID_LUA_FNC_B_BUTTON_MIN_short "(), self:" GRID_LUA_FNC_B_BUTTON_MAX_short \
  "(), self:" GRID_LUA_FNC_B_BUTTON_VALUE_short "() " \
  "if steps == 0 then return false end " \
  "return value // ((max - min) // steps) " \
  "end," \
  \
  GRID_LUA_FNC_E_LED_COLOR_short " =function (self, ...) " \
  GRID_LUA_FNC_G_LED_COLOR_short "(self:" GRID_LUA_FNC_E_ELEMENT_INDEX_short "(), ...) " \
  "end," \
  "}}"
// clang-format on

#define GRID_ACTIONSTRING_ENCODER_INIT                                                                                                                                                                 \
  "<?lua --[[@l]] local "                                                                                                                                                                              \
  "num,bval,eval,red,gre,blu=self:ind(),self:bva(),self:eva(),glr(),glg(),"                                                                                                                            \
  "glb()--[[@glc]] glc(num,1,red,gre,blu)--[[@glc]] "                                                                                                                                                  \
  "glc(num,2,red,gre,blu)--[[@glp]] glp(num,1,bval)--[[@glp]] "                                                                                                                                        \
  "glp(num,2,eval) ?>"

#define GRID_ACTIONSTRING_ENCODER_ENCODER                                                                                                                                                              \
  "<?lua --[[@l]] local "                                                                                                                                                                              \
  "num,val,ch,cc=self:ind(),self:eva(),(gmy()*4+gpc())%16,(32+gmx()*16+self:"                                                                                                                          \
  "ind())%128--[[@gms]] gms(ch,176,cc,val)--[[@glp]] glp(num,2,val) ?>"

#endif /* GRID_UI_ENCODER_H */
