#include "tusb.h"

#include "grid_usb_mouse.h"

#include "grid_usb.h"

int32_t grid_usb_mouse_button_change(struct grid_mouse_model* mouse, uint8_t b_state, uint8_t type) {

  if (!tud_hid_ready()) {
    return 1;
  }

  if (b_state) {
    mouse->buttons |= type;
  } else {
    mouse->buttons &= (uint8_t)~type;
  }

  return 0 == tud_hid_mouse_report(HID_ITF_PROTOCOL_MOUSE, mouse->buttons, 0, 0, 0, 0);
}

int32_t grid_usb_mouse_move(struct grid_mouse_model* mouse, int8_t position, enum mouse_move_type axis) {

  if (!tud_hid_ready()) {
    return 1;
  }

  int8_t delta[3] = {0};

  if (axis < MOUSE_AXIS_X || axis >= MOUSE_AXIS_COUNT) {
    return 0;
  }

  delta[axis - 1] = position;

  return 0 == tud_hid_mouse_report(HID_ITF_PROTOCOL_MOUSE, mouse->buttons, delta[0], delta[1], delta[2], 0);
}

void grid_usb_mouse_init(struct grid_mouse_model* mouse) { mouse->buttons = 0; }

void grid_usb_mouse_on_connect(struct grid_mouse_model* mouse) { (void)mouse; }

void grid_usb_mouse_on_disconnect(struct grid_mouse_model* mouse) { mouse->buttons = 0; }
