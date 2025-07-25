#ifndef GRID_UI_ENCODER_H
#define GRID_UI_ENCODER_H

#include <stdint.h>

#include "grid_protocol.h"
#include "grid_ui.h"

#define GRID_UI_ENCODER_INIT_SAMPLES 2

struct grid_ui_encoder_state {
  uint64_t encoder_last_real_time;
  uint64_t button_last_real_time;
  uint8_t last_nibble;
  uint8_t detent;
  int8_t encoder_last_leave_dir;
  uint8_t initial_samples;
  int8_t direction;
};

void grid_ui_encoder_state_init(struct grid_ui_encoder_state* state, uint8_t detent, int8_t direction);

void grid_ui_element_encoder_init(struct grid_ui_element* ele);
void grid_ui_element_encoder_template_parameter_init(struct grid_ui_template_buffer* buf);

void grid_ui_element_encoder_event_clear_cb(struct grid_ui_event* eve);
void grid_ui_element_encoder_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new);

int16_t grid_ui_encoder_rotation_delta(uint8_t old_value, uint8_t new_value, uint8_t detent, int8_t* dir_lock);
uint8_t grid_ui_encoder_update_trigger(struct grid_ui_element* ele, uint64_t* encoder_last_real_time, int16_t delta);

void grid_ui_encoder_store_input(struct grid_ui_element* ele, struct grid_ui_encoder_state* state, uint8_t new_value);

// ========================= ENCODER =========================== //

// Encoder init function
// clang-format off
#define GRID_LUA_E_META_init                                                                                                                                                                           \
  "encoder_meta = { __index = {" \
  \
  "type = 'encoder', "\
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_E_ELEMENT_INDEX_short, GRID_LUA_FNC_E_ELEMENT_INDEX_index) "," \
  GRID_LUA_FNC_ASSIGN_META_GTV(GRID_LUA_FNC_E_LED_INDEX_short, GRID_LUA_FNC_E_LED_INDEX_index) "," \
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
  "post_init_cb = function (self) " \
  "self:"GRID_LUA_FNC_A_INIT_short"() " \
  "self:"GRID_LUA_FNC_A_BUTTON_short"() " \
  "self:"GRID_LUA_FNC_A_ENCODER_short"() " \
  "end," \
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

#define GRID_ACTIONSTRING_ENCODER_INIT "<?lua --[[@cb]] --[[Encoder Init]] ?>"                                                               

#define GRID_ACTIONSTRING_ENCODER_ENCODER "<?lua --[[@sec]] self:emo(0) self:ev0(50) self:emi(0) self:ema(127) self:ese(100)--[[@sglc]] self:glc(-1,{{-1,-1,-1,1}}) self:glp(-1,-1)--[[@gms]] self:gms(-1,-1,-1,-1) ?>"

#endif /* GRID_UI_ENCODER_H */
