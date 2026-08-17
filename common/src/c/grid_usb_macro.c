#include <assert.h>

#include "grid_usb_macro.h"

#include "grid_health.h"
#include "grid_platform.h"
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

  switch (macro->next.type) {
  case GRID_MACRO_EVENT_TYPE_KEY:
  case GRID_MACRO_EVENT_TYPE_MODIFIER: {

    if (grid_usb_keyboard_keychange(macro->keyboard, &macro->next) != 0) {
      grid_health_record(&grid_health_state, GRID_HEALTH_TX_DROPPED_KEYBOARD);
      return;
    }
  } break;

  case GRID_MACRO_EVENT_TYPE_MOUSE_MOVE: {

    uint8_t axis = macro->next.keycode;
    int8_t position = macro->next.ispressed - 128;

    if (grid_usb_mouse_move(macro->mouse, position, axis) != 0) {
      grid_health_record(&grid_health_state, GRID_HEALTH_TX_DROPPED_MOUSE);
      return;
    }
  } break;
  case GRID_MACRO_EVENT_TYPE_MOUSE_BUTTON: {

    uint8_t state = macro->next.ispressed;
    uint8_t button = macro->next.keycode;

    if (grid_usb_mouse_button_change(macro->mouse, state, button) != 0) {
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
