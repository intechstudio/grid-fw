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

// clang-format off

#define GRID_LUA_E_TYPE "Encoder"

extern const luaL_Reg GRID_LUA_E_INDEX_META[];

#define GRID_LUA_E_META_init \
  GRID_LUA_E_TYPE " = { __index = { " \
  \
  "type = 'encoder', "\
  \
  "post_init_cb = function (self) " \
  "self:"GRID_LUA_FNC_A_INIT_short"() " \
  "self:"GRID_LUA_FNC_A_BUTTON_short"() " \
  "self:"GRID_LUA_FNC_A_ENCODER_short"() " \
  "end," \
  \
  GRID_LUA_FNC_ASSIGN_META_PAR1_RET("gen", GRID_LUA_FNC_G_ELEMENTNAME_short) "," \
  \
  GRID_LUA_FNC_E_BUTTON_STEP_short " =function (self) " \
  "local steps, min, max, value = self:" GRID_LUA_FNC_B_BUTTON_MODE_short "(), self:" GRID_LUA_FNC_B_BUTTON_MIN_short "(), self:" GRID_LUA_FNC_B_BUTTON_MAX_short \
  "(), self:" GRID_LUA_FNC_B_BUTTON_VALUE_short "() " \
  "if steps == 0 then return false end " \
  "return value // ((max - min) // steps) " \
  "end," \
  \
  "}}"

#define GRID_ACTIONSTRING_ENCODER_INIT "<?lua --[[@cb]] --[[Encoder Init]] ?>"

#define GRID_ACTIONSTRING_ENCODER_ENCODER \
  "<?lua --[[@sec]] self:emo(0) self:ev0(50) self:emi(0) self:ema(127) self:ese(100)" \
  "--[[@sglc]] self:glc(-1,{{-1,-1,-1,1}}) self:glp(-1,-1)" \
  "--[[@gms]] self:gms(-1,-1,-1,-1) ?>"

// clang-format on

#endif /* GRID_UI_ENCODER_H */
