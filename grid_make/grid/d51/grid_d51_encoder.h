#ifndef GRID_D51_ENCODER_H
#define GRID_D51_ENCODER_H

#include <stdint.h>

typedef void (*grid_d51_process_encoder_t)(void* user);

struct grid_d51_encoder_model {
  uint8_t tx_buffer[14];
  uint8_t rx_buffer[14];
  grid_d51_process_encoder_t process_encoder;
};

extern struct grid_d51_encoder_model grid_d51_encoder_state;

void grid_d51_encoder_init(struct grid_d51_encoder_model* enc, grid_d51_process_encoder_t process_encoder);

#endif
