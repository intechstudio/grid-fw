#include "grid_utask.h"

#include "grid_platform.h"

bool grid_utask_timer_elapsed(struct grid_utask_timer* timer) {

  uint64_t now = grid_platform_rtc_get_micros();

  bool ret = grid_platform_rtc_get_elapsed_time(timer->last) >= timer->period;

  if (ret) {
    timer->last = now;
  }

  return ret;
}
