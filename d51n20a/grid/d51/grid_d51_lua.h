#ifndef GRID_D51_LUA_H_INCLUDED
#define GRID_D51_LUA_H_INCLUDED

#include <sys/time.h>
#include <sys/times.h>

int _gettimeofday(struct timeval* tv, void* tz);
int _open(void);
clock_t _times(struct tms* buf);
int _unlink(void);
int _link(void);

#endif
