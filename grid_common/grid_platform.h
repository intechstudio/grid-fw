#ifndef GRID_PLATFORM_H
#define GRID_PLATFORM_H

#include <stdint.h>

extern uint32_t grid_platform_get_id(uint32_t* return_array);

extern uint32_t grid_platform_get_hwcfg();

extern uint8_t grid_platform_get_random_8();

extern uint8_t grid_platform_get_reset_cause();

extern uint64_t grid_platform_rtc_get_micros();

extern uint64_t grid_platform_rtc_get_elapsed_time(uint64_t told);

extern void grid_platform_nvm_defrag();

#endif /* GRID_PLATFORM_H */