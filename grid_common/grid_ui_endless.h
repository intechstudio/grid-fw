#ifndef GRID_UI_ENDLESS_H
#define GRID_UI_ENDLESS_H

#include <stdint.h>

#include "grid_protocol.h"
#include "grid_ui.h"

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

// clang-format off

#define GRID_LUA_EP_META_init \
  "endless_meta = { __index = {" \
  "type = 'endless', "\
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_EP_ELEMENT_INDEX_short, GRID_LUA_FNC_EP_ELEMENT_INDEX_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_EP_LED_INDEX_short, GRID_LUA_FNC_EP_LED_INDEX_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_EP_BUTTON_VALUE_short, GRID_LUA_FNC_EP_BUTTON_VALUE_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_EP_BUTTON_MIN_short, GRID_LUA_FNC_EP_BUTTON_MIN_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_EP_BUTTON_MAX_short, GRID_LUA_FNC_EP_BUTTON_MAX_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_EP_BUTTON_MODE_short, GRID_LUA_FNC_EP_BUTTON_MODE_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_EP_BUTTON_ELAPSED_short, GRID_LUA_FNC_EP_BUTTON_ELAPSED_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_EP_BUTTON_STATE_short, GRID_LUA_FNC_EP_BUTTON_STATE_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_EP_LED_OFFSET_short, GRID_LUA_FNC_EP_LED_OFFSET_index) "," \
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
  "post_init_cb = function (self) " \
  "self:"GRID_LUA_FNC_A_INIT_short"() " \
  "self:"GRID_LUA_FNC_A_BUTTON_short"() " \
  "self:"GRID_LUA_FNC_A_ENDLESS_short"() " \
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

#define GRID_ACTIONSTRING_ENDLESS_INIT "<?lua --[[@cb]] --[[Endless Init]] ?>"

#define GRID_ACTIONSTRING_ENDLESS_ENDLESS \
  "<?lua --[[@sen]] self:epmo(0) self:epv0(50) self:epmi(0) self:epma(127) self:epse(50)" \
  "--[[@sglc]] self:glc(-1,{{-1,-1,-1,1}}) self:glp(-1,-1)" \
  "--[[@gms]] self:gms(-1,-1,-1,-1) ?>"

#define GRID_ACTIONSTRING_ENDLESS_BUTTON \
  "<?lua --[[@sbc]] self:bmo(0) self:bmi(0) self:bma(127)" \
  "--[[@sglc]] self:glc(-1,{{-1,-1,-1,1}}) self:glp(-1,-1)" \
  "--[[@gms]] self:gms(-1,-1,-1,-1) ?>"

// clang-format on

#endif /* GRID_UI_ENDLESS_H */
