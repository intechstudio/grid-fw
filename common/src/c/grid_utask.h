#ifndef GRID_UTASK_H
#define GRID_UTASK_H

#include <stdbool.h>
#include <stdint.h>

// clang-format off
#define GRID_UTASK_DISABLED { .last = 0, .period = UINT64_MAX }
// clang-format on

struct grid_utask_timer {
  uint64_t last;
  uint64_t period;
};

bool grid_utask_timer_elapsed(struct grid_utask_timer* timer);
void grid_utask_timer_realign(struct grid_utask_timer* timer);
bool grid_utask_timer_enabled(struct grid_utask_timer* timer);
void grid_utask_timer_disable(struct grid_utask_timer* timer);
void grid_utask_timer_oneshot(struct grid_utask_timer* timer);

#endif /* GRID_UTASK_H */
