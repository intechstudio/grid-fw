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

#include "grid_sys.h"
#include "grid_transport.h"

extern int32_t grid_platform_usb_midi_write(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3);
extern int32_t grid_platform_usb_midi_write_status(void);
extern int32_t grid_platform_usb_mouse_button_change(uint8_t b_state, uint8_t type);
extern int32_t grid_platform_usb_mouse_move(int8_t position, uint8_t axis);

int32_t grid_platform_usb_gamepad_axis_move(uint8_t axis, int32_t value);
int32_t grid_platform_usb_gamepad_button_change(uint8_t button, uint8_t value);

extern uint64_t grid_platform_rtc_get_elapsed_time(uint64_t told);

extern void grid_platform_sync1_pulse_send();

void grid_usb_midi_buffer_init();

enum grid_usb_keyboard_key_state_t { GRID_USB_KEYBOARD_KEY_STATEUP, GRID_USB_KEYBOARD_KEY_STATEDOWN };

// USB MIDI Code Index Number (CIN) values
enum grid_midi_cin_type {
  GRID_MIDI_CIN_MISC = 0x00,             // Miscellaneous function codes
  GRID_MIDI_CIN_CABLE_EVENT = 0x01,      // Cable events
  GRID_MIDI_CIN_SYSCOM_2BYTE = 0x02,     // Two-byte System Common (MTC, SongSelect)
  GRID_MIDI_CIN_SYSCOM_3BYTE = 0x03,     // Three-byte System Common (SPP)
  GRID_MIDI_CIN_SYSEX_START = 0x04,      // SysEx starts or continues (3 bytes)
  GRID_MIDI_CIN_SYSEX_END_1BYTE = 0x05,  // SysEx ends with 1 byte or single-byte System Common
  GRID_MIDI_CIN_SYSEX_END_2BYTE = 0x06,  // SysEx ends with 2 bytes
  GRID_MIDI_CIN_SYSEX_END_3BYTE = 0x07,  // SysEx ends with 3 bytes
  GRID_MIDI_CIN_NOTE_OFF = 0x08,         // Note-off
  GRID_MIDI_CIN_NOTE_ON = 0x09,          // Note-on
  GRID_MIDI_CIN_POLY_KEYPRESS = 0x0A,    // Poly-KeyPress
  GRID_MIDI_CIN_CONTROL_CHANGE = 0x0B,   // Control Change
  GRID_MIDI_CIN_PROGRAM_CHANGE = 0x0C,   // Program Change
  GRID_MIDI_CIN_CHANNEL_PRESSURE = 0x0D, // Channel Pressure
  GRID_MIDI_CIN_PITCHBEND = 0x0E,        // PitchBend Change
  GRID_MIDI_CIN_SINGLE_BYTE = 0x0F       // Single Byte (including Real-Time)
};

// MIDI System messages
enum grid_midi_system_type { GRID_MIDI_SYSEX_START = 0xF0, GRID_MIDI_SYSEX_END = 0xF7 };

// MIDI Real-Time Message bytes
enum grid_midi_rtm_type {
  GRID_MIDI_RTM_TIMING_CLOCK = 0xF8,
  GRID_MIDI_RTM_UNDEFINED_F9 = 0xF9,
  GRID_MIDI_RTM_START = 0xFA,
  GRID_MIDI_RTM_CONTINUE = 0xFB,
  GRID_MIDI_RTM_STOP = 0xFC,
  GRID_MIDI_RTM_UNDEFINED_FD = 0xFD,
  GRID_MIDI_RTM_ACTIVE_SENSING = 0xFE,
  GRID_MIDI_RTM_SYSTEM_RESET = 0xFF
};

struct grid_midi_event_desc {

  uint8_t byte0;
  uint8_t byte1;
  uint8_t byte2;
  uint8_t byte3;
};

#define GRID_MIDI_TX_BUFFER_length 128

extern struct grid_midi_event_desc grid_midi_tx_buffer[GRID_MIDI_TX_BUFFER_length];
extern uint16_t grid_midi_tx_write_index;
extern uint16_t grid_midi_tx_read_index;

uint8_t grid_midi_tx_push(struct grid_midi_event_desc midi_event);
void grid_midi_tx_pop();
bool grid_midi_tx_readable();

#define GRID_MIDI_RX_BUFFER_length 128

extern struct grid_midi_event_desc grid_midi_rx_buffer[GRID_MIDI_RX_BUFFER_length];
extern uint16_t grid_midi_rx_write_index;
extern uint16_t grid_midi_rx_read_index;

void grid_midi_rx_push(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3);
void grid_midi_rx_pop();
bool grid_midi_rx_writable();

// MIDI SysEx RX buffer (raw bytes)
#define GRID_MIDI_SYSEX_RX_BUFFER_length 256

// MIDI Real-Time Message RX buffer (raw bytes)
#define GRID_MIDI_RTM_RX_BUFFER_length 32

struct grid_usb_keyboard_event_desc {

  uint8_t keycode;
  uint8_t ismodifier;
  uint8_t ispressed;
  uint32_t delay;
};

/** Describes the USB HID Keyboard Key descriptors. */
struct grid_usb_hid_kb_desc {
  /* HID Key Value, defined in usb_protocol_hid.h */
  uint8_t key_id;
  /* Flag whether it is a modifier key */
  bool b_modifier;
  /* Key State */
  enum grid_usb_keyboard_key_state_t state;
};

#define GRID_KEYBOARD_KEY_maxcount 6

struct grid_usb_keyboard_model {

  uint8_t tx_buffer_length;
  struct grid_usb_keyboard_event_desc* tx_buffer;

  uint16_t tx_write_index;
  uint16_t tx_read_index;
  uint64_t tx_rtc_lasttimestamp;

  struct grid_usb_keyboard_event_desc active_key_list[GRID_KEYBOARD_KEY_maxcount];
  uint8_t active_key_count;

  uint8_t isenabled;
};

uint8_t grid_usb_keyboard_tx_push(struct grid_usb_keyboard_model* kb, struct grid_usb_keyboard_event_desc keyboard_event);
void grid_usb_keyboard_tx_pop(struct grid_usb_keyboard_model* kb);
bool grid_usb_keyboard_tx_readable(struct grid_usb_keyboard_model* kb);

extern struct grid_usb_keyboard_model grid_usb_keyboard_state;

void grid_usb_keyboard_model_init(struct grid_usb_keyboard_model* kb, uint8_t buffer_length);

uint8_t grid_usb_keyboard_cleanup(struct grid_usb_keyboard_model* kb);

int32_t grid_usb_keyboard_keychange(struct grid_usb_keyboard_model* kb, struct grid_usb_keyboard_event_desc* key);

void grid_usb_gamepad_axis_move(uint8_t axis, int32_t move);
void grid_usb_gamepad_button_change(uint8_t button, uint8_t value);

void grid_usb_keyboard_enable(struct grid_usb_keyboard_model* kb);
void grid_usb_keyboard_disable(struct grid_usb_keyboard_model* kb);
uint8_t grid_usb_keyboard_isenabled(struct grid_usb_keyboard_model* kb);

#endif /* GRID_USB_H */
