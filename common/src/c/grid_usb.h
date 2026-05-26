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

// Language-server-friendly defaults — overridden by platform tusb_config.h
#ifndef CFG_TUD_CDC
#define CFG_TUD_CDC 1
#endif
#ifndef CFG_TUD_MIDI
#define CFG_TUD_MIDI 1
#endif
#ifndef CFG_TUD_HID
#define CFG_TUD_HID 1
#endif

#include "grid_sys.h"
#include "grid_transport.h"
#include "grid_usb_hid.h"
#include "grid_usb_midi.h"

extern uint64_t grid_platform_rtc_get_elapsed_time(uint64_t told);

extern void grid_platform_sync1_pulse_send();

void grid_usb_infrastructure_init(void);

#endif /* GRID_USB_H */
