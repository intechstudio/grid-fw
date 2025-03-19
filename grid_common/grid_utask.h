#ifndef GRID_UTASK_H
#define GRID_UTASK_H

#include <stdbool.h>

#include "grid_platform.h"

struct grid_utask_timer {
  uint64_t last;
  uint64_t period;
};

bool grid_utask_timer_elapsed(struct grid_utask_timer* timer);

#endif /* GRID_UTASK_H */
