#pragma once

#ifndef GRID_UI_ENDLESS_H_INCLUDED
#define GRID_UI_ENDLESS_H_INCLUDED

#include "grid_ui.h"
#include <stdint.h>

struct grid_ui_endless_state {
  uint16_t phase_a;
  uint16_t phase_b;
  uint16_t button_value;
  uint64_t button_last_real_time;
  uint64_t encoder_last_real_time;
  double delta_vel_frac;
};

void grid_ui_element_endless_init(struct grid_ui_element* ele);
void grid_ui_element_endless_template_parameter_init(struct grid_ui_template_buffer* buf);

void grid_ui_element_endless_event_clear_cb(struct grid_ui_event* eve);
void grid_ui_element_endless_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new);

void grid_ui_endless_store_input(uint8_t input_channel, uint8_t adc_bit_depth, struct grid_ui_endless_state* new_value, struct grid_ui_endless_state* old_value);

uint8_t grid_ui_endless_update_trigger(struct grid_ui_element* ele, int16_t delta, uint64_t* endless_last_real_time, double* delta_vel_frac);

// ========================= ENDLESS POTEMETER =========================== //

#define GRID_LUA_FNC_EP_ELEMENT_INDEX_index 0
#define GRID_LUA_FNC_EP_ELEMENT_INDEX_helper "0"
#define GRID_LUA_FNC_EP_ELEMENT_INDEX_short "ind"
#define GRID_LUA_FNC_EP_ELEMENT_INDEX_human "element_index"

#define GRID_LUA_FNC_EP_BUTTON_NUMBER_index 1
#define GRID_LUA_FNC_EP_BUTTON_NUMBER_helper "1"
#define GRID_LUA_FNC_EP_BUTTON_NUMBER_short "bnu"
#define GRID_LUA_FNC_EP_BUTTON_NUMBER_human "button_number"

#define GRID_LUA_FNC_EP_BUTTON_VALUE_index 2
#define GRID_LUA_FNC_EP_BUTTON_VALUE_helper "2"
#define GRID_LUA_FNC_EP_BUTTON_VALUE_short "bva"
#define GRID_LUA_FNC_EP_BUTTON_VALUE_human "button_value"

#define GRID_LUA_FNC_EP_BUTTON_MIN_index 3
#define GRID_LUA_FNC_EP_BUTTON_MIN_helper "3"
#define GRID_LUA_FNC_EP_BUTTON_MIN_short "bmi"
#define GRID_LUA_FNC_EP_BUTTON_MIN_human "button_min"

#define GRID_LUA_FNC_EP_BUTTON_MAX_index 4
#define GRID_LUA_FNC_EP_BUTTON_MAX_helper "4"
#define GRID_LUA_FNC_EP_BUTTON_MAX_short "bma"
#define GRID_LUA_FNC_EP_BUTTON_MAX_human "button_max"

#define GRID_LUA_FNC_EP_BUTTON_MODE_index 5
#define GRID_LUA_FNC_EP_BUTTON_MODE_helper "5"
#define GRID_LUA_FNC_EP_BUTTON_MODE_short "bmo"
#define GRID_LUA_FNC_EP_BUTTON_MODE_human "button_mode"

#define GRID_LUA_FNC_EP_BUTTON_ELAPSED_index 6
#define GRID_LUA_FNC_EP_BUTTON_ELAPSED_helper "6"
#define GRID_LUA_FNC_EP_BUTTON_ELAPSED_short "bel"
#define GRID_LUA_FNC_EP_BUTTON_ELAPSED_human "button_elapsed_time"

#define GRID_LUA_FNC_EP_BUTTON_STATE_index 7
#define GRID_LUA_FNC_EP_BUTTON_STATE_helper "7"
#define GRID_LUA_FNC_EP_BUTTON_STATE_short "bst"
#define GRID_LUA_FNC_EP_BUTTON_STATE_human "button_state"

#define GRID_LUA_FNC_EP_ENDLESS_NUMBER_index 8
#define GRID_LUA_FNC_EP_ENDLESS_NUMBER_helper "8"
#define GRID_LUA_FNC_EP_ENDLESS_NUMBER_short "epnu"
#define GRID_LUA_FNC_EP_ENDLESS_NUMBER_human "endless_number"

#define GRID_LUA_FNC_EP_ENDLESS_VALUE_index 9
#define GRID_LUA_FNC_EP_ENDLESS_VALUE_helper "9"
#define GRID_LUA_FNC_EP_ENDLESS_VALUE_short "epva"
#define GRID_LUA_FNC_EP_ENDLESS_VALUE_human "endless_value"

#define GRID_LUA_FNC_EP_ENDLESS_MIN_index 10
#define GRID_LUA_FNC_EP_ENDLESS_MIN_helper "10"
#define GRID_LUA_FNC_EP_ENDLESS_MIN_short "epmi"
#define GRID_LUA_FNC_EP_ENDLESS_MIN_human "endless_min"

