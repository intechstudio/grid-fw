#ifndef GRID_PLATFORM_H
#define GRID_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

extern void grid_platform_printf(char const* fmt, ...);

extern uint32_t grid_platform_get_id(uint32_t* return_array);

extern uint32_t grid_platform_get_hwcfg();

extern uint32_t grid_platform_get_hwcfg_bit(uint8_t n);

extern uint8_t grid_platform_get_random_8();

extern uint8_t grid_platform_get_reset_cause();

extern uint64_t grid_platform_rtc_get_micros();

extern uint64_t grid_platform_rtc_get_elapsed_time(uint64_t told);

extern void grid_platform_nvm_defrag();

extern void grid_platform_system_reset();

extern uint32_t grid_platform_get_frame_len(uint8_t dir);

extern void grid_platform_send_frame(void* swsr, uint32_t size, uint8_t dir);

extern uint8_t grid_platform_reset_grid_transmitter(uint8_t direction);

extern int32_t grid_platform_usb_serial_ready();

extern int32_t grid_platform_usb_serial_write(char* buffer, uint32_t length);

extern uint32_t grid_plaform_get_nvm_nextwriteoffset();

extern void* grid_platform_allocate_volatile(size_t size);

#endif /* GRID_PLATFORM_H */
