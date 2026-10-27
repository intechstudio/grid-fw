#include "grid_rp2350_module_po16.h"

#include "grid_ui_potmeter.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "grid_ain.h"
#include "grid_asc.h"
#include "grid_platform.h"

#define GRID_MODULE_PO16_MUX_POSITIONS_BM 0x0F
#define GRID_MODULE_PO16_ASC_FACTOR 8

static struct grid_ui_model* ui_ptr = NULL;
static struct grid_asc* asc_array = NULL;
static uint8_t asc_array_length = 0;

static const uint8_t mux_element_lookup[4][4] = {
    {0, 1, 4, 5},
    {8, 9, 12, 13},
    {2, 3, 6, 7},
    {10, 11, 14, 15},
};
static uint16_t element_invert_bm = 0b1111111111111111;

GRID_IRAM_ATTR static void po16_process_analog(struct grid_adc_result* result) {

  assert(result);

  uint8_t element_index = mux_element_lookup[result->channel][result->mux_state];

  result->value = GRID_ADC_INVERT_COND(result->value, element_index, element_invert_bm);

  assert(element_index < asc_array_length);
  if (!grid_asc_process(&asc_array[element_index], result->value, &result->value)) {
    return;
  }

  struct grid_ui_element* ele = &ui_ptr->element_list[element_index];
  grid_ui_potmeter_store_input(grid_ui_potmeter_get_state(ele), result->value);
}

void grid_rp2350_module_po16_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_rp2350_adc_model* adc, struct grid_config_model* conf, struct grid_cal_model* cal) {

  ui_ptr = ui;

  asc_array_length = ui->element_list_length - 1;
  asc_array = grid_platform_allocate_volatile(asc_array_length * sizeof(struct grid_asc));
  memset(asc_array, 0, asc_array_length * sizeof(struct grid_asc));

  grid_config_init(conf, cal);
  grid_cal_init(cal, ui->element_list_length, GRID_AIN_INTERNAL_RESOLUTION);

  for (int i = 0; i < ui->element_list_length; ++i) {
    struct grid_ui_element* ele = &ui->element_list[i];
    if (ele->type == GRID_PARAMETER_ELEMENT_POTMETER) {
      struct grid_ui_potmeter_state* state = grid_ui_potmeter_get_state(ele);
      grid_ui_potmeter_state_init(state, GRID_AIN_INTERNAL_RESOLUTION, GRID_POTMETER_DEADZONE, GRID_POTMETER_CENTER);
      grid_asc_set_factor(&asc_array[i], GRID_MODULE_PO16_ASC_FACTOR);
      grid_cal_channel_set(cal, i, GRID_CAL_LIMITS, &state->limits);
      grid_cal_channel_set(cal, i, GRID_CAL_CENTER, &state->center);
      grid_cal_channel_set(cal, i, GRID_CAL_DETENT, &state->detent);
    }
  }

  grid_ui_bulk_start_with_state(ui, grid_ui_bulk_conf_read, 0, 0, NULL);
  grid_ui_bulk_flush(ui);

  grid_rp2350_adc_init(adc, GRID_MODULE_PO16_MUX_POSITIONS_BM, po16_process_analog);
  grid_rp2350_adc_mux_init(adc, GRID_MODULE_PO16_MUX_POSITIONS_BM);
  grid_rp2350_adc_start(adc);
}
