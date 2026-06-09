#include "grid_health.h"

#include <string.h>

#include "grid_port.h"

struct grid_health_model grid_health_state = {0};

static const char* const s_counter_labels[GRID_HEALTH_COUNTER_COUNT] = {
    "CDC TX dropped",
    "MIDI TX dropped",
    "HID macro TX dropped",
    "HID gamepad TX dropped",
    "keyboard TX dropped",
    "mouse TX dropped",
};

void grid_health_init(struct grid_health_model* health) { memset(health, 0, sizeof(*health)); }

void grid_health_record(struct grid_health_model* health, enum grid_health_counter counter) { health->counters[counter]++; }

void grid_health_report(struct grid_health_model* health) {
  for (int i = 0; i < GRID_HEALTH_COUNTER_COUNT; i++) {
    if (health->counters[i] > 0) {
      grid_port_debug_printf("%s: %lu\n", s_counter_labels[i], (unsigned long)health->counters[i]);
      health->counters[i] = 0;
    }
  }
}
