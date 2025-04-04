#ifndef GRID_ROLLID_H
#define GRID_ROLLID_H

#include <stdint.h>

#include "grid_protocol.h"

enum { ROLLID_MAX = GRID_PARAMETER_SPI_ROLLING_ID_maximum };

struct grid_rollid {
  uint8_t last_send;
  uint8_t last_recv;
  uint8_t errors;
};

void grid_rollid_init(struct grid_rollid* rollid);
void grid_rollid_recv(struct grid_rollid* rollid, uint8_t recv);
uint8_t grid_rollid_send(struct grid_rollid* rollid);

#endif /* GRID_ROLLID_H */
