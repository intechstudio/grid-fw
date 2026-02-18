#include "grid_d51_module_ef44.h"

#include "grid_ain.h"
#include "grid_d51_adc.h"
#include "grid_d51_encoder.h"
#include "grid_platform.h"
#include "grid_ui_button.h"
#include "grid_ui_encoder.h"
#include "grid_ui_potmeter.h"

#include "grid_cal.h"
#include "grid_config.h"

#include <assert.h>

#define GRID_MODULE_EF44_ENCODER_COUNT 4

static const uint8_t mux_element_lookup[2][2] = {
    {4, 5}, // MUX_0 -> ADC_1
    {6, 7}, // MUX_1 -> ADC_0
};
static uint16_t element_invert_bm = 0;

static struct grid_ui_model* ui_ptr = NULL;

static void ef44_process_encoder(struct grid_encoder_result* result) {

  uint8_t encoder_position_lookup[GRID_MODULE_EF44_ENCODER_COUNT] = {2, 3, 0, 1};

  for (uint8_t i = 0; i < GRID_MODULE_EF44_ENCODER_COUNT; i++) {

    assert(i / 2 < result->length);
    uint8_t nibble = GRID_UI_ENCODER_NIBBLE_FROM_BUFFER(result->data, i);
    uint8_t element_index = encoder_position_lookup[i];

    struct grid_ui_encoder_sample sample = GRID_UI_ENCODER_SAMPLE_FROM_NIBBLE(nibble);
    struct grid_ui_element* ele = &ui_ptr->element_list[element_index];
    grid_ui_encoder_store_input(grid_ui_encoder_get_state(ele), sample);
  }
}

static void ef44_process_analog(struct grid_adc_result* result) {

  uint8_t element_index = mux_element_lookup[result->channel][result->mux_state];
  uint16_t inverted = GRID_ADC_INVERT_COND(result->value, element_index, element_invert_bm);

  struct grid_ui_element* ele = &ui_ptr->element_list[element_index];
  grid_ui_potmeter_store_input(grid_ui_potmeter_get_state(ele), inverted);
}

void grid_d51_module_ef44_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_d51_adc_model* adc, struct grid_d51_encoder_model* enc, struct grid_config_model* conf,
                               struct grid_cal_model* cal) {

  ui_ptr = ui;

  uint8_t detent = grid_hwcfg_module_encoder_is_detent(sys);
  int8_t direction = grid_hwcfg_module_encoder_dir(sys);

  grid_config_init(conf, cal);
  grid_cal_init(cal, ui->element_list_length, GRID_AIN_INTERNAL_RESOLUTION);

  for (int i = 0; i < ui->element_list_length; ++i) {
    struct grid_ui_element* ele = &ui->element_list[i];
    if (ele->type == GRID_PARAMETER_ELEMENT_POTMETER) {
      struct grid_ui_potmeter_state* state = grid_ui_potmeter_get_state(ele);
      grid_ui_potmeter_configure(state, GRID_AIN_INTERNAL_RESOLUTION, GRID_POTMETER_DEADZONE, GRID_POTMETER_CENTER);
      grid_cal_attach(cal, i, GRID_CAL_LIMITS, &state->limits);
    } else if (ele->type == GRID_PARAMETER_ELEMENT_ENCODER) {
      grid_ui_encoder_configure(grid_ui_encoder_get_state(ele), detent, direction);
    }
  }

  assert(grid_ui_bulk_start_with_state(&grid_ui_state, grid_ui_bulk_conf_read, 0, 0, NULL));
  grid_ui_bulk_flush(&grid_ui_state);

  uint8_t transfer_length = 1 + GRID_MODULE_EF44_ENCODER_COUNT / 2;
  uint32_t clock_rate = 1000 * transfer_length * 8;
  grid_d51_encoder_init(enc, transfer_length, clock_rate, ef44_process_encoder);
  grid_d51_adc_init(adc, 0b00000011, ef44_process_analog);
  grid_d51_adc_start(adc);
}
