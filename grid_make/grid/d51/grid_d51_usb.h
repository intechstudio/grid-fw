/*
 * grid_d51_usb.h
 *
 * Created: 6/3/2020 5:02:04 PM
 *  Author: WPC-User
 */ 


#ifndef GRID_D51_USB_H_
#define GRID_D51_USB_H_

#include "grid_led.h"
#include "../usb/class/midi/device/audiodf_midi.h"
#include "../usb/class/cdc/device/cdcdf_acm.h"
#include "../usb/class/hid/device/hiddf_mouse.h"
#include "../usb/class/hid/device/hiddf_keyboard.h"

#include "config/usbd_config.h"

#include "grid_buf.h"

#include "grid_port.h"


extern void grid_platform_sync1_pulse_send();

volatile uint16_t grid_usb_rx_double_buffer_index;

volatile uint8_t grid_usb_serial_rx_buffer[CONF_USB_COMPOSITE_CDC_ACM_DATA_BULKIN_MAXPKSZ];


volatile uint8_t grid_usb_serial_rx_flag;
volatile uint16_t grid_usb_serial_rx_size;


static uint8_t *cdcdf_demo_buf;

// SERIAL CALLBACK HANDLERS
static bool grid_usb_serial_bulkout_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count);
static bool grid_usb_serial_bulkin_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count);
static bool grid_usb_serial_statechange_cb(usb_cdc_control_signal_t state);


int32_t grid_platform_usb_serial_write(char* buffer, uint32_t length);

// MIDI CALLBACK HANDLERS
static bool grid_usb_midi_bulkout_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count);
static bool grid_usb_midi_bulkin_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count);


void grid_d51_usb_init(void);

int32_t grid_platform_usb_midi_write(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3);
int32_t grid_platform_usb_midi_write_status(void);




int32_t grid_platform_usb_mouse_button_change(uint8_t b_state, uint8_t type);
int32_t grid_platform_usb_mouse_move(int8_t position, uint8_t axis);


int32_t grid_platform_usb_keyboard_keys_state_change(struct grid_usb_keyboard_event_desc* active_key_list, uint8_t keys_count);

#endif /* GRID_D51_USB_H_ */