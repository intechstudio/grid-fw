#pragma once

#ifndef GRID_UI_POTMETER_H_INCLUDED
#define GRID_UI_POTMETER_H_INCLUDED

#include "grid_ui.h"
#include <stdint.h>

void grid_ui_element_potmeter_init(struct grid_ui_element* ele);
void grid_ui_element_potmeter_template_parameter_init(struct grid_ui_template_buffer* buf);

void grid_ui_element_potmeter_event_clear_cb(struct grid_ui_event* eve);
void grid_ui_element_potmeter_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new);

void grid_ui_potmeter_store_input(uint8_t input_channel, uint64_t* last_real_time, uint16_t value, uint8_t adc_bit_depth);

// ========================= POTMETER =========================== //

#define GRID_LUA_FNC_P_ELEMENT_INDEX_index 0
#define GRID_LUA_FNC_P_ELEMENT_INDEX_helper "0"
#define GRID_LUA_FNC_P_ELEMENT_INDEX_short "ind"
#define GRID_LUA_FNC_P_ELEMENT_INDEX_human "element_index"

#define GRID_LUA_FNC_P_POTMETER_NUMBER_index 1
#define GRID_LUA_FNC_P_POTMETER_NUMBER_helper "1"
#define GRID_LUA_FNC_P_POTMETER_NUMBER_short "pnu"
#define GRID_LUA_FNC_P_POTMETER_NUMBER_human "potmeter_number"

#define GRID_LUA_FNC_P_POTMETER_VALUE_index 2
#define GRID_LUA_FNC_P_POTMETER_VALUE_helper "2"
#define GRID_LUA_FNC_P_POTMETER_VALUE_short "pva"
#define GRID_LUA_FNC_P_POTMETER_VALUE_human "potmeter_value"

#define GRID_LUA_FNC_P_POTMETER_MIN_index 3
#define GRID_LUA_FNC_P_POTMETER_MIN_helper "3"
#define GRID_LUA_FNC_P_POTMETER_MIN_short "pmi"
#define GRID_LUA_FNC_P_POTMETER_MIN_human "potmeter_min"

#define GRID_LUA_FNC_P_POTMETER_MAX_index 4
#define GRID_LUA_FNC_P_POTMETER_MAX_helper "4"
#define GRID_LUA_FNC_P_POTMETER_MAX_short "pma"
#define GRID_LUA_FNC_P_POTMETER_MAX_human "potmeter_max"

#define GRID_LUA_FNC_P_POTMETER_MODE_index 5
#define GRID_LUA_FNC_P_POTMETER_MODE_helper "5"
#define GRID_LUA_FNC_P_POTMETER_MODE_short "pmo"
#define GRID_LUA_FNC_P_POTMETER_MODE_human "potmeter_resolution"

#define GRID_LUA_FNC_P_POTMETER_ELAPSED_index 6
#define GRID_LUA_FNC_P_POTMETER_ELAPSED_helper "6"
#define GRID_LUA_FNC_P_POTMETER_ELAPSED_short "pel"
#define GRID_LUA_FNC_P_POTMETER_ELAPSED_human "potmeter_elapsed_time"

#define GRID_LUA_FNC_P_POTMETER_STATE_index 7
#define GRID_LUA_FNC_P_POTMETER_STATE_helper "7"
#define GRID_LUA_FNC_P_POTMETER_STATE_short "pst"
#define GRID_LUA_FNC_P_POTMETER_STATE_human "potmeter_state"

// Potmeter parameters
#define GRID_LUA_FNC_P_LIST_length 8

// Potmeter init function
#define GRID_LUA_P_META_init                                                                                                                                                                           \
  "potmeter_meta = { __index = { \
   \
  " GRID_LUA_FNC_P_ELEMENT_INDEX_short "=function (self,a) return "                                                                                                                                    \
  "gtv(self.index, " GRID_LUA_FNC_P_ELEMENT_INDEX_helper ", a) end, \
  \
  " GRID_LUA_FNC_P_POTMETER_NUMBER_short "=function (self,a) return "                                                                                                                                  \
  "gtv(self.index, " GRID_LUA_FNC_P_POTMETER_NUMBER_helper ", a) end, \
  " GRID_LUA_FNC_P_POTMETER_VALUE_short "=function (self,a) return "                                                                                                                                   \
  "gtv(self.index, " GRID_LUA_FNC_P_POTMETER_VALUE_helper ", a) end, \
  " GRID_LUA_FNC_P_POTMETER_MIN_short "=function (self,a) return "                                                                                                                                     \
  "gtv(self.index, " GRID_LUA_FNC_P_POTMETER_MIN_helper ", a) end, \
  " GRID_LUA_FNC_P_POTMETER_MAX_short "=function (self,a) return "                                                                                                                                     \
  "gtv(self.index, " GRID_LUA_FNC_P_POTMETER_MAX_helper ", a) end, \
  " GRID_LUA_FNC_P_POTMETER_MODE_short "=function (self,a) return "                                                                                                                                    \
  "gtv(self.index, " GRID_LUA_FNC_P_POTMETER_MODE_helper ", a) end, \
  " GRID_LUA_FNC_P_POTMETER_ELAPSED_short "=function (self,a) return "                                                                                                                                 \
  "gtv(self.index, " GRID_LUA_FNC_P_POTMETER_ELAPSED_helper ", a) end, \
  " GRID_LUA_FNC_P_POTMETER_STATE_short "=function (self,a) return "                                                                                                                                   \
  "gtv(self.index, " GRID_LUA_FNC_P_POTMETER_STATE_helper ", a) end, \
  \
  " GRID_LUA_FNC_A_INIT_short " = function (self) print('undefined action') end,\
  " GRID_LUA_FNC_A_POTMETERCHANGE_short " = function (self) print('undefined action') end,\
  " GRID_LUA_FNC_A_TIMER_short " = function (self) print('undefined action') end,\
  \
  gtt = function (self,a) " GRID_LUA_FNC_G_TIMER_START_short "(self.index,a) end,\
  gtp = function (self) " GRID_LUA_FNC_G_TIMER_STOP_short "(self.index) end,\
  get = function (self,a) " GRID_LUA_FNC_G_EVENT_TRIGGER_short "(self.index,a) end,\
  gen = function (self,a) return gen(self.index,a) end\
    }}"

#define GRID_ACTIONSTRING_POTMETER_INIT                                                                                                                                                                \
  "<?lua --[[@l]] local "                                                                                                                                                                              \
  "num,val,red,gre,blu=self:ind(),self:pva(),glr(),glg(),glb()--[[@glc]] "                                                                                                                             \
  "glc(num,1,red,gre,blu)--[[@glp]] glp(num,1,val) ?>"

// new dynamic midi based on x y and activepage
#define GRID_ACTIONSTRING_POTMETER_POTMETER                                                                                                                                                            \
  "<?lua --[[@l]] local "                                                                                                                                                                              \
  "num,val,ch,cc=self:ind(),self:pva(),(gmy()*4+gpc())%16,(32+gmx()*16+self:"                                                                                                                          \
  "ind())%128--[[@gms]] gms(ch,176,cc,val)--[[@glp]] glp(num,1,val) ?>"

#endif
