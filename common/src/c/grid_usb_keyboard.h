#ifndef GRID_USB_KEYBOARD_H
#define GRID_USB_KEYBOARD_H

#include <stdint.h>

#include "grid_usb_macro.h"

#define GRID_KEYBOARD_KEY_maxcount 6

struct grid_keyboard_model {
  struct grid_macro_event_desc active_key_list[GRID_KEYBOARD_KEY_maxcount];
  uint8_t active_key_count;
  uint8_t isenabled;
};

struct grid_keyboard_report {
  uint8_t modifier_bm;
  uint8_t keycode[6];
};

void grid_usb_keyboard_init(struct grid_keyboard_model* usb_keyboard);
void grid_usb_keyboard_on_connect(struct grid_keyboard_model* usb_keyboard);
void grid_usb_keyboard_on_disconnect(struct grid_keyboard_model* usb_keyboard);

void grid_usb_keyboard_enable(struct grid_keyboard_model* usb_keyboard);
void grid_usb_keyboard_disable(struct grid_keyboard_model* usb_keyboard);

int32_t grid_usb_keyboard_keychange(struct grid_keyboard_model* usb_keyboard, struct grid_macro_event_desc* key);

#endif /* GRID_USB_KEYBOARD_H */
