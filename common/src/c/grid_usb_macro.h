#ifndef GRID_USB_MACRO_H
#define GRID_USB_MACRO_H

#include <stdbool.h>
#include <stdint.h>

#include "grid_swsr.h"

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

struct grid_keyboard_model; // defined in grid_usb_keyboard.h
struct grid_mouse_model;    // defined in grid_usb_mouse.h

struct grid_macro_model {
  struct grid_swsr_t tx;
  struct grid_macro_event_desc next;
  bool has_next;
  uint64_t tx_rtc_lasttimestamp;
  uint32_t tx_dropped;
  struct grid_keyboard_model* keyboard;
  struct grid_mouse_model* mouse;
};

void grid_usb_macro_init(struct grid_macro_model* usb_macro, uint16_t buffer_size, struct grid_keyboard_model* usb_keyboard, struct grid_mouse_model* usb_mouse);
void grid_usb_macro_on_connect(struct grid_macro_model* usb_macro);
void grid_usb_macro_on_disconnect(struct grid_macro_model* usb_macro);

uint8_t grid_usb_macro_tx_push(struct grid_macro_model* usb_macro, struct grid_macro_event_desc event);
void grid_usb_macro_tx_flush(struct grid_macro_model* usb_macro);
bool grid_usb_macro_tx_available(struct grid_macro_model* usb_macro);

#endif /* GRID_USB_MACRO_H */
