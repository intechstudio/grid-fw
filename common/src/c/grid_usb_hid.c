#include "tusb.h"

#include "grid_usb_hid.h"

#if CFG_TUD_HID
static bool tx_interface_ready_impl(void) { return tud_hid_ready(); }
#else
static bool tx_interface_ready_impl(void) { return true; }
#endif

void grid_usb_hid_init(struct grid_usb_hid_model* usb_hid) {
  grid_usb_keyboard_init(&usb_hid->keyboard, tx_interface_ready_impl);
  grid_usb_mouse_init(&usb_hid->mouse, tx_interface_ready_impl);
  grid_usb_macro_init(&usb_hid->macro, GRID_MACRO_TX_BUFFER_SIZE, &usb_hid->keyboard, &usb_hid->mouse, tx_interface_ready_impl);
  grid_usb_gamepad_init(&usb_hid->gamepad, GRID_GAMEPAD_TX_BUFFER_SIZE, tx_interface_ready_impl);
}

void grid_usb_hid_on_connect(struct grid_usb_hid_model* usb_hid) {
  grid_usb_macro_on_connect(&usb_hid->macro);
  grid_usb_keyboard_on_connect(&usb_hid->keyboard);
  grid_usb_mouse_on_connect(&usb_hid->mouse);
  grid_usb_gamepad_on_connect(&usb_hid->gamepad);
}

void grid_usb_hid_on_disconnect(struct grid_usb_hid_model* usb_hid) {
  grid_usb_macro_on_disconnect(&usb_hid->macro);
  grid_usb_keyboard_on_disconnect(&usb_hid->keyboard);
  grid_usb_mouse_on_disconnect(&usb_hid->mouse);
  grid_usb_gamepad_on_disconnect(&usb_hid->gamepad);
}

#if CFG_TUD_HID

static const uint8_t s_hid_report_desc[] = {GRID_HID_REPORT_DESC_CONTENT};

uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance) {
  (void)instance;
  return s_hid_report_desc;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;
  return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)bufsize;
}

#endif // CFG_TUD_HID
