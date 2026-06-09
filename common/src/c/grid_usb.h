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

#include "grid_usb_acm.h"
#include "grid_usb_hid.h"
#include "grid_usb_midi.h"

extern uint64_t grid_platform_rtc_get_elapsed_time(uint64_t told);

extern void grid_platform_sync1_pulse_send();

struct grid_usb_model {
  struct grid_usb_acm_model acm;
  struct grid_usb_midi_model midi;
  struct grid_usb_hid_model hid;
};

extern struct grid_usb_model grid_usb_state;

void grid_usb_init(uint16_t vid, uint16_t pid, const char* serial);

bool grid_usb_connected(void);
void grid_usb_task(void);

#endif /* GRID_USB_H */
