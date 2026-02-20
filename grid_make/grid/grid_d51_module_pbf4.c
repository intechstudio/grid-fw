#include "grid_d51_module_pbf4.h"

#include "grid_ain.h"
#include "grid_d51_adc.h"
#include "grid_platform.h"
#include "grid_ui_button.h"
#include "grid_ui_potmeter.h"

#include "grid_cal.h"
#include "grid_config.h"

#include <assert.h>

#define X GRID_MUX_UNUSED
static const uint8_t mux_element_lookup[2][8] = {
    {0, 1, 4, 5, X, X, 8, 9},   // MUX_0 -> ADC_1
    {2, 3, 6, 7, X, X, 10, 11}, // MUX_1 -> ADC_0
};
#undef X
static uint16_t element_invert_bm = 0b0000000000001111;

static struct grid_ui_model* ui_ptr = NULL;

static void pbf4_process_analog(struct grid_adc_result* result) {

  uint8_t element_index = mux_element_lookup[result->channel][result->mux_state];
  assert(element_index != GRID_MUX_UNUSED);

  uint16_t inverted = GRID_ADC_INVERT_COND(result->value, element_index, element_invert_bm);

  struct grid_ui_element* ele = &ui_ptr->element_list[element_index];
  if (ele->type == GRID_PARAMETER_ELEMENT_POTMETER) {
    grid_ui_potmeter_store_input(grid_ui_potmeter_get_state(ele), inverted);
  } else if (ele->type == GRID_PARAMETER_ELEMENT_BUTTON) {
    grid_ui_button_store_input(grid_ui_button_get_state(ele), inverted);
  }
}

void grid_d51_module_pbf4_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_d51_adc_model* adc, struct grid_config_model* conf, struct grid_cal_model* cal) {

  ui_ptr = ui;

  for (int i = 0; i < ui->element_list_length; ++i) {
    struct grid_ui_element* ele = &ui->element_list[i];
    if (ele->type == GRID_PARAMETER_ELEMENT_POTMETER) {
      grid_ui_potmeter_configure(grid_ui_potmeter_get_state(ele), GRID_AIN_INTERNAL_RESOLUTION, GRID_POTMETER_DEADZONE, GRID_POTMETER_CENTER);
    } else if (ele->type == GRID_PARAMETER_ELEMENT_BUTTON) {
      grid_ui_button_configure(grid_ui_button_get_state(ele), GRID_AIN_INTERNAL_RESOLUTION, GRID_BUTTON_THRESHOLD, GRID_BUTTON_HYSTERESIS);
    }
  }

  grid_config_init(conf, cal);
  grid_cal_init(cal, ui->element_list_length, GRID_AIN_INTERNAL_RESOLUTION);

  for (int i = 0; i < ui->element_list_length; ++i) {
    struct grid_ui_element* ele = &ui->element_list[i];
    if (ele->type == GRID_PARAMETER_ELEMENT_POTMETER && i < 4) {
      struct grid_ui_potmeter_state* state = grid_ui_potmeter_get_state(ele);
      grid_cal_channel_set(cal, i, GRID_CAL_LIMITS, &state->limits);
      grid_cal_channel_set(cal, i, GRID_CAL_CENTER, &state->center);
      grid_cal_channel_set(cal, i, GRID_CAL_DETENT, &state->detent);
    } else if (ele->type == GRID_PARAMETER_ELEMENT_POTMETER) {
      grid_cal_channel_set(cal, i, GRID_CAL_LIMITS, &grid_ui_potmeter_get_state(ele)->limits);
    }
  }

  assert(grid_ui_bulk_conf_init(ui, GRID_UI_BULK_CONFREAD_PROGRESS, 0, NULL) == 0);
  grid_ui_bulk_confread_next(ui);

  grid_d51_adc_init(adc, 0b11001111, pbf4_process_analog);
  grid_d51_adc_start(adc);
}
