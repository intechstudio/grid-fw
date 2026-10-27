#ifndef GRID_RP2350_ADC_H
#define GRID_RP2350_ADC_H

#include <stdint.h>

#include "grid_ain.h"

struct grid_rp2350_adc_model {

  uint8_t mux_index;
  uint8_t mux_positions_bm;
  grid_process_analog_t process_analog;
};

extern struct grid_rp2350_adc_model grid_rp2350_adc_state;

void grid_rp2350_adc_init(struct grid_rp2350_adc_model* adc, uint8_t mux_positions_bm, grid_process_analog_t process_analog);

void grid_rp2350_adc_mux_init(struct grid_rp2350_adc_model* adc, uint8_t mux_positions_bm);

void grid_rp2350_adc_start(struct grid_rp2350_adc_model* adc);

#endif /* GRID_RP2350_ADC_H */
