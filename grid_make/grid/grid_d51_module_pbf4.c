#include "grid_d51_module_pbf4.h"

#include "grid_ui_button.h"
#include "grid_ui_potmeter.h"
#include "grid_ui_system.h"

static volatile uint8_t adc_complete_count = 0;

static uint8_t multiplexer_index = 0;
static const uint8_t multiplexer_lookup[16] = {0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15};

#define GRID_MODULE_PBF4_BUT_NUM 4

#define GRID_MODULE_PBF4_POT_NUM 8

static struct grid_ui_button_state ui_button_state[GRID_MODULE_PBF4_BUT_NUM] = {0};
static uint64_t potmeter_last_real_time[GRID_MODULE_PBF4_POT_NUM] = {0};
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

  // FAKE CALIBRATION to compensate oversampling and decimation
  uint32_t input_0 = adcresult_0 * 1.03;
  if (input_0 > (1 << 16) - 1) {
    input_0 = (1 << 16) - 1;
  }
  adcresult_0 = input_0;

  uint32_t input_1 = adcresult_1 * 1.03;
  if (input_1 > (1 << 16) - 1) {
    input_1 = (1 << 16) - 1;
  }
  adcresult_1 = input_1;

  if (adc_index_0 < 8 || adc_index_0 > 13) {
    // mux position is valid

    if (adc_index_0 > 13) {

      // adjust button index
      adc_index_0 -= 4;
      adc_index_1 -= 4;
    }

    if (adc_index_0 < 4) {

      // adjust potentiometer polarity
      adcresult_0 = 65535 - adcresult_0;
      adcresult_1 = 65535 - adcresult_1;
    }

    struct grid_ui_element* ele_0 = &elements[adc_index_0];

    if (adc_index_0 > 7) {
      grid_ui_button_store_input(ele_0, &ui_button_state[adc_index_0 - 8], adcresult_0, 16);
    } else {
      grid_ui_potmeter_store_input(ele_0, adc_index_0, &potmeter_last_real_time[adc_index_0], adcresult_0, 16);
    }

    struct grid_ui_element* ele_1 = &elements[adc_index_1];

    if (adc_index_1 > 7) {
      grid_ui_button_store_input(ele_1, &ui_button_state[adc_index_1 - 8], adcresult_1, 16);
    } else {
      grid_ui_potmeter_store_input(ele_1, adc_index_1, &potmeter_last_real_time[adc_index_1], adcresult_1, 16);
    }
  }

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

  for (int i = 0; i < GRID_MODULE_PBF4_BUT_NUM; ++i) {
    grid_ui_button_state_init(&ui_button_state[i], 16, 0.5, 0.2);
  }

  elements = grid_ui_model_get_elements(&grid_ui_state);

  hardware_init();
  hardware_start_transfer();
}
