#ifndef GRID_UI_ENDLESS_H
#define GRID_UI_ENDLESS_H

#include <stdint.h>

#include "grid_protocol.h"
#include "grid_ui.h"
#include "grid_ui_button.h"

struct grid_ui_endless_sample {
  uint16_t phase_a;
  uint16_t phase_b;
  uint16_t button_value;
};

struct grid_ui_endless_state {
  struct grid_ui_element* parent;
  uint64_t encoder_last_real_time;
  double delta_vel_frac;
  uint16_t prev_phase_a;
  uint16_t prev_phase_b;
  uint16_t prev_button_value;
  uint8_t adc_bit_depth;
  struct grid_ui_button_state button;
};

void grid_ui_endless_configure(struct grid_ui_endless_state* state, uint8_t adc_bit_depth, uint8_t button_adc_bit_depth, double button_threshold, double button_hysteresis);

void grid_ui_element_endless_init(struct grid_ui_element* ele);
void grid_ui_element_endless_template_parameter_init(struct grid_ui_template_buffer* buf);

void grid_ui_element_endless_event_clear_cb(struct grid_ui_event* eve);
void grid_ui_element_endless_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new);

static inline struct grid_ui_endless_state* grid_ui_endless_get_state(struct grid_ui_element* ele) {
  return (struct grid_ui_endless_state*)ele->primary_state;
}

void grid_ui_endless_store_input(struct grid_ui_endless_state* state, struct grid_ui_endless_sample sample);

uint8_t grid_ui_endless_update_trigger(struct grid_ui_element* ele, int stabilized, int16_t delta, uint16_t value_degrees, uint64_t* endless_last_real_time, double* delta_frac);

// ========================= ENDLESS POTEMETER =========================== //

// clang-format off

#define GRID_LUA_EP_TYPE "Endless"

extern const luaL_Reg GRID_LUA_EP_INDEX_META[];

#define GRID_LUA_EP_META_init \
  GRID_LUA_EP_TYPE " = { __index = {" \
  "type = 'endless', "\
  \
  "post_init_cb = function (self) " \
  "self:"GRID_LUA_FNC_A_INIT_short"() " \
  "self:"GRID_LUA_FNC_A_BUTTON_short"() " \
  "self:"GRID_LUA_FNC_A_ENDLESS_short"() " \
  "end," \
  \
  GRID_LUA_FNC_ASSIGN_META_PAR1_RET("gen", GRID_LUA_FNC_G_ELEMENTNAME_short) "," \
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
