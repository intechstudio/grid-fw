#include "tusb.h"

#include "grid_usb_hid.h"

void grid_usb_hid_init(struct grid_usb_hid_model* hid) {
  grid_usb_keyboard_init(&hid->keyboard);
  grid_usb_mouse_init(&hid->mouse);
  grid_usb_macro_init(&hid->macro, GRID_MACRO_TX_BUFFER_SIZE, &hid->keyboard, &hid->mouse);
  grid_usb_gamepad_init(&hid->gamepad, GRID_GAMEPAD_TX_BUFFER_SIZE);
}

void grid_usb_hid_on_connect(struct grid_usb_hid_model* hid) {
  grid_usb_macro_on_connect(&hid->macro);
  grid_usb_keyboard_on_connect(&hid->keyboard);
  grid_usb_mouse_on_connect(&hid->mouse);
  grid_usb_gamepad_on_connect(&hid->gamepad);
}

void grid_usb_hid_on_disconnect(struct grid_usb_hid_model* hid) {
  grid_usb_macro_on_disconnect(&hid->macro);
  grid_usb_keyboard_on_disconnect(&hid->keyboard);
  grid_usb_mouse_on_disconnect(&hid->mouse);
  grid_usb_gamepad_on_disconnect(&hid->gamepad);
}

static const uint8_t s_hid_report_desc[] = {GRID_HID_REPORT_DESC_CONTENT};

uint8_t const* tud_hid_descriptor_report_cb(uint8_t) { return s_hid_report_desc; }

uint16_t tud_hid_get_report_cb(uint8_t, uint8_t, hid_report_type_t, uint8_t*, uint16_t) { return 0; }

void tud_hid_set_report_cb(uint8_t, uint8_t, hid_report_type_t, uint8_t const*, uint16_t) {}
