#ifndef GRID_USB_HID_H
#define GRID_USB_HID_H

#include <stdbool.h>
#include <stdint.h>

#include "grid_swsr.h"

enum mouse_move_type { MOUSE_AXIS_X = 0x01, MOUSE_AXIS_Y = 0x02, MOUSE_AXIS_SCROLL = 0x03, MOUSE_AXIS_COUNT = 0x04 };

enum gamepad_axis_t { GAMEPAD_AXIS_X = 0, GAMEPAD_AXIS_Y, GAMEPAD_AXIS_Z, GAMEPAD_AXIS_RX, GAMEPAD_AXIS_RY, GAMEPAD_AXIS_RZ, GAMEPAD_AXIS_COUNT };

#if CFG_TUD_HID

#define GRID_HID_REPORT_DESC_EXTRA , TUD_HID_REPORT_DESC_GAMEPAD(HID_REPORT_ID(3))

#define GRID_HID_REPORT_DESC_CONTENT TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD)), TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE)) GRID_HID_REPORT_DESC_EXTRA

#define GRID_HID_REPORT_DESC_SIZE sizeof((uint8_t[]){GRID_HID_REPORT_DESC_CONTENT})

#endif // CFG_TUD_HID

#define GRID_MACRO_TX_BUFFER_SIZE 800
#define GRID_GAMEPAD_TX_BUFFER_SIZE 64

enum grid_macro_event_type {
  GRID_MACRO_EVENT_TYPE_KEY = 0,          // regular (non-modifier) key press/release
  GRID_MACRO_EVENT_TYPE_MODIFIER = 1,     // modifier key press/release (Ctrl, Shift, …)
  GRID_MACRO_EVENT_TYPE_MOUSE_MOVE = 2,   // mouse axis movement
  GRID_MACRO_EVENT_TYPE_MOUSE_BUTTON = 3, // mouse button press/release
  GRID_MACRO_EVENT_TYPE_DELAY = 0xf,      // timed delay between events
};

struct grid_macro_event_desc {

  uint8_t keycode;
  enum grid_macro_event_type type;
  uint8_t ispressed;
  uint32_t delay;
};

enum grid_gamepad_event_type {
  GRID_GAMEPAD_EVENT_AXIS = 0,
  GRID_GAMEPAD_EVENT_BUTTON = 1,
};

struct grid_gamepad_event_desc {
  uint8_t type;
  uint8_t index;
  uint8_t value; // axis: int8_t encoded as value+128; button: 0/1
  uint8_t _pad;
};

#define GRID_KEYBOARD_KEY_maxcount 6

struct grid_macro_model {
  struct grid_swsr_t tx;
  struct grid_macro_event_desc next;
  bool has_next;
  uint64_t tx_rtc_lasttimestamp;
  uint32_t tx_dropped;
  struct grid_keyboard_model* keyboard;
  struct grid_mouse_model* mouse;
};

struct grid_keyboard_model {
  struct grid_macro_event_desc active_key_list[GRID_KEYBOARD_KEY_maxcount];
  uint8_t active_key_count;
  uint8_t isenabled;
};

struct grid_keyboard_report {
  uint8_t modifier_bm;
  uint8_t keycode[6];
};

struct grid_mouse_model {
  uint8_t buttons;
};

struct grid_gamepad_model {
  struct grid_swsr_t tx;
  uint32_t tx_dropped;
  uint32_t buttons;
  int8_t axis[GAMEPAD_AXIS_COUNT];
  uint8_t hat;
};

extern struct grid_macro_model grid_macro_state;
extern struct grid_keyboard_model grid_keyboard_state;
extern struct grid_mouse_model grid_mouse_state;
extern struct grid_gamepad_model grid_gamepad_state;

int32_t grid_usb_gamepad_axis_move(struct grid_gamepad_model* usb_gamepad, uint8_t axis, int32_t value);
int32_t grid_usb_gamepad_button_change(struct grid_gamepad_model* usb_gamepad, uint8_t button, uint8_t value);

uint8_t grid_usb_gamepad_tx_push(struct grid_gamepad_model* usb_gamepad, struct grid_gamepad_event_desc event);
void grid_usb_gamepad_tx_flush(struct grid_gamepad_model* usb_gamepad);
bool grid_usb_gamepad_tx_available(struct grid_gamepad_model* usb_gamepad);

uint8_t grid_usb_macro_tx_push(struct grid_macro_model* usb_macro, struct grid_macro_event_desc event);
void grid_usb_macro_tx_flush(struct grid_macro_model* usb_macro);
bool grid_usb_macro_tx_available(struct grid_macro_model* usb_macro);

void grid_usb_macro_init(struct grid_macro_model* usb_macro, uint16_t buffer_size, struct grid_keyboard_model* usb_keyboard, struct grid_mouse_model* usb_mouse);
void grid_usb_keyboard_init(struct grid_keyboard_model* usb_keyboard);
void grid_usb_mouse_init(struct grid_mouse_model* usb_mouse);
void grid_usb_gamepad_init(struct grid_gamepad_model* usb_gamepad, uint16_t buffer_size);

void grid_usb_keyboard_enable(struct grid_keyboard_model* usb_keyboard);
void grid_usb_keyboard_disable(struct grid_keyboard_model* usb_keyboard);

#endif /* GRID_USB_HID_H */
