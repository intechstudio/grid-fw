#include <assert.h>

#include "tusb.h"

#include "grid_usb_macro.h"

#include "grid_platform.h"
#include "grid_sys.h"
#include "grid_usb.h"
#include "grid_usb_keyboard.h"
#include "grid_usb_mouse.h"

#if CFG_TUD_HID
static bool grid_usb_hid_ready(void) { return tud_hid_ready(); }
#else
static bool grid_usb_hid_ready(void) { return true; }
#endif

void grid_usb_macro_init(struct grid_macro_model* usb_macro, uint16_t buffer_size, struct grid_keyboard_model* usb_keyboard, struct grid_mouse_model* usb_mouse) {
  usb_macro->tx_rtc_lasttimestamp = grid_platform_rtc_get_micros();
  usb_macro->has_next = false;
  usb_macro->keyboard = usb_keyboard;
  usb_macro->mouse = usb_mouse;
  assert(grid_swsr_malloc(&usb_macro->tx, buffer_size) == 0);
}

void grid_usb_macro_on_connect(struct grid_macro_model* usb_macro) { (void)usb_macro; }

void grid_usb_macro_on_disconnect(struct grid_macro_model* usb_macro) {
  grid_swsr_read(&usb_macro->tx, NULL, grid_swsr_size(&usb_macro->tx));
  usb_macro->has_next = false;
}

uint8_t grid_usb_macro_tx_push(struct grid_macro_model* usb_macro, struct grid_macro_event_desc event) {
  if (!grid_usb_connected()) {
    usb_macro->tx_dropped++;
    return 1;
  }
  uint8_t dropped = 0;
  if (!grid_swsr_writable(&usb_macro->tx, sizeof(struct grid_macro_event_desc))) {
    grid_swsr_read(&usb_macro->tx, NULL, sizeof(struct grid_macro_event_desc));
    usb_macro->tx_dropped++;
    dropped = 1;
  }
  grid_swsr_write(&usb_macro->tx, &event, sizeof(struct grid_macro_event_desc));
  return dropped;
}

bool grid_usb_macro_tx_available(struct grid_macro_model* usb_macro) { return usb_macro->has_next || grid_swsr_readable(&usb_macro->tx, sizeof(struct grid_macro_event_desc)); }

void grid_usb_macro_tx_flush(struct grid_macro_model* usb_macro) {

  if (!usb_macro->has_next) {
    if (!grid_swsr_readable(&usb_macro->tx, sizeof(struct grid_macro_event_desc))) {
      return;
    }
    grid_swsr_read(&usb_macro->tx, &usb_macro->next, sizeof(struct grid_macro_event_desc));
    usb_macro->has_next = true;
  }

  uint64_t elapsed = grid_platform_rtc_get_elapsed_time(usb_macro->tx_rtc_lasttimestamp);
  if (elapsed <= usb_macro->next.delay * MS_TO_US) {
    return;
  }

  if (!grid_usb_hid_ready()) {
    return;
  }

  int32_t result = 0;

  switch (usb_macro->next.type) {
  case GRID_MACRO_EVENT_TYPE_KEY:
  case GRID_MACRO_EVENT_TYPE_MODIFIER:
    result = grid_usb_keyboard_keychange(usb_macro->keyboard, &usb_macro->next);
    break;
  case GRID_MACRO_EVENT_TYPE_MOUSE_MOVE: {
    uint8_t axis = usb_macro->next.keycode;
    int8_t position = usb_macro->next.ispressed - 128;
    result = grid_usb_mouse_move(usb_macro->mouse, position, axis);
    break;
  }
  case GRID_MACRO_EVENT_TYPE_MOUSE_BUTTON: {
    uint8_t state = usb_macro->next.ispressed;
    uint8_t button = usb_macro->next.keycode;
    result = grid_usb_mouse_button_change(usb_macro->mouse, state, button);
    break;
  }
  case GRID_MACRO_EVENT_TYPE_DELAY:
  default:
    break;
  }

  if (result != 0) {
    return;
  }

  usb_macro->has_next = false;
  usb_macro->tx_rtc_lasttimestamp = grid_platform_rtc_get_micros();
}
