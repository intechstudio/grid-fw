#include "grid_d51_module_po16.h"

#include "grid_ain.h"
#include "grid_platform.h"
#include "grid_ui_potmeter.h"
#include "grid_ui_system.h"

#include "grid_cal.h"
#include "grid_config.h"

#include <assert.h>

static volatile uint8_t adc_complete_count = 0;

static uint8_t multiplexer_index = 0;
static const uint8_t mux_positions_bm = 0b11111111;
static const uint8_t multiplexer_lookup[16] = {2, 0, 3, 1, 6, 4, 7, 5, 10, 8, 11, 9, 14, 12, 15, 13};
static uint8_t invert_result_lookup[16] = {0};

#define GRID_MODULE_PO16_POT_NUM 16

static struct grid_ui_potmeter_state ui_potmeter_state[GRID_MODULE_PO16_POT_NUM] = {0};
static struct grid_ui_element* elements = NULL;

static void hardware_start_transfer(void) {

  adc_async_start_conversion(&ADC_0);
  adc_async_start_conversion(&ADC_1);
}

static void adc_transfer_complete_cb(void) {

  if (adc_complete_count == 0) {
    adc_complete_count++;
    return;
  }

  struct adc_async_descriptor* adcs[2] = {&ADC_0, &ADC_1};

  /* Read and process both channels */

  for (int i = 0; i < 2; i++) {
    uint16_t result = 0;
    adc_async_read_channel(adcs[i], 0, &result, 2);

    uint8_t lookup_index = multiplexer_index * 2 + i;
    uint8_t element_index = multiplexer_lookup[lookup_index];

    if (invert_result_lookup[lookup_index]) {
      result = GRID_ADC_INVERT(result);
    }

    struct grid_ui_element* ele = &elements[element_index];

    grid_ui_potmeter_store_input(ele, element_index, &ui_potmeter_state[element_index], result >> 4, 12);
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

void grid_module_po16_init() {

  grid_module_po16_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);

  uint8_t invert = grid_hwcfg_module_is_po16_reverse_polarity(&grid_sys_state);
  for (int i = 0; i < 16; ++i) {
    invert_result_lookup[i] = invert;
  }

  for (int i = 0; i < GRID_MODULE_PO16_POT_NUM; ++i) {
    grid_ui_potmeter_state_init(&ui_potmeter_state[i], GRID_AIN_INTERNAL_RESOLUTION, GRID_POTMETER_DEADZONE, GRID_POTMETER_CENTER);
  }

  elements = grid_ui_model_get_elements(&grid_ui_state);

  grid_config_init(&grid_config_state, &grid_cal_state);

  grid_cal_init(&grid_cal_state, grid_ui_state.element_list_length, 12);

  for (int i = 0; i < 16; ++i) {
    assert(grid_cal_set(&grid_cal_state, i, GRID_CAL_LIMITS, &ui_potmeter_state[i].limits) == 0);
    assert(grid_cal_set(&grid_cal_state, i, GRID_CAL_CENTER, &ui_potmeter_state[i].center) == 0);
    assert(grid_cal_set(&grid_cal_state, i, GRID_CAL_DETENT, &ui_potmeter_state[i].detent) == 0);
  }

  assert(grid_ui_bulk_conf_init(&grid_ui_state, GRID_UI_BULK_CONFREAD_PROGRESS, 0, NULL) == 0);
  grid_ui_bulk_confread_next(&grid_ui_state);

  hardware_init();
  hardware_start_transfer();
}
