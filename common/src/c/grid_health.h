#ifndef GRID_HEALTH_H
#define GRID_HEALTH_H

#include <stdint.h>

enum grid_health_counter {
  GRID_HEALTH_TX_DROPPED_CDC = 0,
  GRID_HEALTH_TX_DROPPED_MIDI,
  GRID_HEALTH_TX_DROPPED_MACRO,
  GRID_HEALTH_TX_DROPPED_GAMEPAD,
  GRID_HEALTH_TX_DROPPED_KEYBOARD,
  GRID_HEALTH_TX_DROPPED_MOUSE,
  GRID_HEALTH_COUNTER_COUNT,
};

struct grid_health_model {
  uint32_t counters[GRID_HEALTH_COUNTER_COUNT];
};

extern struct grid_health_model grid_health_state;

void grid_health_init(struct grid_health_model* health);
void grid_health_record(struct grid_health_model* health, enum grid_health_counter counter);
void grid_health_report(struct grid_health_model* health);

#endif /* GRID_HEALTH_H */
