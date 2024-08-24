#pragma once

#ifndef GRID_UI_ENCODER_H_INCLUDED
#define GRID_UI_ENCODER_H_INCLUDED

#include "grid_ui.h"
#include <stdint.h>

void grid_ui_element_encoder_init(struct grid_ui_element* ele);
void grid_ui_element_encoder_template_parameter_init(struct grid_ui_template_buffer* buf);

void grid_ui_element_encoder_event_clear_cb(struct grid_ui_event* eve);
void grid_ui_element_encoder_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new);

int16_t grid_ui_encoder_rotation_delta(uint8_t old_value, uint8_t new_value);
uint8_t grid_ui_encoder_update_trigger(struct grid_ui_element* ele, uint64_t* encoder_last_real_time, int16_t delta);

void grid_ui_encoder_store_input(uint8_t input_channel, uint64_t* encoder_last_real_time, uint64_t* button_last_real_time, uint8_t old_value, uint8_t new_value, uint8_t* phase_change_lock);

// ========================= ENCODER =========================== //

#define GRID_LUA_FNC_E_ELEMENT_INDEX_index 0
#define GRID_LUA_FNC_E_ELEMENT_INDEX_helper "0"
#define GRID_LUA_FNC_E_ELEMENT_INDEX_short "ind"
#define GRID_LUA_FNC_E_ELEMENT_INDEX_human "element_index"

#define GRID_LUA_FNC_E_BUTTON_NUMBER_index 1
#define GRID_LUA_FNC_E_BUTTON_NUMBER_helper "1"
#define GRID_LUA_FNC_E_BUTTON_NUMBER_short "bnu"
#define GRID_LUA_FNC_E_BUTTON_NUMBER_human "button_number"

#define GRID_LUA_FNC_E_BUTTON_VALUE_index 2
#define GRID_LUA_FNC_E_BUTTON_VALUE_helper "2"
#define GRID_LUA_FNC_E_BUTTON_VALUE_short "bva"
#define GRID_LUA_FNC_E_BUTTON_VALUE_human "button_value"

#define GRID_LUA_FNC_E_BUTTON_MIN_index 3
#define GRID_LUA_FNC_E_BUTTON_MIN_helper "3"
#define GRID_LUA_FNC_E_BUTTON_MIN_short "bmi"
#define GRID_LUA_FNC_E_BUTTON_MIN_human "button_min"

#define GRID_LUA_FNC_E_BUTTON_MAX_index 4
#define GRID_LUA_FNC_E_BUTTON_MAX_helper "4"
#define GRID_LUA_FNC_E_BUTTON_MAX_short "bma"
#define GRID_LUA_FNC_E_BUTTON_MAX_human "button_max"

#define GRID_LUA_FNC_E_BUTTON_MODE_index 5
#define GRID_LUA_FNC_E_BUTTON_MODE_helper "5"
#define GRID_LUA_FNC_E_BUTTON_MODE_short "bmo"
#define GRID_LUA_FNC_E_BUTTON_MODE_human "button_mode"

#define GRID_LUA_FNC_E_BUTTON_ELAPSED_index 6
#define GRID_LUA_FNC_E_BUTTON_ELAPSED_helper "6"
#define GRID_LUA_FNC_E_BUTTON_ELAPSED_short "bel"
#define GRID_LUA_FNC_E_BUTTON_ELAPSED_human "button_elapsed_time"

#define GRID_LUA_FNC_E_BUTTON_STATE_index 7
#define GRID_LUA_FNC_E_BUTTON_STATE_helper "7"
#define GRID_LUA_FNC_E_BUTTON_STATE_short "bst"
#define GRID_LUA_FNC_E_BUTTON_STATE_human "button_state"

#define GRID_LUA_FNC_E_ENCODER_NUMBER_index 8
#define GRID_LUA_FNC_E_ENCODER_NUMBER_helper "8"
#define GRID_LUA_FNC_E_ENCODER_NUMBER_short "enu"
#define GRID_LUA_FNC_E_ENCODER_NUMBER_human "encoder_number"

