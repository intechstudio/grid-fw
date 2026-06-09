#ifndef GRID_USB_MOUSE_H
#define GRID_USB_MOUSE_H

#include <stdbool.h>
#include <stdint.h>

enum mouse_move_type {
  MOUSE_AXIS_X = 0x01,
  MOUSE_AXIS_Y = 0x02,
  MOUSE_AXIS_SCROLL = 0x03,
  MOUSE_AXIS_COUNT = 0x04,
};

struct grid_mouse_model {
  uint8_t buttons;
  bool (*tx_interface_ready)(void);
};

void grid_usb_mouse_init(struct grid_mouse_model* usb_mouse, bool (*tx_interface_ready)(void));
void grid_usb_mouse_on_connect(struct grid_mouse_model* usb_mouse);
void grid_usb_mouse_on_disconnect(struct grid_mouse_model* usb_mouse);

int32_t grid_usb_mouse_button_change(struct grid_mouse_model* usb_mouse, uint8_t b_state, uint8_t type);
int32_t grid_usb_mouse_move(struct grid_mouse_model* usb_mouse, int8_t position, uint8_t axis);

#endif /* GRID_USB_MOUSE_H */
