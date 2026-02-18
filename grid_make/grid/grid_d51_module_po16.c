#include "grid_d51_module_po16.h"

#include "grid_ain.h"
#include "grid_platform.h"
#include "grid_ui_potmeter.h"

#include "grid_cal.h"
#include "grid_config.h"
#include "grid_d51_adc.h"

#include <assert.h>

static const uint8_t mux_element_lookup[2][8] = {
    {0, 1, 4, 5, 8, 9, 12, 13},   // MUX_0 -> ADC_1
    {2, 3, 6, 7, 10, 11, 14, 15}, // MUX_1 -> ADC_0
};
static uint16_t element_invert_bm = 0;

static struct grid_ui_model* ui_ptr = NULL;

static void po16_process_analog(struct grid_adc_result* result) {

  uint8_t element_index = mux_element_lookup[result->channel][result->mux_state];
  uint16_t inverted = GRID_ADC_INVERT_COND(result->value, element_index, element_invert_bm);

  struct grid_ui_element* ele = &ui_ptr->element_list[element_index];
  grid_ui_potmeter_store_input(grid_ui_potmeter_get_state(ele), inverted);
}

void grid_d51_module_po16_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_d51_adc_model* adc, struct grid_config_model* conf, struct grid_cal_model* cal) {

  ui_ptr = ui;

  if (grid_hwcfg_module_is_po16_reverse_polarity(sys)) {
    element_invert_bm = 0b1111111111111111;
  }

  for (int i = 0; i < ui->element_list_length; ++i) {
    struct grid_ui_element* ele = &ui->element_list[i];
    if (ele->type == GRID_PARAMETER_ELEMENT_POTMETER) {
      struct grid_ui_potmeter_state* state = (struct grid_ui_potmeter_state*)ele->primary_state;
      grid_ui_potmeter_configure(state, GRID_AIN_INTERNAL_RESOLUTION, GRID_POTMETER_DEADZONE, GRID_POTMETER_CENTER);
    }
  }

  grid_config_init(conf, cal);
  grid_cal_init(cal, ui->element_list_length, GRID_AIN_INTERNAL_RESOLUTION);

  for (int i = 0; i < ui->element_list_length; ++i) {
    struct grid_ui_element* ele = &ui->element_list[i];
    if (ele->type == GRID_PARAMETER_ELEMENT_POTMETER) {
      struct grid_ui_potmeter_state* state = (struct grid_ui_potmeter_state*)ele->primary_state;
      assert(grid_cal_set(cal, i, GRID_CAL_LIMITS, &state->limits) == 0);
      assert(grid_cal_set(cal, i, GRID_CAL_CENTER, &state->center) == 0);
      assert(grid_cal_set(cal, i, GRID_CAL_DETENT, &state->detent) == 0);
    }
  }

  assert(grid_ui_bulk_start_with_state(&grid_ui_state, grid_ui_bulk_conf_read, 0, 0, NULL));
  grid_ui_bulk_flush(&grid_ui_state);

  grid_d51_adc_init(adc, 0b11111111, po16_process_analog);
  grid_d51_adc_start(adc);
}
