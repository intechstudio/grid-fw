/*
 * grid_usb.c
 *
 * Created: 7/6/2020 12:07:54 PM
 *  Author: suku
 *
 * USB functionality has been refactored into:
 *   grid_usb_midi.c  - MIDI buffer/TX/RX/SysEx/RTM
 *   grid_usb_hid.c   - HID keyboard model + gamepad + enable/disable
 */

#include "grid_usb.h"
#include "grid_protocol.h"
#include "grid_usb_acm.h"
#include "tusb.h"

bool grid_usb_connected(void) { return tud_mounted(); }

void grid_usb_task(void) { tud_task_ext(0, false); }

void grid_usb_infrastructure_init(void) {
  grid_usb_acm_init(&grid_usb_acm_state, GRID_PARAMETER_SPI_TRANSACTION_length * 2);
  grid_usb_midi_init(&grid_usb_midi_state, GRID_MIDI_TX_BUFFER_SIZE, GRID_MIDI_VOICE_RX_BUFFER_SIZE, GRID_MIDI_SYSEX_BUFFER_SIZE, GRID_MIDI_RTM_BUFFER_SIZE);
  grid_usb_keyboard_init(&grid_keyboard_state);
  grid_usb_mouse_init(&grid_mouse_state);
  grid_usb_macro_init(&grid_macro_state, GRID_MACRO_TX_BUFFER_SIZE, &grid_keyboard_state, &grid_mouse_state);
  grid_usb_gamepad_init(&grid_gamepad_state);
}
