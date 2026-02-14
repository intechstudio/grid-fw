#include "grid_d51_module_po16.h"

#include <string.h>

#include "grid_ain.h"
#include "grid_platform.h"
#include "grid_ui_potmeter.h"
#include "grid_ui_system.h"

#include "grid_asc.h"
#include "grid_cal.h"
#include "grid_config.h"
#include "grid_d51_adc.h"

#include <assert.h>

#define GRID_MODULE_PO16_POTMETER_COUNT 16

static const uint8_t mux_element_lookup[2][8] = {
    {0, 1, 4, 5, 8, 9, 12, 13},   // MUX_0 -> ADC_1
    {2, 3, 6, 7, 10, 11, 14, 15}, // MUX_1 -> ADC_0
};
static uint16_t element_invert_bm = 0;

static struct grid_asc* asc_state = NULL;

static void po16_process_analog(void* user) {

  struct grid_d51_adc_result* result = (struct grid_d51_adc_result*)user;

  uint8_t element_index = mux_element_lookup[result->channel][result->mux_state];
  uint16_t inverted = GRID_ADC_INVERT_COND(result->value, element_index, element_invert_bm);
  uint16_t downsampled = GRID_ADC_DOWNSAMPLE(inverted);

  uint16_t processed;
  if (!grid_asc_process(asc_state, element_index, downsampled, &processed)) {
    return;
  }

  grid_ui_potmeter_store_input(&grid_ui_state, element_index, processed);
}

void grid_d51_module_po16_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_d51_adc_model* adc, struct grid_config_model* conf, struct grid_cal_model* cal) {

  asc_state = grid_platform_allocate_volatile(16 * sizeof(struct grid_asc));
  memset(asc_state, 0, 16 * sizeof(struct grid_asc));

  if (grid_hwcfg_module_is_po16_reverse_polarity(sys)) {
    element_invert_bm = 0b1111111111111111;
  }

  for (int i = 0; i < GRID_MODULE_PO16_POTMETER_COUNT; ++i) {
    grid_ui_potmeter_state_init(ui, i, GRID_AIN_INTERNAL_RESOLUTION, GRID_POTMETER_DEADZONE, GRID_POTMETER_CENTER);
  }

  grid_asc_array_set_factors(asc_state, 16, 0, 16, 1);

  grid_config_init(conf, cal);

  grid_cal_init(cal, ui->element_list_length, GRID_AIN_INTERNAL_RESOLUTION);

  for (int i = 0; i < GRID_MODULE_PO16_POTMETER_COUNT; ++i) {
    struct grid_ui_element* ele = &ui->element_list[i];
    struct grid_ui_potmeter_state* state = (struct grid_ui_potmeter_state*)ele->primary_state;
    assert(grid_cal_set(cal, i, GRID_CAL_LIMITS, &state->limits) == 0);
    assert(grid_cal_set(cal, i, GRID_CAL_CENTER, &state->center) == 0);
    assert(grid_cal_set(cal, i, GRID_CAL_DETENT, &state->detent) == 0);
  }

  assert(grid_ui_bulk_conf_init(ui, GRID_UI_BULK_CONFREAD_PROGRESS, 0, NULL) == 0);
  grid_ui_bulk_confread_next(ui);

  grid_d51_adc_init(adc, 0b11111111, po16_process_analog);
  grid_d51_adc_start(adc);
}
