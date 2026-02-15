#ifndef GRID_D51_ENCODER_H
#define GRID_D51_ENCODER_H

#include <stdint.h>

#include "grid_ui_encoder.h"

struct grid_d51_encoder_model {
  uint8_t* rx_buffer;
  uint8_t transfer_length;
  grid_process_encoder_t process_encoder;
};

extern struct grid_d51_encoder_model grid_d51_encoder_state;

void grid_d51_encoder_init(struct grid_d51_encoder_model* enc, uint8_t transfer_length, uint32_t clock_rate, grid_process_encoder_t process_encoder);

#endif
