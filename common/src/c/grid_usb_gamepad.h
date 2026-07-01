#ifndef GRID_USB_GAMEPAD_H
#define GRID_USB_GAMEPAD_H

#include <stdbool.h>
#include <stdint.h>

#include "grid_swsr.h"

enum gamepad_axis_t {
  GAMEPAD_AXIS_X,
  GAMEPAD_AXIS_Y,
  GAMEPAD_AXIS_Z,
  GAMEPAD_AXIS_RX,
  GAMEPAD_AXIS_RY,
  GAMEPAD_AXIS_RZ,
  GAMEPAD_AXIS_COUNT,
};

#define GRID_GAMEPAD_TX_BUFFER_SIZE 64

enum grid_gamepad_event_type {
  GRID_GAMEPAD_EVENT_AXIS,
  GRID_GAMEPAD_EVENT_BUTTON,
};

struct grid_gamepad_event_desc {
  enum grid_gamepad_event_type type;
  uint8_t index;
  uint8_t value; // axis: int8_t encoded as value+128; button: 0/1
  uint8_t _pad;
};

struct grid_gamepad_model {
  struct grid_swsr_t tx;
  uint32_t buttons;
  int8_t axis[GAMEPAD_AXIS_COUNT];
  uint8_t hat;
};

void grid_usb_gamepad_init(struct grid_gamepad_model* gamepad, uint16_t buffer_size);
void grid_usb_gamepad_on_connect(struct grid_gamepad_model* gamepad);
void grid_usb_gamepad_on_disconnect(struct grid_gamepad_model* gamepad);

int32_t grid_usb_gamepad_axis_move(struct grid_gamepad_model* gamepad, enum gamepad_axis_t axis, int32_t value);
int32_t grid_usb_gamepad_button_change(struct grid_gamepad_model* gamepad, uint8_t button, uint8_t value);

uint8_t grid_usb_gamepad_tx_push(struct grid_gamepad_model* gamepad, struct grid_gamepad_event_desc event);
void grid_usb_gamepad_tx_flush(struct grid_gamepad_model* gamepad);
bool grid_usb_gamepad_tx_available(struct grid_gamepad_model* gamepad);

#endif /* GRID_USB_GAMEPAD_H */
