#include "grid_d51_module_pbf4.h"

#include <string.h>

#include "grid_ain.h"
#include "grid_asc.h"
#include "grid_platform.h"
#include "grid_ui_button.h"
#include "grid_ui_potmeter.h"
#include "grid_ui_system.h"

#include "grid_cal.h"
#include "grid_config.h"

#include <assert.h>

static volatile uint8_t adc_complete_count = 0;

static uint8_t multiplexer_index = 0;
static const uint8_t mux_positions_bm = 0b11001111;
#define GRID_MODULE_PBF4_BUT_NUM 4
#define GRID_MODULE_PBF4_POT_NUM 8

#define X GRID_MUX_UNUSED
static const uint8_t mux_element_lookup[2][8] = {
    {0, 1, 4, 5, X, X, 8, 9},   // MUX_0 -> ADC_1
    {2, 3, 6, 7, X, X, 10, 11}, // MUX_1 -> ADC_0
};
#undef X
static uint16_t element_invert_bm = 0b0000000000001111;

static struct adc_async_descriptor* adcs[2] = {&ADC_1, &ADC_0};

static struct grid_asc* asc_state = NULL;

static void hardware_start_transfer(void) {

  adc_async_start_conversion(&ADC_0);
  adc_async_start_conversion(&ADC_1);
}

static void adc_transfer_complete_cb(void) {

  if (adc_complete_count == 0) {
    adc_complete_count++;
    return;
  }

  /* Read and process both channels */

  for (int channel = 0; channel < 2; channel++) {
    uint16_t raw = 0;
    adc_async_read_channel(adcs[channel], 0, &raw, 2);

    uint8_t element_index = mux_element_lookup[channel][multiplexer_index];
    assert(element_index != GRID_MUX_UNUSED);

    uint16_t inverted = GRID_ADC_INVERT_COND(raw, element_index, element_invert_bm);
    uint16_t downsampled = GRID_ADC_DOWNSAMPLE(inverted);

    uint16_t processed;
    if (!grid_asc_process(asc_state, element_index, downsampled, &processed)) {
      continue;
    }

    if (element_index >= GRID_MODULE_PBF4_POT_NUM) {
      grid_ui_button_store_input(&grid_ui_state, element_index, processed);
    } else {
      grid_ui_potmeter_store_input(&grid_ui_state, element_index, processed);
    }
  }

  /* Update the multiplexer for next iteration */

  GRID_MUX_INCREMENT(multiplexer_index, mux_positions_bm);
  grid_platform_mux_write(multiplexer_index);

  adc_complete_count = 0;
  hardware_start_transfer();
}

static void hardware_init(void) {

  adc_async_register_callback(&ADC_0, 0, ADC_ASYNC_CONVERT_CB, adc_transfer_complete_cb);
  adc_async_register_callback(&ADC_1, 0, ADC_ASYNC_CONVERT_CB, adc_transfer_complete_cb);

  adc_async_enable_channel(&ADC_0, 0);
  adc_async_enable_channel(&ADC_1, 0);
}

void grid_module_pbf4_init() {

  grid_module_pbf4_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);

  asc_state = grid_platform_allocate_volatile(12 * sizeof(struct grid_asc));
  memset(asc_state, 0, 12 * sizeof(struct grid_asc));

  // Buttons are elements 8-11
  for (int i = 0; i < GRID_MODULE_PBF4_BUT_NUM; ++i) {
    struct grid_ui_element* ele = &grid_ui_state.element_list[GRID_MODULE_PBF4_POT_NUM + i];
    struct grid_ui_button_state* state = (struct grid_ui_button_state*)ele->primary_state;
    grid_ui_button_state_init(state, GRID_AIN_INTERNAL_RESOLUTION, 0.5, 0.2);
  }

  // Potmeters are elements 0-7
  for (int i = 0; i < GRID_MODULE_PBF4_POT_NUM; ++i) {
    struct grid_ui_element* ele = &grid_ui_state.element_list[i];
    struct grid_ui_potmeter_state* state = (struct grid_ui_potmeter_state*)ele->primary_state;
    grid_ui_potmeter_state_init(state, GRID_AIN_INTERNAL_RESOLUTION, GRID_POTMETER_DEADZONE, GRID_POTMETER_CENTER);
  }

  grid_asc_array_set_factors(asc_state, 12, 0, 12, 1);

  grid_config_init(&grid_config_state, &grid_cal_state);

  grid_cal_init(&grid_cal_state, grid_ui_state.element_list_length, 12);

  // Potmeter calibration (elements 0-7, first 4 have center detent)
  for (int i = 0; i < GRID_MODULE_PBF4_POT_NUM; ++i) {
    struct grid_ui_element* ele = &grid_ui_state.element_list[i];
    struct grid_ui_potmeter_state* state = (struct grid_ui_potmeter_state*)ele->primary_state;
    assert(grid_cal_set(&grid_cal_state, i, GRID_CAL_LIMITS, &state->limits) == 0);
    if (i < 4) {
      assert(grid_cal_set(&grid_cal_state, i, GRID_CAL_CENTER, &state->center) == 0);
      assert(grid_cal_set(&grid_cal_state, i, GRID_CAL_DETENT, &state->detent) == 0);
    }
  }

  assert(grid_ui_bulk_conf_init(&grid_ui_state, GRID_UI_BULK_CONFREAD_PROGRESS, 0, NULL) == 0);
  grid_ui_bulk_confread_next(&grid_ui_state);

  hardware_init();
  hardware_start_transfer();
}
