#include <assert.h>

#include "tusb.h"

#include "grid_usb_gamepad.h"

#include "grid_swsr.h"
#include "grid_usb.h"

// Axes RX/RY/RZ (3,4,5) are passed in reverse order to match the HID descriptor layout.
static int32_t grid_usb_gamepad_report_send(struct grid_gamepad_model* usb_gamepad) {
  return 0 == tud_hid_gamepad_report(3, usb_gamepad->axis[0], usb_gamepad->axis[1], usb_gamepad->axis[2], usb_gamepad->axis[5], usb_gamepad->axis[4], usb_gamepad->axis[3], usb_gamepad->hat,
                                     usb_gamepad->buttons);
}

void grid_usb_gamepad_init(struct grid_gamepad_model* usb_gamepad, uint16_t buffer_size, bool (*tx_interface_ready)(void)) {
  usb_gamepad->buttons = 0;
  usb_gamepad->hat = 0;
  for (uint8_t i = 0; i < GAMEPAD_AXIS_COUNT; i++) {
    usb_gamepad->axis[i] = 0;
  }
  usb_gamepad->tx_interface_ready = tx_interface_ready;
  assert(grid_swsr_malloc(&usb_gamepad->tx, buffer_size) == 0);
}

void grid_usb_gamepad_on_connect(struct grid_gamepad_model* usb_gamepad) { (void)usb_gamepad; }

void grid_usb_gamepad_on_disconnect(struct grid_gamepad_model* usb_gamepad) { grid_swsr_read(&usb_gamepad->tx, NULL, grid_swsr_size(&usb_gamepad->tx)); }

int32_t grid_usb_gamepad_axis_move(struct grid_gamepad_model* usb_gamepad, uint8_t axis, int32_t value) {
  if (axis >= GAMEPAD_AXIS_COUNT) {
    return 0;
  }
  struct grid_gamepad_event_desc event = {.type = GRID_GAMEPAD_EVENT_AXIS, .index = axis, .value = (uint8_t)(value + 128)};
  return grid_usb_gamepad_tx_push(usb_gamepad, event);
}

int32_t grid_usb_gamepad_button_change(struct grid_gamepad_model* usb_gamepad, uint8_t button, uint8_t value) {
  struct grid_gamepad_event_desc event = {.type = GRID_GAMEPAD_EVENT_BUTTON, .index = button, .value = value};
  return grid_usb_gamepad_tx_push(usb_gamepad, event);
}

uint8_t grid_usb_gamepad_tx_push(struct grid_gamepad_model* usb_gamepad, struct grid_gamepad_event_desc event) {
  if (!grid_usb_connected()) {
    usb_gamepad->tx_dropped++;
    return 1;
  }
  uint8_t dropped = 0;
  if (!grid_swsr_writable(&usb_gamepad->tx, sizeof(struct grid_gamepad_event_desc))) {
    grid_swsr_read(&usb_gamepad->tx, NULL, sizeof(struct grid_gamepad_event_desc));
    usb_gamepad->tx_dropped++;
    dropped = 1;
  }
  grid_swsr_write(&usb_gamepad->tx, &event, sizeof(struct grid_gamepad_event_desc));
  return dropped;
}

bool grid_usb_gamepad_tx_available(struct grid_gamepad_model* usb_gamepad) { return grid_swsr_readable(&usb_gamepad->tx, sizeof(struct grid_gamepad_event_desc)); }

void grid_usb_gamepad_tx_flush(struct grid_gamepad_model* usb_gamepad) {
  if (!grid_swsr_readable(&usb_gamepad->tx, sizeof(struct grid_gamepad_event_desc))) {
    return;
  }
  if (!usb_gamepad->tx_interface_ready()) {
    return;
  }
  struct grid_gamepad_event_desc event;
  grid_swsr_read(&usb_gamepad->tx, &event, sizeof(struct grid_gamepad_event_desc));
  if (event.type == GRID_GAMEPAD_EVENT_AXIS) {
    if (event.index < GAMEPAD_AXIS_COUNT) {
      usb_gamepad->axis[event.index] = (int8_t)(event.value - 128);
    }
  } else if (event.type == GRID_GAMEPAD_EVENT_BUTTON) {
    if (event.value) {
      usb_gamepad->buttons |= (1u << event.index);
    } else {
      usb_gamepad->buttons &= ~(1u << event.index);
    }
  }
  grid_usb_gamepad_report_send(usb_gamepad);
}
