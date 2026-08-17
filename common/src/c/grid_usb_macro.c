#include <assert.h>

#include "tusb.h"

#include "grid_usb_macro.h"

#include "grid_health.h"
#include "grid_platform.h"
#include "grid_port.h"
#include "grid_sys.h"
#include "grid_usb.h"
#include "grid_usb_keyboard.h"
#include "grid_usb_mouse.h"

void grid_usb_macro_init(struct grid_macro_model* macro, uint16_t buffer_size, struct grid_keyboard_model* keyboard, struct grid_mouse_model* mouse) {
  macro->tx_rtc_lasttimestamp = grid_platform_rtc_get_micros();
  macro->has_next = false;
  macro->keyboard = keyboard;
  macro->mouse = mouse;
  assert(grid_swsr_malloc(&macro->tx, buffer_size) == 0);
}

void grid_usb_macro_on_connect(struct grid_macro_model* macro) { (void)macro; }

void grid_usb_macro_on_disconnect(struct grid_macro_model* macro) {
  grid_swsr_read(&macro->tx, NULL, grid_swsr_size(&macro->tx));
  macro->has_next = false;
}

uint8_t grid_usb_macro_tx_push(struct grid_macro_model* macro, struct grid_macro_event_desc event) {

  if (!grid_usb_connected()) {
    return 1;
  }

  uint8_t dropped = 0;

  if (!grid_swsr_writable(&macro->tx, sizeof(struct grid_macro_event_desc))) {
    grid_swsr_read(&macro->tx, NULL, sizeof(struct grid_macro_event_desc));
    dropped = 1;
  }

  grid_swsr_write(&macro->tx, &event, sizeof(struct grid_macro_event_desc));

  return dropped;
}

bool grid_usb_macro_tx_available(struct grid_macro_model* macro) { return macro->has_next || grid_swsr_readable(&macro->tx, sizeof(struct grid_macro_event_desc)); }

static void grid_usb_macro_mouse_apply_event(int8_t delta[3], uint8_t* buttons, struct grid_macro_event_desc* event) {

  if (event->type == GRID_MACRO_EVENT_TYPE_MOUSE_MOVE) {
    uint8_t axis = event->keycode;
    int8_t position = event->ispressed - 128;
    if (axis >= MOUSE_AXIS_X && axis < MOUSE_AXIS_COUNT) {
      delta[axis - 1] += position;
    }
  } else if (event->type == GRID_MACRO_EVENT_TYPE_MOUSE_BUTTON) {
    if (event->ispressed) {
      *buttons |= event->keycode;
    } else {
      *buttons &= (uint8_t)~event->keycode;
    }
  }
}

void grid_usb_macro_tx_flush(struct grid_macro_model* macro) {

  if (!macro->has_next) {
    if (!grid_swsr_readable(&macro->tx, sizeof(struct grid_macro_event_desc))) {
      return;
    }
    grid_swsr_read(&macro->tx, &macro->next, sizeof(struct grid_macro_event_desc));
    macro->has_next = true;
  }

  uint64_t elapsed = grid_platform_rtc_get_elapsed_time(macro->tx_rtc_lasttimestamp);
  if (elapsed <= macro->next.delay * MS_TO_US) {
    return;
  }

  if (!tud_hid_ready()) {
    return;
  }

  switch (macro->next.type) {
  case GRID_MACRO_EVENT_TYPE_KEY:
  case GRID_MACRO_EVENT_TYPE_MODIFIER: {

    if (grid_usb_keyboard_keychange(macro->keyboard, &macro->next) != 0) {
      return;
    }
  } break;

  case GRID_MACRO_EVENT_TYPE_MOUSE_MOVE:
  case GRID_MACRO_EVENT_TYPE_MOUSE_BUTTON: {

    int8_t delta[3] = {0};
    uint8_t buttons = macro->mouse->buttons;

    grid_usb_macro_mouse_apply_event(delta, &buttons, &macro->next);

    macro->has_next = false;

    while (grid_swsr_readable(&macro->tx, sizeof(struct grid_macro_event_desc))) {
      struct grid_macro_event_desc peek;
      grid_swsr_read(&macro->tx, &peek, sizeof(struct grid_macro_event_desc));

      if ((peek.type == GRID_MACRO_EVENT_TYPE_MOUSE_MOVE || peek.type == GRID_MACRO_EVENT_TYPE_MOUSE_BUTTON) && peek.delay == 0) {
        grid_usb_macro_mouse_apply_event(delta, &buttons, &peek);
      } else {
        macro->next = peek;
        macro->has_next = true;
        break;
      }
    }

    macro->mouse->buttons = buttons;

    if (tud_hid_mouse_report(HID_ITF_PROTOCOL_MOUSE, buttons, delta[0], delta[1], delta[2], 0) == 0) {
      grid_port_debug_printf("mouse report failed: ready=%d btn=%02x dx=%d dy=%d dz=%d\n", tud_hid_ready(), buttons, delta[0], delta[1], delta[2]);
      grid_health_record(&grid_health_state, GRID_HEALTH_TX_DROPPED_MOUSE);
      return;
    }
  } break;
  default:
    break;
  }

  macro->has_next = false;
  macro->tx_rtc_lasttimestamp = grid_platform_rtc_get_micros();
  return;
}
