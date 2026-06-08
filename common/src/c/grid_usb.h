/*
 * grid_usb.h
 *
 * Created: 7/6/2020 12:07:42 PM
 *  Author: suku
 */

#ifndef GRID_USB_H
#define GRID_USB_H

#include <stdbool.h>
#include <stdint.h>

extern uint64_t grid_platform_rtc_get_elapsed_time(uint64_t told);

extern void grid_platform_sync1_pulse_send();

void grid_usb_init(uint16_t vid, uint16_t pid, const char* serial);

bool grid_usb_connected(void);
void grid_usb_task(void);

#endif /* GRID_USB_H */
