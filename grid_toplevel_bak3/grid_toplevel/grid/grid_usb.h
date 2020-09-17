/*
 * grid_usb.h
 *
 * Created: 7/6/2020 12:07:42 PM
 *  Author: suku
 */ 


#ifndef GRID_USB_H_
#define GRID_USB_H_

#include "grid_module.h"

static uint8_t *cdcdf_demo_buf;
static bool grid_usb_serial_bulkout_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count);
static bool grid_usb_serial_bulkin_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count);
static bool grid_usb_serial_statechange_cb(usb_cdc_control_signal_t state);
void grid_usb_serial_init();


static bool grid_usb_midi_bulkout_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count);
static bool grid_usb_midi_bulkin_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count);

void grid_usb_midi_init();

#endif /* GRID_USB_H_ */