#define GRID_LUA_FNC_EP_ENDLESS_MAX_index 11
#define GRID_LUA_FNC_EP_ENDLESS_MAX_helper "11"
#define GRID_LUA_FNC_EP_ENDLESS_MAX_short "epma"
#define GRID_LUA_FNC_EP_ENDLESS_MAX_human "endless_max"

#define GRID_LUA_FNC_EP_ENDLESS_MODE_index 12
#define GRID_LUA_FNC_EP_ENDLESS_MODE_helper "12"
#define GRID_LUA_FNC_EP_ENDLESS_MODE_short "epmo"
#define GRID_LUA_FNC_EP_ENDLESS_MODE_human "endless_mode"

#define GRID_LUA_FNC_EP_ENDLESS_ELAPSED_index 13
#define GRID_LUA_FNC_EP_ENDLESS_ELAPSED_helper "13"
#define GRID_LUA_FNC_EP_ENDLESS_ELAPSED_short "epel"
#define GRID_LUA_FNC_EP_ENDLESS_ELAPSED_human "endless_elapsed_time"

#define GRID_LUA_FNC_EP_ENDLESS_STATE_index 14
#define GRID_LUA_FNC_EP_ENDLESS_STATE_helper "14"
#define GRID_LUA_FNC_EP_ENDLESS_STATE_short "epst"
#define GRID_LUA_FNC_EP_ENDLESS_STATE_human "endless_state"

#define GRID_LUA_FNC_EP_ENDLESS_VELOCITY_index 15
#define GRID_LUA_FNC_EP_ENDLESS_VELOCITY_helper "15"
#define GRID_LUA_FNC_EP_ENDLESS_VELOCITY_short "epv0"
#define GRID_LUA_FNC_EP_ENDLESS_VELOCITY_human "endless_velocity"

#define GRID_LUA_FNC_EP_ENDLESS_DIRECTION_index 16
#define GRID_LUA_FNC_EP_ENDLESS_DIRECTION_helper "16"
#define GRID_LUA_FNC_EP_ENDLESS_DIRECTION_short "epdir"
#define GRID_LUA_FNC_EP_ENDLESS_DIRECTION_human "endless_direction"

#define GRID_LUA_FNC_EP_ENDLESS_SENSITIVITY_index 17
#define GRID_LUA_FNC_EP_ENDLESS_SENSITIVITY_helper "17"
#define GRID_LUA_FNC_EP_ENDLESS_SENSITIVITY_short "epse"
#define GRID_LUA_FNC_EP_ENDLESS_SENSITIVITY_human "endless_sensitivity"

// Endless potentiometer parameters
#define GRID_LUA_FNC_EP_LIST_length 18

