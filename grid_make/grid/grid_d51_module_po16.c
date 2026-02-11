#include "grid_d51_module_po16.h"

#include <string.h>

#include "grid_ain.h"
#include "grid_platform.h"
#include "grid_ui_potmeter.h"
#include "grid_ui_system.h"

#include "grid_asc.h"
#include "grid_cal.h"
#include "grid_config.h"

#include <assert.h>

static volatile uint8_t adc_complete_count = 0;

static uint8_t multiplexer_index = 0;
static const uint8_t mux_positions_bm = 0b11111111;
#define GRID_MODULE_PO16_POTMETER_COUNT 16

static const uint8_t mux_element_lookup[2][8] = {
    {0, 1, 4, 5, 8, 9, 12, 13},   // MUX_0 -> ADC_1
    {2, 3, 6, 7, 10, 11, 14, 15}, // MUX_1 -> ADC_0
};
static uint16_t element_invert_bm = 0;

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
    uint16_t inverted = GRID_ADC_INVERT_COND(raw, element_index, element_invert_bm);
    uint16_t downsampled = GRID_ADC_DOWNSAMPLE(inverted);

    uint16_t processed;
    if (!grid_asc_process(asc_state, element_index, downsampled, &processed)) {
      continue;
    }

    grid_ui_potmeter_store_input(&grid_ui_state, element_index, processed);
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

void grid_d51_module_po16_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_config_model* conf, struct grid_cal_model* cal) {

  asc_state = grid_platform_allocate_volatile(16 * sizeof(struct grid_asc));
  memset(asc_state, 0, 16 * sizeof(struct grid_asc));

  if (grid_hwcfg_module_is_po16_reverse_polarity(sys)) {
    element_invert_bm = 0b1111111111111111;
  }

  for (int i = 0; i < GRID_MODULE_PO16_POTMETER_COUNT; ++i) {
    struct grid_ui_element* ele = &ui->element_list[i];
    struct grid_ui_potmeter_state* state = (struct grid_ui_potmeter_state*)ele->primary_state;
    grid_ui_potmeter_state_init(state, GRID_AIN_INTERNAL_RESOLUTION, GRID_POTMETER_DEADZONE, GRID_POTMETER_CENTER);
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

  assert(grid_ui_bulk_start_with_state(&grid_ui_state, grid_ui_bulk_conf_read, 0, 0, NULL));
  grid_ui_bulk_flush(&grid_ui_state);

  hardware_init();
  hardware_start_transfer();
}
