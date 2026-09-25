#ifndef GRID_RP2350_ENCODER_H
#define GRID_RP2350_ENCODER_H

#include <stdint.h>

#include "grid_ui_encoder.h"

struct grid_rp2350_encoder_model {

  uint8_t rx_length;
  uint8_t* rx_buffer;
  uint8_t* rx_buffer2;

  grid_process_encoder_t process_encoder;
};

extern struct grid_rp2350_encoder_model grid_rp2350_encoder_state;

void grid_rp2350_encoder_init(struct grid_rp2350_encoder_model* enc, uint8_t transfer_length, uint32_t clock_rate, grid_process_encoder_t process_encoder);
void grid_rp2350_encoder_start(struct grid_rp2350_encoder_model* enc);

#endif /* GRID_RP2350_ENCODER_H */
