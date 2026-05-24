/*
 * grid_d51_usb.h — TinyUSB-based USB for Grid SAMD51 firmware.
 *
 * Replaces the former ASF4/Atmel START composite USB stack.
 */
#pragma once

#include <stdint.h>

#include "tusb.h"

#include "grid_usb.h"

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------

void grid_d51_usb_init(void);

// ---------------------------------------------------------------------------
// CDC (virtual serial)
// ---------------------------------------------------------------------------

int32_t grid_platform_usb_serial_ready(void);
int32_t grid_platform_usb_serial_write(char* buffer, uint32_t length);

// ---------------------------------------------------------------------------
// MIDI
// ---------------------------------------------------------------------------

/** Poll TinyUSB MIDI RX buffer and push packets into grid_midi_rx. */
void grid_d51_midi_rx_poll(void);

int32_t grid_platform_usb_midi_write(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3);
int32_t grid_platform_usb_midi_write_status(void);

// ---------------------------------------------------------------------------
// HID
// ---------------------------------------------------------------------------

int32_t grid_platform_usb_mouse_button_change(uint8_t b_state, uint8_t type);
int32_t grid_platform_usb_mouse_move(int8_t position, uint8_t axis);

int32_t grid_platform_usb_gamepad_axis_move(uint8_t axis, int32_t value);
int32_t grid_platform_usb_gamepad_button_change(uint8_t button, uint8_t value);

int32_t grid_platform_usb_keyboard_keys_state_change(struct grid_usb_keyboard_event_desc* active_key_list, uint8_t keys_count);