// Endless potentiometer init function
#define GRID_LUA_EP_META_init                                                                                                                                                                          \
  "endless_meta = { __index = { \
   \
  " GRID_LUA_FNC_EP_ELEMENT_INDEX_short "=function (self,a) return "                                                                                                                                   \
  "gtv(self.index, " GRID_LUA_FNC_EP_ELEMENT_INDEX_helper ", a) end, \
  \
  " GRID_LUA_FNC_EP_BUTTON_NUMBER_short "=function (self,a) return "                                                                                                                                   \
  "gtv(self.index, " GRID_LUA_FNC_EP_BUTTON_NUMBER_helper ", a) end, \
  " GRID_LUA_FNC_EP_BUTTON_VALUE_short "=function (self,a) return "                                                                                                                                    \
  "gtv(self.index, " GRID_LUA_FNC_EP_BUTTON_VALUE_helper ", a) end, \
  " GRID_LUA_FNC_EP_BUTTON_MIN_short "=function (self,a) return "                                                                                                                                      \
  "gtv(self.index, " GRID_LUA_FNC_EP_BUTTON_MIN_helper ", a) end, \
  " GRID_LUA_FNC_EP_BUTTON_MAX_short "=function (self,a) return "                                                                                                                                      \
  "gtv(self.index, " GRID_LUA_FNC_EP_BUTTON_MAX_helper ", a) end, \
  " GRID_LUA_FNC_EP_BUTTON_MODE_short "=function (self,a) return "                                                                                                                                     \
  "gtv(self.index, " GRID_LUA_FNC_EP_BUTTON_MODE_helper ", a) end, \
  " GRID_LUA_FNC_EP_BUTTON_ELAPSED_short "=function (self,a) return "                                                                                                                                  \
  "gtv(self.index, " GRID_LUA_FNC_EP_BUTTON_ELAPSED_helper ", a) end, \
  " GRID_LUA_FNC_EP_BUTTON_STATE_short "=function (self,a) return "                                                                                                                                    \
  "gtv(self.index, " GRID_LUA_FNC_EP_BUTTON_STATE_helper ", a) end, \
  \
  " GRID_LUA_FNC_EP_ENDLESS_NUMBER_short "=function (self,a) return "                                                                                                                                  \
  "gtv(self.index, " GRID_LUA_FNC_EP_ENDLESS_NUMBER_helper ", a) end, \
  " GRID_LUA_FNC_EP_ENDLESS_VALUE_short "=function (self,a) return "                                                                                                                                   \
  "gtv(self.index, " GRID_LUA_FNC_EP_ENDLESS_VALUE_helper ", a) end, \
  " GRID_LUA_FNC_EP_ENDLESS_MIN_short "=function (self,a) return "                                                                                                                                     \
  "gtv(self.index, " GRID_LUA_FNC_EP_ENDLESS_MIN_helper ", a) end, \
  " GRID_LUA_FNC_EP_ENDLESS_MAX_short "=function (self,a) return "                                                                                                                                     \
  "gtv(self.index, " GRID_LUA_FNC_EP_ENDLESS_MAX_helper ", a) end, \
  " GRID_LUA_FNC_EP_ENDLESS_MODE_short "=function (self,a) return "                                                                                                                                    \
  "gtv(self.index, " GRID_LUA_FNC_EP_ENDLESS_MODE_helper ", a) end, \
  " GRID_LUA_FNC_EP_ENDLESS_ELAPSED_short "=function (self,a) return "                                                                                                                                 \
  "gtv(self.index, " GRID_LUA_FNC_EP_ENDLESS_ELAPSED_helper ", a) end, \
  " GRID_LUA_FNC_EP_ENDLESS_STATE_short "=function (self,a) return "                                                                                                                                   \
  "gtv(self.index, " GRID_LUA_FNC_EP_ENDLESS_STATE_helper ", a) end, \
  " GRID_LUA_FNC_EP_ENDLESS_VELOCITY_short "=function (self,a) return "                                                                                                                                \
  "gtv(self.index, " GRID_LUA_FNC_EP_ENDLESS_VELOCITY_helper ", a) end, \
  " GRID_LUA_FNC_EP_ENDLESS_DIRECTION_short "=function (self,a) return "                                                                                                                               \
  "gtv(self.index, " GRID_LUA_FNC_EP_ENDLESS_DIRECTION_helper ", a) end, \
  " GRID_LUA_FNC_EP_ENDLESS_SENSITIVITY_short "=function (self,a) return "                                                                                                                             \
  "gtv(self.index, " GRID_LUA_FNC_EP_ENDLESS_SENSITIVITY_helper ", a) end, \
  \
  " GRID_LUA_FNC_A_INIT_short " = function (self) print('undefined action') end,\
  " GRID_LUA_FNC_A_ENDLESS_short " = function (self) print('undefined action') end,\
  " GRID_LUA_FNC_A_BUTTON_short " = function (self) print('undefined action') end,\
  " GRID_LUA_FNC_A_TIMER_short " = function (self) print('undefined action') end,\
  \
  gtt = function (self,a) " GRID_LUA_FNC_G_TIMER_START_short "(self.index,a) end,\
  gtp = function (self) " GRID_LUA_FNC_G_TIMER_STOP_short "(self.index) end,\
  get = function (self,a) " GRID_LUA_FNC_G_EVENT_TRIGGER_short "(self.index,a) end,\
  gen = function (self,a) return gen(self.index,a) end\
  \
    }}"

#define GRID_ACTIONSTRING_ENDLESS_INIT                                                                                                                                                                 \
  "<?lua --[[@l]] local num,val,min,max,red,gre,blu=self:ind(),self:epva(),self:epmi(),self:epma(),glr(),glg(),glb()--[[@for]] for i=1,5,1 do--[[@l]] local "                                          \
  "intensity,lednum=gsc(i-1,val,min,max),num+i*2-2--[[@glc]] glc(lednum,1,red,gre,blu,0)--[[@glc]] glc(lednum,2,red,gre,blu,1)--[[@glp]] glp(lednum,1,0)--[[@glp]] glp(lednum,2,intensity)--[[@enl]] " \
  "end ?>"
#define GRID_ACTIONSTRING_ENDLESS_ENDLESS                                                                                                                                                              \
  "<?lua --[[@l]] local num,val,min,max,ch,cc=self:ind(),self:epva(),self:epmi(),self:epma(),(gmy()*4+gpc())%16,(32+gmx()*16+self:ind())%128--[[@gmsh]] gms(ch,176,cc,val//128) "                      \
  "gms(ch,176,cc+32,val%128)--[[@for]] for i=1,5,1 do--[[@l]] local intensity,lednum=gsc(i-1,val,min,max),num+i*2-2--[[@glp]] glp(lednum,2,intensity)--[[@enl]] end ?>"
#define GRID_ACTIONSTRING_ENDLESS_BUTTON                                                                                                                                                               \
  "<?lua --[[@l]] local num,val,ch,note=self:ind(),self:bva(),(gmy()*4+gpc())%16,(32+gmx()*16+self:ind())%128--[[@gms]] gms(ch,144,note,val)--[[@for]] for i=1,5,1 do--[[@l]] local "                  \
  "lednum=num+i*2-2--[[@glp]] glp(lednum,1,val)--[[@enl]] end ?>"

#endif
