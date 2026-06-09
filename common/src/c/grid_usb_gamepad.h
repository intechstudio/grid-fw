#ifndef GRID_USB_GAMEPAD_H
#define GRID_USB_GAMEPAD_H

#include <stdbool.h>
#include <stdint.h>

#include "grid_swsr.h"

enum gamepad_axis_t {
  GAMEPAD_AXIS_X = 0,
  GAMEPAD_AXIS_Y,
  GAMEPAD_AXIS_Z,
  GAMEPAD_AXIS_RX,
  GAMEPAD_AXIS_RY,
  GAMEPAD_AXIS_RZ,
  GAMEPAD_AXIS_COUNT,
};

#define GRID_GAMEPAD_TX_BUFFER_SIZE 64

enum grid_gamepad_event_type {
  GRID_GAMEPAD_EVENT_AXIS = 0,
  GRID_GAMEPAD_EVENT_BUTTON = 1,
};

struct grid_gamepad_event_desc {
  uint8_t type;
  uint8_t index;
  uint8_t value; // axis: int8_t encoded as value+128; button: 0/1
  uint8_t _pad;
};

struct grid_gamepad_model {
  struct grid_swsr_t tx;
  uint32_t tx_dropped;
  uint32_t buttons;
  int8_t axis[GAMEPAD_AXIS_COUNT];
  uint8_t hat;
  bool (*tx_interface_ready)(void);
};

void grid_usb_gamepad_init(struct grid_gamepad_model* usb_gamepad, uint16_t buffer_size, bool (*tx_interface_ready)(void));
void grid_usb_gamepad_on_connect(struct grid_gamepad_model* usb_gamepad);
void grid_usb_gamepad_on_disconnect(struct grid_gamepad_model* usb_gamepad);

int32_t grid_usb_gamepad_axis_move(struct grid_gamepad_model* usb_gamepad, uint8_t axis, int32_t value);
int32_t grid_usb_gamepad_button_change(struct grid_gamepad_model* usb_gamepad, uint8_t button, uint8_t value);

uint8_t grid_usb_gamepad_tx_push(struct grid_gamepad_model* usb_gamepad, struct grid_gamepad_event_desc event);
void grid_usb_gamepad_tx_flush(struct grid_gamepad_model* usb_gamepad);
bool grid_usb_gamepad_tx_available(struct grid_gamepad_model* usb_gamepad);

#endif /* GRID_USB_GAMEPAD_H */