#define GRID_LUA_FNC_E_ENCODER_VALUE_index 9
#define GRID_LUA_FNC_E_ENCODER_VALUE_helper "9"
#define GRID_LUA_FNC_E_ENCODER_VALUE_short "eva"
#define GRID_LUA_FNC_E_ENCODER_VALUE_human "encoder_value"

#define GRID_LUA_FNC_E_ENCODER_MIN_index 10
#define GRID_LUA_FNC_E_ENCODER_MIN_helper "10"
#define GRID_LUA_FNC_E_ENCODER_MIN_short "emi"
#define GRID_LUA_FNC_E_ENCODER_MIN_human "encoder_min"

#define GRID_LUA_FNC_E_ENCODER_MAX_index 11
#define GRID_LUA_FNC_E_ENCODER_MAX_helper "11"
#define GRID_LUA_FNC_E_ENCODER_MAX_short "ema"
#define GRID_LUA_FNC_E_ENCODER_MAX_human "encoder_max"

#define GRID_LUA_FNC_E_ENCODER_MODE_index 12
#define GRID_LUA_FNC_E_ENCODER_MODE_helper "12"
#define GRID_LUA_FNC_E_ENCODER_MODE_short "emo"
#define GRID_LUA_FNC_E_ENCODER_MODE_human "encoder_mode"

#define GRID_LUA_FNC_E_ENCODER_ELAPSED_index 13
#define GRID_LUA_FNC_E_ENCODER_ELAPSED_helper "13"
#define GRID_LUA_FNC_E_ENCODER_ELAPSED_short "eel"
#define GRID_LUA_FNC_E_ENCODER_ELAPSED_human "encoder_elapsed_time"

#define GRID_LUA_FNC_E_ENCODER_STATE_index 14
#define GRID_LUA_FNC_E_ENCODER_STATE_helper "14"
#define GRID_LUA_FNC_E_ENCODER_STATE_short "est"
#define GRID_LUA_FNC_E_ENCODER_STATE_human "encoder_state"

#define GRID_LUA_FNC_E_ENCODER_VELOCITY_index 15
#define GRID_LUA_FNC_E_ENCODER_VELOCITY_helper "15"
#define GRID_LUA_FNC_E_ENCODER_VELOCITY_short "ev0"
#define GRID_LUA_FNC_E_ENCODER_VELOCITY_human "encoder_velocity"

#define GRID_LUA_FNC_E_ENCODER_SENSITIVITY_index 16
#define GRID_LUA_FNC_E_ENCODER_SENSITIVITY_helper "16"
#define GRID_LUA_FNC_E_ENCODER_SENSITIVITY_short "ese"
#define GRID_LUA_FNC_E_ENCODER_SENSITIVITY_human "encoder_sensitivity"

// Encoder parameters
#define GRID_LUA_FNC_E_LIST_length 17

