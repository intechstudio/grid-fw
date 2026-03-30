#include "grid_rollid.h"

void grid_rollid_init(struct grid_rollid* rollid) {

  rollid->last_send = UINT8_MAX;
  rollid->last_recv = UINT8_MAX;
  rollid->errors = 0;
}

void grid_rollid_recv(struct grid_rollid* rollid, uint8_t recv) {

  uint8_t expect = (rollid->last_recv + 1) % ROLLID_MAX;

  if (recv != expect) {
    if (rollid->errors < UINT8_MAX) {
      ++rollid->errors;
    }
  }

  rollid->last_recv = recv;
}

uint8_t grid_rollid_send(struct grid_rollid* rollid) {

  rollid->last_send = (rollid->last_send + 1) % ROLLID_MAX;

  return rollid->last_send;
}
