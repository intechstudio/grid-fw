#include "grid_utask.h"

#include <limits.h>

#include "grid_platform.h"

bool grid_utask_timer_elapsed(struct grid_utask_timer* timer) {

  uint64_t now = grid_platform_rtc_get_micros();

  bool ret = grid_platform_rtc_get_diff(now, timer->last) > timer->period;

  if (ret) {
    timer->last = now;
  }

  return ret;
}

void grid_utask_timer_realign(struct grid_utask_timer* timer) { timer->last = grid_platform_rtc_get_micros(); }

bool grid_utask_timer_enabled(struct grid_utask_timer* timer) { return timer->period != UINT64_MAX; }

void grid_utask_timer_disable(struct grid_utask_timer* timer) { timer->period = UINT64_MAX; }

void grid_utask_timer_oneshot(struct grid_utask_timer* timer) {

  grid_utask_timer_realign(timer);

  timer->period = 0;
}
