#include "grid_d51_module_bu16.h"

#include "grid_ain.h"
#include "grid_d51_adc.h"
#include "grid_platform.h"
#include "grid_ui_button.h"

static struct grid_ui_model* ui_ptr = NULL;

static const uint8_t mux_element_lookup[2][8] = {
    {0, 1, 4, 5, 8, 9, 12, 13},   // MUX_0 -> ADC_1
    {2, 3, 6, 7, 10, 11, 14, 15}, // MUX_1 -> ADC_0
};
static uint16_t element_invert_bm = 0;

static void bu16_process_analog(struct grid_adc_result* result) {

  uint8_t element_index = mux_element_lookup[result->channel][result->mux_state];
  uint16_t inverted = GRID_ADC_INVERT_COND(result->value, element_index, element_invert_bm);

  struct grid_ui_element* ele = &ui_ptr->element_list[element_index];
  grid_ui_button_store_input(grid_ui_button_get_state(ele), inverted);
}

void grid_d51_module_bu16_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_d51_adc_model* adc, struct grid_config_model* conf, struct grid_cal_model* cal) {

  ui_ptr = ui;

  for (int i = 0; i < ui->element_list_length; ++i) {
    struct grid_ui_element* ele = &ui->element_list[i];
    if (ele->type == GRID_PARAMETER_ELEMENT_BUTTON) {
      struct grid_ui_button_state* state = (struct grid_ui_button_state*)ele->primary_state;
      grid_ui_button_configure(state, GRID_AIN_INTERNAL_RESOLUTION, 0.5, 0.2);
    }
  }

  grid_d51_adc_init(adc, 0b11111111, bu16_process_analog);
  grid_d51_adc_start(adc);
}
