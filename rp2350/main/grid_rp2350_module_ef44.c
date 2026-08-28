#include "grid_rp2350_module_ef44.h"

#include "grid_ui_encoder.h"
#include "grid_ui_potmeter.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "grid_ain.h"
#include "grid_asc.h"
#include "grid_cal.h"
#include "grid_config.h"
#include "grid_platform.h"
#include "grid_sys.h"
#include "grid_ui.h"

#include "grid_rp2350_adc.h"
#include "grid_rp2350_encoder.h"

// This board has 4 direct ADC pins (no external mux, unlike BU16's two
// 74HC4052s), and its 4 encoders are read via a shift-register chain
// cascaded behind HWCFG's (see grid_rp2350_encoder.h) rather than ESP32's
// I2S-TDM or D51's dedicated SPI bus.
#define GRID_MODULE_EF44_ENCODER_COUNT 4
#define GRID_MODULE_EF44_ADC_CHANNELS 4
#define GRID_MODULE_EF44_ASC_FACTOR 16
#define GRID_MODULE_EF44_MUX_POSITIONS_BM 0x01

static struct grid_ui_model* ui_ptr = NULL;
static struct grid_asc* asc_array = NULL;
static uint8_t asc_array_length = 0;
static uint16_t element_invert_bm = 0;

// grid_module_ef44_ui_init lays out elements 0-3 as encoders, 4-7 as
// potmeters/faders, 8 as system. There's no mux stepping (mux_state always
// 0), so this is a straight 1D table rather than D51/ESP32's [channel][mux]
// table -- kept as an explicit lookup rather than computed arithmetic so a
// future PCB revision with a different channel order is a one-line change.
static const uint8_t mux_element_lookup[GRID_MODULE_EF44_ADC_CHANNELS] = {4, 5, 6, 7};

static void ef44_process_analog(struct grid_adc_result* result) {

  assert(result);

  uint8_t element_index = mux_element_lookup[result->channel];

  result->value = GRID_ADC_INVERT_COND(result->value, element_index, element_invert_bm);

  assert(element_index < asc_array_length);
  if (!grid_asc_process(&asc_array[element_index], result->value, &result->value)) {
    return;
  }

  struct grid_ui_element* ele = &ui_ptr->element_list[element_index];
  grid_ui_potmeter_store_input(grid_ui_potmeter_get_state(ele), result->value);
}

static void ef44_process_encoder(struct grid_encoder_result* result) {

  static const uint8_t encoder_lookup[GRID_MODULE_EF44_ENCODER_COUNT] = {2, 3, 0, 1};

  for (uint8_t i = 0; i < GRID_MODULE_EF44_ENCODER_COUNT; ++i) {

    assert(i / 2 < result->length);
    uint8_t nibble = GRID_UI_ENCODER_NIBBLE_FROM_BUFFER(result->data, i);
    uint8_t element_index = encoder_lookup[i];

    struct grid_ui_encoder_sample sample = GRID_UI_ENCODER_SAMPLE_FROM_NIBBLE(nibble);
    struct grid_ui_element* ele = &ui_ptr->element_list[element_index];
    grid_ui_encoder_store_input(grid_ui_encoder_get_state(ele), sample);
  }
}

void grid_rp2350_module_ef44_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_rp2350_adc_model* adc, struct grid_rp2350_encoder_model* enc, struct grid_config_model* conf,
                                   struct grid_cal_model* cal) {

  ui_ptr = ui;

  uint8_t detent = grid_hwcfg_module_encoder_is_detent(sys);
  int8_t direction = grid_hwcfg_module_encoder_dir(sys);

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
      grid_asc_set_factor(&asc_array[i], GRID_MODULE_EF44_ASC_FACTOR);
      grid_cal_channel_set(cal, i, GRID_CAL_LIMITS, &state->limits);
    } else if (ele->type == GRID_PARAMETER_ELEMENT_ENCODER) {
      grid_ui_encoder_state_init(grid_ui_encoder_get_state(ele), detent, direction);
    }
  }

  grid_ui_bulk_start_with_state(ui, grid_ui_bulk_conf_read, 0, 0, NULL);
  grid_ui_bulk_flush(ui);

  // 1 hwcfg byte (discarded) + 2 encoder-data bytes (4 encoders, 2 per byte).
  // 500 Hz target poll rate, matching D51/ESP32's EF44 encoder read rate.
  uint8_t transfer_length = 1 + GRID_MODULE_EF44_ENCODER_COUNT / 2;
  uint32_t clock_rate = 500 * transfer_length * 8;
  grid_rp2350_encoder_init(enc, transfer_length, clock_rate, ef44_process_encoder);

  grid_rp2350_adc_init(adc, GRID_MODULE_EF44_MUX_POSITIONS_BM, ef44_process_analog);
  grid_rp2350_adc_mux_init(adc, GRID_MODULE_EF44_MUX_POSITIONS_BM);

  grid_rp2350_encoder_start(enc);
  grid_rp2350_adc_start(adc);
}
