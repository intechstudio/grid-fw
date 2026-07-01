#ifndef GRID_USB_HID_H
#define GRID_USB_HID_H

#include "grid_usb_gamepad.h"
#include "grid_usb_keyboard.h"
#include "grid_usb_mouse.h"

#define GRID_HID_REPORT_DESC_EXTRA , TUD_HID_REPORT_DESC_GAMEPAD(HID_REPORT_ID(3))

#define GRID_HID_REPORT_DESC_CONTENT TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD)), TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE)) GRID_HID_REPORT_DESC_EXTRA

#define GRID_HID_REPORT_DESC_SIZE sizeof((uint8_t[]){GRID_HID_REPORT_DESC_CONTENT})

struct grid_usb_hid_model {
  struct grid_macro_model macro;
  struct grid_keyboard_model keyboard;
  struct grid_mouse_model mouse;
  struct grid_gamepad_model gamepad;
};

void grid_usb_hid_init(struct grid_usb_hid_model* hid);
void grid_usb_hid_on_connect(struct grid_usb_hid_model* hid);
void grid_usb_hid_on_disconnect(struct grid_usb_hid_model* hid);

#endif /* GRID_USB_HID_H */
