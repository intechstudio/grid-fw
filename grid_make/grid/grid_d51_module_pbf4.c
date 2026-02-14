#include "grid_d51_module_pbf4.h"

#include <string.h>

#include "grid_ain.h"
#include "grid_asc.h"
#include "grid_d51_adc.h"
#include "grid_platform.h"
#include "grid_ui_button.h"
#include "grid_ui_potmeter.h"
#include "grid_ui_system.h"

#include "grid_cal.h"
#include "grid_config.h"

#include <assert.h>

#define GRID_MODULE_PBF4_BUTTON_COUNT 4
#define GRID_MODULE_PBF4_POTMETER_COUNT 8

#define X GRID_MUX_UNUSED
static const uint8_t mux_element_lookup[2][8] = {
    {0, 1, 4, 5, X, X, 8, 9},   // MUX_0 -> ADC_1
    {2, 3, 6, 7, X, X, 10, 11}, // MUX_1 -> ADC_0
};
#undef X
static uint16_t element_invert_bm = 0b0000000000001111;

static struct grid_asc* asc_state = NULL;

static void pbf4_process_analog(void* user) {

  struct grid_d51_adc_result* result = (struct grid_d51_adc_result*)user;

  uint8_t element_index = mux_element_lookup[result->channel][result->mux_state];
  assert(element_index != GRID_MUX_UNUSED);

  uint16_t inverted = GRID_ADC_INVERT_COND(result->value, element_index, element_invert_bm);
  uint16_t downsampled = GRID_ADC_DOWNSAMPLE(inverted);

  uint16_t processed;
  if (!grid_asc_process(asc_state, element_index, downsampled, &processed)) {
    return;
  }

  if (element_index >= GRID_MODULE_PBF4_POTMETER_COUNT) {
    grid_ui_button_store_input(&grid_ui_state, element_index, processed);
  } else {
    grid_ui_potmeter_store_input(&grid_ui_state, element_index, processed);
  }
}

void grid_d51_module_pbf4_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_d51_adc_model* adc, struct grid_config_model* conf, struct grid_cal_model* cal) {

  asc_state = grid_platform_allocate_volatile(12 * sizeof(struct grid_asc));
  memset(asc_state, 0, 12 * sizeof(struct grid_asc));

  // Buttons are elements 8-11
  for (int i = 0; i < GRID_MODULE_PBF4_BUTTON_COUNT; ++i) {
    grid_ui_button_state_init(ui, GRID_MODULE_PBF4_POTMETER_COUNT + i, GRID_AIN_INTERNAL_RESOLUTION, 0.5, 0.2);
  }

  // Potmeters are elements 0-7
  for (int i = 0; i < GRID_MODULE_PBF4_POTMETER_COUNT; ++i) {
    grid_ui_potmeter_state_init(ui, i, GRID_AIN_INTERNAL_RESOLUTION, GRID_POTMETER_DEADZONE, GRID_POTMETER_CENTER);
  }

  grid_asc_array_set_factors(asc_state, 12, 0, 12, 1);

  grid_config_init(conf, cal);

  grid_cal_init(cal, ui->element_list_length, 12);

  // Potmeter calibration (elements 0-7, first 4 have center detent)
  for (int i = 0; i < GRID_MODULE_PBF4_POTMETER_COUNT; ++i) {
    struct grid_ui_element* ele = &ui->element_list[i];
    struct grid_ui_potmeter_state* state = (struct grid_ui_potmeter_state*)ele->primary_state;
    assert(grid_cal_set(cal, i, GRID_CAL_LIMITS, &state->limits) == 0);
    if (i < 4) {
      assert(grid_cal_set(cal, i, GRID_CAL_CENTER, &state->center) == 0);
      assert(grid_cal_set(cal, i, GRID_CAL_DETENT, &state->detent) == 0);
    }
  }

  assert(grid_ui_bulk_start_with_state(&grid_ui_state, grid_ui_bulk_conf_read, 0, 0, NULL));
  grid_ui_bulk_flush(&grid_ui_state);

  grid_d51_adc_init(adc, 0b11001111, pbf4_process_analog);
  grid_d51_adc_start(adc);
}
