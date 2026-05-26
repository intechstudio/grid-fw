#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "grid_swsr.h"

enum mouse_move_type { X_AXIS_MV = 0x01, Y_AXIS_MV = 0x02, SCROLL_MV = 0x03, MOUSE_AXIS_COUNT = 0x04 };

enum gamepad_axis_t { GAMEPAD_AXIS_X = 0, GAMEPAD_AXIS_Y, GAMEPAD_AXIS_Z, GAMEPAD_AXIS_RX, GAMEPAD_AXIS_RY, GAMEPAD_AXIS_RZ, GAMEPAD_AXIS_COUNT };

#if CFG_TUD_HID

#define GRID_HID_REPORT_DESC_EXTRA , TUD_HID_REPORT_DESC_GAMEPAD(HID_REPORT_ID(3))

#define GRID_HID_REPORT_DESC_CONTENT TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD)), TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE)) GRID_HID_REPORT_DESC_EXTRA

#define GRID_HID_REPORT_DESC_SIZE sizeof((uint8_t[]){GRID_HID_REPORT_DESC_CONTENT})

#endif // CFG_TUD_HID

#define GRID_MACRO_TX_BUFFER_SIZE 800

struct grid_macro_event_desc {

  uint8_t keycode;
  uint8_t ismodifier;
  uint8_t ispressed;
  uint32_t delay;
};

#define GRID_KEYBOARD_KEY_maxcount 6

struct grid_macro_model {
  struct grid_swsr_t tx;
  struct grid_macro_event_desc next;
  bool has_next;
  uint64_t tx_rtc_lasttimestamp;
  struct grid_keyboard_model* keyboard;
  struct grid_mouse_model* mouse;
};

struct grid_keyboard_model {
  struct grid_macro_event_desc active_key_list[GRID_KEYBOARD_KEY_maxcount];
  uint8_t active_key_count;
  uint8_t isenabled;
};

struct grid_mouse_model {
  uint8_t buttons;
};

struct grid_gamepad_model {
  uint32_t buttons;
  int8_t axis[GAMEPAD_AXIS_COUNT];
  uint8_t hat;
};

extern struct grid_macro_model grid_macro_state;
extern struct grid_keyboard_model grid_keyboard_state;
extern struct grid_mouse_model grid_mouse_state;
extern struct grid_gamepad_model grid_gamepad_state;

int32_t grid_usb_gamepad_axis_move(struct grid_gamepad_model* gamepad, uint8_t axis, int32_t value);
int32_t grid_usb_gamepad_button_change(struct grid_gamepad_model* gamepad, uint8_t button, uint8_t value);

uint8_t grid_usb_macro_tx_push(struct grid_macro_model* macro, struct grid_macro_event_desc event);
void grid_usb_macro_tx_flush(struct grid_macro_model* macro);
bool grid_usb_macro_tx_available(struct grid_macro_model* macro);

void grid_usb_macro_init(struct grid_macro_model* macro, uint16_t buffer_size, struct grid_keyboard_model* keyboard, struct grid_mouse_model* mouse);
void grid_usb_keyboard_init(struct grid_keyboard_model* kb);
void grid_usb_mouse_init(struct grid_mouse_model* mouse);
void grid_usb_gamepad_init(struct grid_gamepad_model* gamepad);

void grid_usb_keyboard_enable(struct grid_keyboard_model* kb);
void grid_usb_keyboard_disable(struct grid_keyboard_model* kb);
