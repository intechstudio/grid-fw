#pragma once

#include <stdint.h>

#include "grid_usb.h"

int32_t grid_platform_usb_mouse_button_change(uint8_t b_state, uint8_t type);
int32_t grid_platform_usb_mouse_move(int8_t position, uint8_t axis);

int32_t grid_platform_usb_gamepad_axis_move(uint8_t axis, int32_t value);
int32_t grid_platform_usb_gamepad_button_change(uint8_t button, uint8_t value);

int32_t grid_platform_usb_keyboard_keys_state_change(struct grid_usb_keyboard_event_desc* active_key_list, uint8_t keys_count);
