#ifndef GRID_USB_HID_H
#define GRID_USB_HID_H

#include <stdbool.h>
#include <stdint.h>

#include "grid_swsr.h"

#if CFG_TUD_HID

#define GRID_HID_REPORT_DESC_EXTRA , TUD_HID_REPORT_DESC_GAMEPAD(HID_REPORT_ID(3))

#define GRID_HID_REPORT_DESC_CONTENT TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD)), TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE)) GRID_HID_REPORT_DESC_EXTRA

#define GRID_HID_REPORT_DESC_SIZE sizeof((uint8_t[]){GRID_HID_REPORT_DESC_CONTENT})

#endif // CFG_TUD_HID

#define GRID_MACRO_TX_BUFFER_SIZE 800

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

#define GRID_KEYBOARD_KEY_maxcount 6

struct grid_mouse_model; // defined in grid_usb_mouse.h

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

extern struct grid_macro_model grid_macro_state;
extern struct grid_keyboard_model grid_keyboard_state;

uint8_t grid_usb_macro_tx_push(struct grid_macro_model* usb_macro, struct grid_macro_event_desc event);
void grid_usb_macro_tx_flush(struct grid_macro_model* usb_macro);
bool grid_usb_macro_tx_available(struct grid_macro_model* usb_macro);

void grid_usb_hid_on_connect(struct grid_macro_model* usb_macro, struct grid_keyboard_model* usb_keyboard);
void grid_usb_hid_on_disconnect(struct grid_macro_model* usb_macro, struct grid_keyboard_model* usb_keyboard);

void grid_usb_macro_init(struct grid_macro_model* usb_macro, uint16_t buffer_size, struct grid_keyboard_model* usb_keyboard, struct grid_mouse_model* usb_mouse);
void grid_usb_keyboard_init(struct grid_keyboard_model* usb_keyboard);

void grid_usb_keyboard_enable(struct grid_keyboard_model* usb_keyboard);
void grid_usb_keyboard_disable(struct grid_keyboard_model* usb_keyboard);

#endif /* GRID_USB_HID_H */
