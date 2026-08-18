#ifndef GRID_USB_MOUSE_H
#define GRID_USB_MOUSE_H

#include <stdbool.h>
#include <stdint.h>

enum mouse_move_type {
  MOUSE_AXIS_X = 0x01,
  MOUSE_AXIS_Y,
  MOUSE_AXIS_SCROLL,
  MOUSE_AXIS_COUNT,
};

struct grid_mouse_model {
  uint8_t buttons;
};

void grid_usb_mouse_init(struct grid_mouse_model* mouse);
void grid_usb_mouse_on_connect(struct grid_mouse_model* mouse);
void grid_usb_mouse_on_disconnect(struct grid_mouse_model* mouse);

int32_t grid_usb_mouse_button_change(struct grid_mouse_model* mouse, uint8_t b_state, uint8_t type);
int32_t grid_usb_mouse_move(struct grid_mouse_model* mouse, int8_t position, enum mouse_move_type axis);

#endif /* GRID_USB_MOUSE_H */
