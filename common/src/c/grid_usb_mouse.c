#include "tusb.h"

#include "grid_usb_mouse.h"

#include "grid_usb.h"

#if CFG_TUD_HID

int32_t grid_usb_mouse_button_change(struct grid_mouse_model* usb_mouse, uint8_t b_state, uint8_t type) {
  if (!usb_mouse->tx_interface_ready()) {
    return 1;
  }
  if (b_state) {
    usb_mouse->buttons |= type;
  } else {
    usb_mouse->buttons &= (uint8_t)~type;
  }
  return 0 == tud_hid_mouse_report(HID_ITF_PROTOCOL_MOUSE, usb_mouse->buttons, 0, 0, 0, 0);
}

int32_t grid_usb_mouse_move(struct grid_mouse_model* usb_mouse, int8_t position, uint8_t axis) {
  if (!usb_mouse->tx_interface_ready()) {
    return 1;
  }
  int8_t delta[3] = {0};
  if (axis < MOUSE_AXIS_X || axis >= MOUSE_AXIS_COUNT) {
    return 0;
  }
  delta[axis - 1] = position;
  return 0 == tud_hid_mouse_report(HID_ITF_PROTOCOL_MOUSE, usb_mouse->buttons, delta[0], delta[1], delta[2], 0);
}

#else // !CFG_TUD_HID

int32_t grid_usb_mouse_button_change(struct grid_mouse_model* usb_mouse, uint8_t b_state, uint8_t type) {
  (void)usb_mouse;
  (void)b_state;
  (void)type;
  return 0;
}

int32_t grid_usb_mouse_move(struct grid_mouse_model* usb_mouse, int8_t position, uint8_t axis) {
  (void)usb_mouse;
  (void)position;
  (void)axis;
  return 0;
}

#endif // CFG_TUD_HID

void grid_usb_mouse_init(struct grid_mouse_model* usb_mouse, bool (*tx_interface_ready)(void)) {
  usb_mouse->buttons = 0;
  usb_mouse->tx_interface_ready = tx_interface_ready;
}

void grid_usb_mouse_on_connect(struct grid_mouse_model* usb_mouse) { (void)usb_mouse; }

void grid_usb_mouse_on_disconnect(struct grid_mouse_model* usb_mouse) { usb_mouse->buttons = 0; }
