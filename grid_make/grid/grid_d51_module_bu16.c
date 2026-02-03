#include "grid_d51_module_bu16.h"

#include "grid_ain.h"
#include "grid_asc.h"
#include "grid_platform.h"
#include "grid_ui_button.h"
#include "grid_ui_system.h"

static volatile uint8_t adc_complete_count = 0;

static uint8_t multiplexer_index = 0;
static const uint8_t mux_positions_bm = 0b11111111;
#define GRID_MODULE_BU16_BUT_NUM 16

static const uint8_t mux_element_lookup[2][8] = {
    {0, 1, 4, 5, 8, 9, 12, 13},   // MUX_0 -> ADC_1
    {2, 3, 6, 7, 10, 11, 14, 15}, // MUX_1 -> ADC_0
};
static uint16_t element_invert_bm = 0;

static struct adc_async_descriptor* adcs[2] = {&ADC_1, &ADC_0};

static struct grid_ui_button_state ui_button_state[GRID_MODULE_BU16_BUT_NUM] = {0};
static struct grid_asc asc_state[16] = {0};
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

    grid_ui_button_store_input(&grid_ui_state, element_index, &ui_button_state[element_index], processed, GRID_AIN_INTERNAL_RESOLUTION);
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

void grid_module_bu16_init() {

  grid_module_bu16_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);

  for (int i = 0; i < GRID_MODULE_BU16_BUT_NUM; ++i) {
    grid_ui_button_state_init(&ui_button_state[i], GRID_AIN_INTERNAL_RESOLUTION, 0.5, 0.2);
  }

  grid_asc_array_set_factors(asc_state, 16, 0, 16, 1);

  elements = grid_ui_model_get_elements(&grid_ui_state);

  hardware_init();
  hardware_start_transfer();
};
