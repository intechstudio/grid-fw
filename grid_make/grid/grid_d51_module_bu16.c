#include "grid_d51_module_bu16.h"

#include <string.h>

#include "grid_ain.h"
#include "grid_asc.h"
#include "grid_d51_adc.h"
#include "grid_platform.h"
#include "grid_ui_button.h"
#include "grid_ui_system.h"

#define GRID_MODULE_BU16_BUTTON_COUNT 16

static const uint8_t mux_element_lookup[2][8] = {
    {0, 1, 4, 5, 8, 9, 12, 13},   // MUX_0 -> ADC_1
    {2, 3, 6, 7, 10, 11, 14, 15}, // MUX_1 -> ADC_0
};
static uint16_t element_invert_bm = 0;

static struct grid_asc* asc_state = NULL;

static void bu16_process_analog(void* user) {

  struct grid_d51_adc_result* result = (struct grid_d51_adc_result*)user;

  uint8_t element_index = mux_element_lookup[result->channel][result->mux_state];
  uint16_t inverted = GRID_ADC_INVERT_COND(result->value, element_index, element_invert_bm);
  uint16_t downsampled = GRID_ADC_DOWNSAMPLE(inverted);

  uint16_t processed;
  if (!grid_asc_process(asc_state, element_index, downsampled, &processed)) {
    return;
  }

  grid_ui_button_store_input(&grid_ui_state, element_index, processed);
}

void grid_d51_module_bu16_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_d51_adc_model* adc, struct grid_config_model* conf, struct grid_cal_model* cal) {

  asc_state = grid_platform_allocate_volatile(16 * sizeof(struct grid_asc));
  memset(asc_state, 0, 16 * sizeof(struct grid_asc));

  for (int i = 0; i < GRID_MODULE_BU16_BUTTON_COUNT; ++i) {
    struct grid_ui_element* ele = &ui->element_list[i];
    struct grid_ui_button_state* state = (struct grid_ui_button_state*)ele->primary_state;
    grid_ui_button_state_init(state, GRID_AIN_INTERNAL_RESOLUTION, 0.5, 0.2);
  }

  grid_asc_array_set_factors(asc_state, 16, 0, 16, 1);

  grid_d51_adc_init(adc, 0b11111111, bu16_process_analog);
  grid_d51_adc_start(adc);
}
