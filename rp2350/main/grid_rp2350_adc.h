#ifndef GRID_RP2350_ADC_H
#define GRID_RP2350_ADC_H

#include <stdint.h>

#include "grid_ain.h"

// Hardware oversampling factor: at each mux position the ADC takes this many
// round-robin passes over the four channels before advancing, and the per-sample
// results are averaged in the ISR (mean stays a 12-bit value, sqrt(N) less noise).
//
// Why 8x:
//  - Floor requirement is the per-input sample rate of the ESP32 platform, since
//    the velocity-sensitive analog buttons need enough temporal resolution for
//    slope (dv/dt) calculation. The ESP32-S3 ULP driver was measured at ~2516
//    Hz/input (nomux mode) -- see grid_esp32_adc.c. 8x keeps us above that.
//  - Noise matters as much as rate for slope work: averaging error propagates
//    straight into dv/dt. 8x cuts noise by sqrt(8) ~= 2.8x (observed: floating
//    inputs settle from +/-2-3 LSB down to +/-1 LSB).
//
// Measured on RP2350 (16 inputs = 4 channels x 4 mux positions):
//  -  1x (no oversampling): 20833 Hz/input, 333 k conv/s
//  -  8x (this setting):     3658 Hz/input, 468 k conv/s, ~273 us between samples
// Oversampling costs less than linear (8x samples -> only 5.7x slower output)
// because the fixed ~4 us/sweep stop-mux-restart overhead is amortized over the
// longer dwell; raw ADC throughput actually rises toward the ~2 us/conv floor.
//
// Retune here if the velocity-button work needs a different rate/resolution
// trade: higher N = more resolution + lower noise but a lower output rate.
#define GRID_RP2350_ADC_OVERSAMPLE 8

// Interrupt-driven ADC + multiplexer scanner for RP2350, structured after the
// D51 driver (d51n20a/grid/d51/grid_d51_adc.c). One ADC round-robins the four
// common lines (AIN0..3 on GPIO26..29) while two 74HC4052 muxes fan each line
// out to four inputs via the shared A1:A0 address lines (GPIO24:GPIO25).
//
// Unlike the D51 driver there is no first-sample discard: each sweep is a burst
// of exactly four DMA-captured conversions after which the ADC is stopped, so
// it sits idle while the mux switches and settles before the next explicit
// start. See grid_rp2350_adc.c for the burst/restart flow.

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
