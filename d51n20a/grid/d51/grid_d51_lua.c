/*
 * grid_d51_lua.c
 *
 * Created: 4/12/2019 5:27:13 PM
 * Author : SUKU WC
 */

#include "grid_d51_lua.h"

#include <time.h>

#include "grid_platform.h"

int _gettimeofday(struct timeval* tv, void* tz) {
  uint64_t micros = grid_platform_rtc_get_micros();
  tv->tv_sec = micros / 1000000ULL;
  tv->tv_usec = micros % 1000000ULL;
  return 0;
}
int _open() {
  while (1)
    ;
}
clock_t _times(struct tms* buf) {
  clock_t ticks = (clock_t)(grid_platform_rtc_get_micros() / (1000000ULL / CLOCKS_PER_SEC));
  buf->tms_utime = ticks;
  buf->tms_stime = 0;
  buf->tms_cutime = 0;
  buf->tms_cstime = 0;
  return ticks;
}
int _unlink() {
  while (1)
    ;
}
int _link() {
  while (1)
    ;
}
