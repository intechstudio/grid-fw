#include "grid_d51_adc.h"

#include "grid_d51_module.h"
#include "grid_ain.h"
#include "grid_platform.h"

struct grid_d51_adc_model grid_d51_adc_state;

static struct adc_async_descriptor* adcs[2] = {&ADC_1, &ADC_0};

static void adc_transfer_complete_cb(void) {

  struct grid_d51_adc_model* adc = &grid_d51_adc_state;

  if (adc->adc_complete_count == 0) {
    adc->adc_complete_count++;
    return;
  }

  uint8_t mux_state = adc->mux_index;

  for (int channel = 0; channel < 2; channel++) {
    uint16_t raw = 0;
    adc_async_read_channel(adcs[channel], 0, &raw, 2);

    struct grid_d51_adc_result result = {
        .channel = channel,
        .mux_state = mux_state,
        .value = raw,
    };

    adc->process_analog(&result);
  }

  GRID_MUX_INCREMENT(adc->mux_index, adc->mux_positions_bm);
  grid_platform_mux_write(adc->mux_index);

  adc->adc_complete_count = 0;

  adc_async_start_conversion(&ADC_0);
  adc_async_start_conversion(&ADC_1);
}

void grid_d51_adc_init(struct grid_d51_adc_model* adc, uint8_t mux_positions_bm, grid_d51_process_analog_t process_analog) {

  adc->process_analog = process_analog;
  adc->adc_complete_count = 0;

  adc_async_register_callback(&ADC_0, 0, ADC_ASYNC_CONVERT_CB, adc_transfer_complete_cb);
  adc_async_register_callback(&ADC_1, 0, ADC_ASYNC_CONVERT_CB, adc_transfer_complete_cb);

  adc_async_enable_channel(&ADC_0, 0);
  adc_async_enable_channel(&ADC_1, 0);

  grid_platform_mux_init(mux_positions_bm);
}

void grid_d51_adc_mux_init(struct grid_d51_adc_model* adc, uint8_t mux_positions_bm) {

  adc->mux_positions_bm = mux_positions_bm;
  GRID_MUX_FIRST_VALID(adc->mux_index, adc->mux_positions_bm);
  grid_platform_mux_write(adc->mux_index);
}

void grid_d51_adc_start(struct grid_d51_adc_model* adc) {

  adc_async_start_conversion(&ADC_0);
  adc_async_start_conversion(&ADC_1);
}
