#ifndef GRID_UI_BUTTON_H
#define GRID_UI_BUTTON_H

#include <stdint.h>

#include "grid_cal.h"
#include "grid_protocol.h"
#include "grid_ui.h"

struct grid_ui_button_state {
  uint64_t last_real_time;
  double threshold;
  double hysteresis;
  uint32_t full_range;
  struct grid_cal_limits limits;
  struct grid_cal_limits limits_prev;
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

void grid_ui_button_store_input(struct grid_ui_element* ele, struct grid_ui_button_state* state, uint16_t value, uint8_t adc_bit_depth);

// ========================= BUTTON =========================== //

// clang-format off

#define GRID_LUA_B_TYPE "Button"

extern const luaL_Reg GRID_LUA_B_INDEX_META[];

#define GRID_LUA_B_META_init \
  GRID_LUA_B_TYPE " = { __index = {" \
   \
  "type = 'button', "\
  \
  "post_init_cb = function (self) " \
  "self:"GRID_LUA_FNC_A_INIT_short"() " \
  "self:"GRID_LUA_FNC_A_BUTTON_short"() " \
  "end," \
  \
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

#define GRID_ACTIONSTRING_BUTTON_INIT "<?lua --[[@cb]] --[[Button Init]] ?>"

#define GRID_ACTIONSTRING_BUTTON_BUTTON \
  "<?lua --[[@sbc]] self:bmo(0) self:bmi(0) self:bma(127)" \
  "--[[@sglc]] self:glc(-1,{{-1,-1,-1,1}}) self:glp(-1,-1)" \
  "--[[@gms]] self:gms(-1,-1,-1,-1) ?>"

// clang-format on

#endif /* GRID_UI_BUTTON_H */
