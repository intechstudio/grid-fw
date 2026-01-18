/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_usb_hid.h"

#include "esp_log.h"
#include "rom/ets_sys.h"
#include "tinyusb.h"

#include "grid_usb.h"

static const char* TAG = "USB_HID";

#if CFG_TUD_HID

/**
 * @brief HID report descriptor
 *
 * Keyboard + Mouse + Gamepad HID device
 */
const uint8_t hid_report_descriptor[] = {HID_REPORT_DESCRIPTOR_CONTENT};

/********* TinyUSB HID callbacks ***************/

// Invoked when received GET HID REPORT DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long
// enough for transfer to complete
uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance) {
  // We use only one interface and one HID report descriptor, so we can ignore
  // parameter 'instance'
  return hid_report_descriptor;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)bufsize;
}

/********* HID Platform API ***************/

enum mouse_button_type { LEFT_BTN = 0x01, RIGHT_BTN = 0x02, MIDDLE_BTN = 0x04 };

static uint8_t hid_mouse_button_state = 0;

static uint32_t hid_gamepad_button_state = 0;

static int8_t hid_gamepad_axis_x = 0;
static int8_t hid_gamepad_axis_y = 0;
static int8_t hid_gamepad_axis_z = 0;

static int8_t hid_gamepad_axis_rx = 0;
static int8_t hid_gamepad_axis_ry = 0;
static int8_t hid_gamepad_axis_rz = 0;

static uint8_t hid_gamepad_hat = 0;

int32_t grid_platform_usb_mouse_button_change(uint8_t b_state, uint8_t type) {

  if (b_state == 1) { // button pressed
    hid_mouse_button_state |= type;
  } else {
    hid_mouse_button_state &= ~type;
  }

  // report_id, buttons, dx, dy, wheel, pan
  return 0 == tud_hid_mouse_report(HID_ITF_PROTOCOL_MOUSE, hid_mouse_button_state, 0, 0, 0, 0);
}

int32_t grid_platform_usb_mouse_move(int8_t position, uint8_t axis) {

  int8_t delta_x = 0;
  int8_t delta_y = 0;
  int8_t wheel = 0;
  int8_t pan = 0; // not used

  if (axis == X_AXIS_MV) {
    delta_x = position;
  } else if (axis == Y_AXIS_MV) {
    delta_y = position;
  } else if (axis == SCROLL_MV) {
    wheel = position;
  } else {
    return 0; // Invalid axis
  }

  // report_id, buttons, dx, dy, wheel, pan
  return 0 == tud_hid_mouse_report(HID_ITF_PROTOCOL_MOUSE, hid_mouse_button_state, delta_x, delta_y, wheel, pan);
}

int32_t grid_platform_usb_gamepad_axis_move(uint8_t axis, int32_t value) {

  switch (axis) {
  case GAMEPAD_AXIS_X:
    hid_gamepad_axis_x = value;
    break;
  case GAMEPAD_AXIS_Y:
    hid_gamepad_axis_y = value;
    break;
  case GAMEPAD_AXIS_Z:
    hid_gamepad_axis_z = value;
    break;
  case GAMEPAD_AXIS_RX:
    hid_gamepad_axis_rx = value;
    break;
  case GAMEPAD_AXIS_RY:
    hid_gamepad_axis_ry = value;
    break;
  case GAMEPAD_AXIS_RZ:
    hid_gamepad_axis_rz = value;
    break;
  default:
    ets_printf("INVALID AXIS\r\n");
    return 0;
  }

  return 0 == tud_hid_gamepad_report(3, hid_gamepad_axis_x, hid_gamepad_axis_y, hid_gamepad_axis_z, hid_gamepad_axis_rz, hid_gamepad_axis_ry, hid_gamepad_axis_rx, hid_gamepad_hat,
                                     hid_gamepad_button_state);
}

int32_t grid_platform_usb_gamepad_button_change(uint8_t button, uint8_t value) {

  if (value) {
    hid_gamepad_button_state |= (1 << button);
  } else {
    hid_gamepad_button_state &= ~(1 << button);
  }

  return 0 == tud_hid_gamepad_report(3, hid_gamepad_axis_x, hid_gamepad_axis_y, hid_gamepad_axis_z, hid_gamepad_axis_rx, hid_gamepad_axis_ry, hid_gamepad_axis_rz, hid_gamepad_hat,
                                     hid_gamepad_button_state);
}

int32_t grid_platform_usb_keyboard_keys_state_change(struct grid_usb_keyboard_event_desc* active_key_list, uint8_t keys_count) {

  struct grid_usb_hid_kb_desc key_descriptor_array[GRID_KEYBOARD_KEY_maxcount];
  for (uint8_t i = 0; i < GRID_KEYBOARD_KEY_maxcount; i++) {

    key_descriptor_array[i].b_modifier = active_key_list[i].ismodifier;
    key_descriptor_array[i].key_id = active_key_list[i].keycode;
    key_descriptor_array[i].state = active_key_list[i].ispressed;
  }

  uint8_t keycode[6] = {0};

  uint8_t modifier = 0; // modifier flags

  if (keys_count == 0) {
    ESP_LOGD(TAG, "No Key Is Pressed");
  }

  for (uint8_t i = 0; i < keys_count; i++) {

    ESP_LOGD(TAG, "IsMod: %d, KeyCode: %d, State: %d", key_descriptor_array[i].b_modifier, key_descriptor_array[i].key_id, key_descriptor_array[i].state);

    if (key_descriptor_array[i].b_modifier) {

      modifier |= key_descriptor_array[i].key_id;
    } else if (key_descriptor_array[i].state && key_descriptor_array[i].key_id != 255) {

      keycode[keys_count] = key_descriptor_array[i].key_id;
      keys_count++;
    }
  }

  // Report Id, modifier, keycodearray
  return 0 == tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, modifier, keycode);
}

#else // !CFG_TUD_HID - stub implementations

int32_t grid_platform_usb_mouse_button_change(uint8_t b_state, uint8_t type) {
  (void)b_state;
  (void)type;
  return 0;
}

int32_t grid_platform_usb_mouse_move(int8_t position, uint8_t axis) {
  (void)position;
  (void)axis;
  return 0;
}

int32_t grid_platform_usb_gamepad_axis_move(uint8_t axis, int32_t value) {
  (void)axis;
  (void)value;
  return 0;
}

int32_t grid_platform_usb_gamepad_button_change(uint8_t button, uint8_t value) {
  (void)button;
  (void)value;
  return 0;
}

int32_t grid_platform_usb_keyboard_keys_state_change(struct grid_usb_keyboard_event_desc* active_key_list, uint8_t keys_count) {
  (void)active_key_list;
  (void)keys_count;
  return 0;
}

#endif // CFG_TUD_HID
