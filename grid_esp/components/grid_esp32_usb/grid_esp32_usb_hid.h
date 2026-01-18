/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>
#include "tinyusb.h"

#ifdef __cplusplus
extern "C" {
#endif

/* HID Mouse Class Pointer Move Type */
enum mouse_move_type { X_AXIS_MV = 0x01, Y_AXIS_MV = 0x02, SCROLL_MV = 0x03 };

enum gamepad_axis_t { GAMEPAD_AXIS_X = 0, GAMEPAD_AXIS_Y, GAMEPAD_AXIS_Z, GAMEPAD_AXIS_RX, GAMEPAD_AXIS_RY, GAMEPAD_AXIS_RZ };

#if CFG_TUD_HID

#include "class/hid/hid_device.h"

/**
 * @brief HID report descriptor
 *
 * Keyboard + Mouse + Gamepad HID device
 * Defined as a macro so size can be computed at compile time
 */
#define HID_REPORT_DESCRIPTOR_CONTENT \
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD)), \
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE)), \
    TUD_HID_REPORT_DESC_GAMEPAD(HID_REPORT_ID(3))

extern const uint8_t hid_report_descriptor[];

// Use sizeof on the actual array in grid_esp32_usb.c
// For descriptor registration, use this computed size
#define HID_REPORT_DESCRIPTOR_SIZE sizeof((uint8_t[]){HID_REPORT_DESCRIPTOR_CONTENT})

#endif // CFG_TUD_HID

// Platform API
int32_t grid_platform_usb_mouse_button_change(uint8_t b_state, uint8_t type);
int32_t grid_platform_usb_mouse_move(int8_t position, uint8_t axis);
int32_t grid_platform_usb_gamepad_axis_move(uint8_t axis, int32_t value);
int32_t grid_platform_usb_gamepad_button_change(uint8_t button, uint8_t value);

struct grid_usb_keyboard_event_desc;
int32_t grid_platform_usb_keyboard_keys_state_change(struct grid_usb_keyboard_event_desc* active_key_list, uint8_t keys_count);

#ifdef __cplusplus
}
#endif
