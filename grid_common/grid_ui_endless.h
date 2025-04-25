#ifndef GRID_UI_ENDLESS_H
#define GRID_UI_ENDLESS_H

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "grid_ain.h"
#include "grid_protocol.h"
#include "grid_ui.h"
#include "grid_ui_system.h"

struct grid_ui_endless_state {
  uint16_t phase_a;
  uint16_t phase_b;
  uint16_t button_value;
  uint64_t encoder_last_real_time;
  double delta_vel_frac;
};

void grid_ui_element_endless_init(struct grid_ui_element* ele);
void grid_ui_element_endless_template_parameter_init(struct grid_ui_template_buffer* buf);

void grid_ui_element_endless_event_clear_cb(struct grid_ui_event* eve);
void grid_ui_element_endless_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new);

void grid_ui_endless_store_input(struct grid_ui_element* ele, uint8_t input_channel, uint8_t adc_bit_depth, struct grid_ui_endless_state* new_value, struct grid_ui_endless_state* old_value);

uint8_t grid_ui_endless_update_trigger(struct grid_ui_element* ele, int stabilized, int16_t delta, uint64_t* endless_last_real_time, double* delta_frac);

// ========================= ENDLESS POTEMETER =========================== //

// Endless potentiometer init function
// clang-format off
#define GRID_LUA_EP_META_init                                                                                                                                                                          \
  "endless_meta = { __index = {" \
  \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_EP_ELEMENT_INDEX_short, GRID_LUA_FNC_EP_ELEMENT_INDEX_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_EP_BUTTON_NUMBER_short, GRID_LUA_FNC_EP_BUTTON_NUMBER_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_EP_BUTTON_VALUE_short, GRID_LUA_FNC_EP_BUTTON_VALUE_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_EP_BUTTON_MIN_short, GRID_LUA_FNC_EP_BUTTON_MIN_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_EP_BUTTON_MAX_short, GRID_LUA_FNC_EP_BUTTON_MAX_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_EP_BUTTON_MODE_short, GRID_LUA_FNC_EP_BUTTON_MODE_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_EP_BUTTON_ELAPSED_short, GRID_LUA_FNC_EP_BUTTON_ELAPSED_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_EP_BUTTON_STATE_short, GRID_LUA_FNC_EP_BUTTON_STATE_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_EP_ENDLESS_NUMBER_short, GRID_LUA_FNC_EP_ENDLESS_NUMBER_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_EP_ENDLESS_VALUE_short, GRID_LUA_FNC_EP_ENDLESS_VALUE_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_EP_ENDLESS_MIN_short, GRID_LUA_FNC_EP_ENDLESS_MIN_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_EP_ENDLESS_MAX_short, GRID_LUA_FNC_EP_ENDLESS_MAX_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_EP_ENDLESS_MODE_short, GRID_LUA_FNC_EP_ENDLESS_MODE_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_EP_ENDLESS_ELAPSED_short, GRID_LUA_FNC_EP_ENDLESS_ELAPSED_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_EP_ENDLESS_STATE_short, GRID_LUA_FNC_EP_ENDLESS_STATE_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_EP_ENDLESS_VELOCITY_short, GRID_LUA_FNC_EP_ENDLESS_VELOCITY_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_EP_ENDLESS_DIRECTION_short, GRID_LUA_FNC_EP_ENDLESS_DIRECTION_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_EP_ENDLESS_SENSITIVITY_short, GRID_LUA_FNC_EP_ENDLESS_SENSITIVITY_index) "," \
  \
  GRID_LUA_FNC_ASSIGN_META_UNDEF(GRID_LUA_FNC_A_INIT_short) "," \
  GRID_LUA_FNC_ASSIGN_META_UNDEF(GRID_LUA_FNC_A_BUTTON_short) "," \
  GRID_LUA_FNC_ASSIGN_META_UNDEF(GRID_LUA_FNC_A_TIMER_short) "," \
  GRID_LUA_FNC_ASSIGN_META_UNDEF(GRID_LUA_FNC_A_ENDLESS_short) "," \
  \
  GRID_LUA_FNC_ASSIGN_META_PAR1("gtt", GRID_LUA_FNC_G_TIMER_START_short) "," \
  GRID_LUA_FNC_ASSIGN_META_PAR0("gtp", GRID_LUA_FNC_G_TIMER_STOP_short) "," \
  GRID_LUA_FNC_ASSIGN_META_PAR1("get", GRID_LUA_FNC_G_EVENT_TRIGGER_short) "," \
  GRID_LUA_FNC_ASSIGN_META_PAR1_RET("gen", GRID_LUA_FNC_G_ELEMENTNAME_short) "," \
  \
  "}}"
// clang-format on

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

#endif /* GRID_UI_ENDLESS_H */
