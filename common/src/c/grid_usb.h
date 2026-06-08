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

void grid_usb_infrastructure_init(void);

bool grid_usb_connected(void);
void grid_usb_task(void);
void grid_usb_on_connect(void);
void grid_usb_on_disconnect(void);

#endif /* GRID_USB_H */
