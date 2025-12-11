#include "grid_d51_module_po16.h"

#include "grid_ui_potmeter.h"
#include "grid_ui_system.h"

#include "grid_cal.h"
#include "grid_config.h"

static volatile uint8_t adc_complete_count = 0;

static uint8_t multiplexer_index = 0;
static const uint8_t multiplexer_lookup[16] = {0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15};

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

  /* Read conversion results */

  uint16_t adcresult_0 = 0;
  uint16_t adcresult_1 = 0;

  uint8_t adc_index_0 = multiplexer_lookup[multiplexer_index + 8];
  uint8_t adc_index_1 = multiplexer_lookup[multiplexer_index + 0];

  adc_async_read_channel(&ADC_0, 0, &adcresult_0, 2);
  adc_async_read_channel(&ADC_1, 0, &adcresult_1, 2);

  /* Update the multiplexer */

  multiplexer_index++;
  multiplexer_index %= 8;

  gpio_set_pin_level(MUX_A, multiplexer_index / 1 % 2);
  gpio_set_pin_level(MUX_B, multiplexer_index / 2 % 2);
  gpio_set_pin_level(MUX_C, multiplexer_index / 4 % 2);

  // POT POLARITY IS REVERSED ON PO16_RevC
  if (grid_hwcfg_module_is_po16_reverse_polarity(&grid_sys_state)) {

    // Reverse the 16bit result
    adcresult_0 = 65535 - adcresult_0;
    adcresult_1 = 65535 - adcresult_1;
  }

  struct grid_ui_element* ele_0 = &elements[adc_index_0];

  grid_ui_potmeter_store_input(ele_0, adc_index_0, &ui_potmeter_state[adc_index_0], adcresult_0 >> 4, 12);

  struct grid_ui_element* ele_1 = &elements[adc_index_1];

  grid_ui_potmeter_store_input(ele_1, adc_index_1, &ui_potmeter_state[adc_index_1], adcresult_1 >> 4, 12);

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

  for (int i = 0; i < GRID_MODULE_PO16_POT_NUM; ++i) {
    grid_ui_potmeter_state_init(&ui_potmeter_state[i], 12, 64, 2048);
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