// Encoder init function
#define GRID_LUA_E_META_init                                                                                                                                                                           \
  "encoder_meta = { __index = { \
   \
  " GRID_LUA_FNC_E_ELEMENT_INDEX_short "=function (self,a) return "                                                                                                                                    \
  "gtv(self.index, " GRID_LUA_FNC_E_ELEMENT_INDEX_helper ", a) end, \
  \
  " GRID_LUA_FNC_E_BUTTON_NUMBER_short "=function (self,a) return "                                                                                                                                    \
  "gtv(self.index, " GRID_LUA_FNC_E_BUTTON_NUMBER_helper ", a) end, \
  " GRID_LUA_FNC_E_BUTTON_VALUE_short "=function (self,a) return "                                                                                                                                     \
  "gtv(self.index, " GRID_LUA_FNC_E_BUTTON_VALUE_helper ", a) end, \
  " GRID_LUA_FNC_E_BUTTON_MIN_short "=function (self,a) return "                                                                                                                                       \
  "gtv(self.index, " GRID_LUA_FNC_E_BUTTON_MIN_helper ", a) end, \
  " GRID_LUA_FNC_E_BUTTON_MAX_short "=function (self,a) return "                                                                                                                                       \
  "gtv(self.index, " GRID_LUA_FNC_E_BUTTON_MAX_helper ", a) end, \
  " GRID_LUA_FNC_E_BUTTON_MODE_short "=function (self,a) return "                                                                                                                                      \
  "gtv(self.index, " GRID_LUA_FNC_E_BUTTON_MODE_helper ", a) end, \
  " GRID_LUA_FNC_E_BUTTON_ELAPSED_short "=function (self,a) return "                                                                                                                                   \
  "gtv(self.index, " GRID_LUA_FNC_E_BUTTON_ELAPSED_helper ", a) end, \
  " GRID_LUA_FNC_E_BUTTON_STATE_short "=function (self,a) return "                                                                                                                                     \
  "gtv(self.index, " GRID_LUA_FNC_E_BUTTON_STATE_helper ", a) end, \
  \
  " GRID_LUA_FNC_E_ENCODER_NUMBER_short "=function (self,a) return "                                                                                                                                   \
  "gtv(self.index, " GRID_LUA_FNC_E_ENCODER_NUMBER_helper ", a) end, \
  " GRID_LUA_FNC_E_ENCODER_VALUE_short "=function (self,a) return "                                                                                                                                    \
  "gtv(self.index, " GRID_LUA_FNC_E_ENCODER_VALUE_helper ", a) end, \
  " GRID_LUA_FNC_E_ENCODER_MIN_short "=function (self,a) return "                                                                                                                                      \
  "gtv(self.index, " GRID_LUA_FNC_E_ENCODER_MIN_helper ", a) end, \
  " GRID_LUA_FNC_E_ENCODER_MAX_short "=function (self,a) return "                                                                                                                                      \
  "gtv(self.index, " GRID_LUA_FNC_E_ENCODER_MAX_helper ", a) end, \
  " GRID_LUA_FNC_E_ENCODER_MODE_short "=function (self,a) return "                                                                                                                                     \
  "gtv(self.index, " GRID_LUA_FNC_E_ENCODER_MODE_helper ", a) end, \
  " GRID_LUA_FNC_E_ENCODER_ELAPSED_short "=function (self,a) return "                                                                                                                                  \
  "gtv(self.index, " GRID_LUA_FNC_E_ENCODER_ELAPSED_helper ", a) end, \
  " GRID_LUA_FNC_E_ENCODER_STATE_short "=function (self,a) return "                                                                                                                                    \
  "gtv(self.index, " GRID_LUA_FNC_E_ENCODER_STATE_helper ", a) end, \
  " GRID_LUA_FNC_E_ENCODER_VELOCITY_short "=function (self,a) return "                                                                                                                                 \
  "gtv(self.index, " GRID_LUA_FNC_E_ENCODER_VELOCITY_helper ", a) end, \
  " GRID_LUA_FNC_E_ENCODER_SENSITIVITY_short "=function (self,a) return "                                                                                                                              \
  "gtv(self.index, " GRID_LUA_FNC_E_ENCODER_SENSITIVITY_helper ", a) end, \
  \
  " GRID_LUA_FNC_A_INIT_short " = function (self) print('undefined action') end,\
  " GRID_LUA_FNC_A_ENCODER_short " = function (self) print('undefined action') end,\
  " GRID_LUA_FNC_A_BUTTON_short " = function (self) print('undefined action') end,\
  " GRID_LUA_FNC_A_TIMER_short " = function (self) print('undefined action') end,\
  \
  gtt = function (self,a) " GRID_LUA_FNC_G_TIMER_START_short "(self.index,a) end,\
  gtp = function (self) " GRID_LUA_FNC_G_TIMER_STOP_short "(self.index) end,\
  get = function (self,a) " GRID_LUA_FNC_G_EVENT_TRIGGER_short "(self.index,a) end,\
  gen = function (self,a) return gen(self.index,a) end\
  \
    }}"

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

#endif
