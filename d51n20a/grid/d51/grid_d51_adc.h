#ifndef GRID_D51_ADC_H
#define GRID_D51_ADC_H

#include <stdint.h>

#include "grid_ain.h"

struct grid_d51_adc_model {
  uint8_t mux_index;
  uint8_t mux_positions_bm;
  volatile uint8_t adc_complete_count;
  grid_process_analog_t process_analog;
};

extern struct grid_d51_adc_model grid_d51_adc_state;

void grid_d51_adc_init(struct grid_d51_adc_model* adc, uint8_t mux_positions_bm, grid_process_analog_t process_analog);
void grid_d51_adc_mux_init(struct grid_d51_adc_model* adc, uint8_t mux_positions_bm);
void grid_d51_adc_start(struct grid_d51_adc_model* adc);

#endif
