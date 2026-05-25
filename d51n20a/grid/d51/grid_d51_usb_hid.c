#include "grid_d51_usb_hid.h"

#include "class/hid/hid_device.h"
#include "tusb.h"

#include "grid_usb.h"

#define HID_REPORT_DESC_CONTENT                                                                                        \
  TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD)),                                             \
      TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE))

static const uint8_t s_hid_report_desc[] = {HID_REPORT_DESC_CONTENT};

#define HID_REPORT_DESC_LEN sizeof(s_hid_report_desc)

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

static uint8_t s_mouse_buttons = 0;

int32_t grid_platform_usb_mouse_button_change(uint8_t b_state, uint8_t type) {
  if (b_state) {
    s_mouse_buttons |= type;
  } else {
    s_mouse_buttons &= (uint8_t)~type;
  }
  return 0 == tud_hid_mouse_report(HID_ITF_PROTOCOL_MOUSE, s_mouse_buttons, 0, 0, 0, 0);
}

int32_t grid_platform_usb_mouse_move(int8_t position, uint8_t axis) {
  int8_t dx = 0, dy = 0, wheel = 0, pan = 0;

  switch (axis) {
  case 0x01:
    dx = position;
    break;
  case 0x02:
    dy = position;
    break;
  case 0x03:
    wheel = position;
    break;
  default:
    return 0;
  }
  return 0 == tud_hid_mouse_report(HID_ITF_PROTOCOL_MOUSE, s_mouse_buttons, dx, dy, wheel, pan);
}

int32_t grid_platform_usb_gamepad_axis_move(uint8_t axis, int32_t value) {
  (void)axis;
  (void)value;
  return -1;
}

int32_t grid_platform_usb_gamepad_button_change(uint8_t button, uint8_t value) {
  (void)button;
  (void)value;
  return -1;
}

int32_t grid_platform_usb_keyboard_keys_state_change(struct grid_usb_keyboard_event_desc* active_key_list, uint8_t keys_count) {
  uint8_t keycode[6] = {0};
  uint8_t modifier = 0;
  uint8_t key_idx = 0;

  for (uint8_t i = 0; i < keys_count && i < GRID_KEYBOARD_KEY_maxcount; i++) {
    if (active_key_list[i].ismodifier) {
      modifier |= active_key_list[i].keycode;
    } else if (active_key_list[i].ispressed && active_key_list[i].keycode != 255 && key_idx < 6) {
      keycode[key_idx++] = active_key_list[i].keycode;
    }
  }

  return 0 == tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, modifier, keycode);
}

int32_t grid_platform_websocket_ready(void) { return 0; }

int32_t grid_platform_websocket_write(char* buffer, uint32_t length) {
  (void)buffer;
  (void)length;
  return 0;
}
