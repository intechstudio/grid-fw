#include <assert.h>

#include "tusb.h"

#include "grid_usb_hid.h"

#include "grid_msg.h"
#include "grid_platform.h"
#include "grid_port.h"
#include "grid_sys.h"
#include "grid_transport.h"
#include "grid_usb.h"

struct grid_macro_model grid_macro_state = {0};
struct grid_keyboard_model grid_keyboard_state = {0};
struct grid_mouse_model grid_mouse_state = {0};
struct grid_gamepad_model grid_gamepad_state = {0};

static struct grid_keyboard_report grid_usb_keyboard_report_build(struct grid_keyboard_model* usb_keyboard);
static int32_t grid_usb_keyboard_report_send(struct grid_keyboard_report* report);
static bool grid_usb_hid_ready(void);
static int32_t grid_usb_mouse_button_change(struct grid_mouse_model* usb_mouse, uint8_t b_state, uint8_t type);
static int32_t grid_usb_mouse_move(struct grid_mouse_model* usb_mouse, int8_t position, uint8_t axis);
static int32_t grid_usb_gamepad_report_send(struct grid_gamepad_model* usb_gamepad);

void grid_usb_keyboard_init(struct grid_keyboard_model* usb_keyboard) {

  for (uint8_t i = 0; i < GRID_KEYBOARD_KEY_maxcount; i++) {
    usb_keyboard->active_key_list[i].type = GRID_MACRO_EVENT_TYPE_KEY;
    usb_keyboard->active_key_list[i].ispressed = 0;
  }

  usb_keyboard->active_key_count = 0;
  usb_keyboard->isenabled = 1;
}

void grid_usb_macro_init(struct grid_macro_model* usb_macro, uint16_t buffer_size, struct grid_keyboard_model* usb_keyboard, struct grid_mouse_model* usb_mouse) {

  usb_macro->tx_rtc_lasttimestamp = grid_platform_rtc_get_micros();
  usb_macro->has_next = false;
  usb_macro->keyboard = usb_keyboard;
  usb_macro->mouse = usb_mouse;

  assert(grid_swsr_malloc(&usb_macro->tx, buffer_size) == 0);
}

void grid_usb_mouse_init(struct grid_mouse_model* usb_mouse) { usb_mouse->buttons = 0; }

void grid_usb_gamepad_init(struct grid_gamepad_model* usb_gamepad, uint16_t buffer_size) {
  usb_gamepad->buttons = 0;
  usb_gamepad->hat = 0;
  for (uint8_t i = 0; i < GAMEPAD_AXIS_COUNT; i++) {
    usb_gamepad->axis[i] = 0;
  }
  assert(grid_swsr_malloc(&usb_gamepad->tx, buffer_size) == 0);
}

int32_t grid_usb_gamepad_axis_move(struct grid_gamepad_model* usb_gamepad, uint8_t axis, int32_t value) {
  (void)usb_gamepad;
  if (axis >= GAMEPAD_AXIS_COUNT) {
    return 0;
  }
  struct grid_gamepad_event_desc event = {.type = GRID_GAMEPAD_EVENT_AXIS, .index = axis, .value = (uint8_t)(value + 128)};
  return grid_usb_gamepad_tx_push(&grid_gamepad_state, event);
}

int32_t grid_usb_gamepad_button_change(struct grid_gamepad_model* usb_gamepad, uint8_t button, uint8_t value) {
  (void)usb_gamepad;
  struct grid_gamepad_event_desc event = {.type = GRID_GAMEPAD_EVENT_BUTTON, .index = button, .value = value};
  return grid_usb_gamepad_tx_push(&grid_gamepad_state, event);
}

static uint8_t grid_usb_keyboard_cleanup(struct grid_keyboard_model* usb_keyboard) {

  uint8_t changed_flag = 0;

  for (uint8_t i = 0; i < usb_keyboard->active_key_count; i++) {

    if (usb_keyboard->active_key_list[i].ispressed == false) {

      changed_flag = 1;

      for (uint8_t j = i + 1; j < usb_keyboard->active_key_count; j++) {
        usb_keyboard->active_key_list[j - 1] = usb_keyboard->active_key_list[j];
      }

      usb_keyboard->active_key_count--;
      i--;
    }
  }

  return changed_flag;
}

static int32_t grid_usb_keyboard_keychange(struct grid_keyboard_model* usb_keyboard, struct grid_macro_event_desc* key) {

  bool found = false;
  uint8_t changed_flag = 0;

  for (uint8_t i = 0; i < usb_keyboard->active_key_count; i++) {

    if (usb_keyboard->active_key_list[i].keycode == key->keycode && usb_keyboard->active_key_list[i].type == key->type) {
      found = true;

      if (usb_keyboard->active_key_list[i].ispressed == true) {

        if (key->ispressed == true) {
        } else {
          usb_keyboard->active_key_list[i].ispressed = false;
          changed_flag = 1;
        }
      }
    }
  }

  grid_usb_keyboard_cleanup(usb_keyboard);

  if (!found) {

    if (usb_keyboard->active_key_count < GRID_KEYBOARD_KEY_maxcount) {

      if (key->ispressed == true) {

        usb_keyboard->active_key_list[usb_keyboard->active_key_count] = *key;
        usb_keyboard->active_key_count++;
        changed_flag = 1;
      }
    } else {
    }
  }

  if (changed_flag == 1) {

    if (usb_keyboard->isenabled) {

      struct grid_keyboard_report report = grid_usb_keyboard_report_build(usb_keyboard);
      return grid_usb_keyboard_report_send(&report);
    } else {

      grid_port_debug_print_text("KB IS DISABLED");

      struct grid_msg msg;
      uint8_t xy = GRID_PARAMETER_GLOBAL_POSITION;
      grid_msg_init_brc(&grid_msg_state, &msg, xy, xy);

      grid_msg_add_frame(&msg, GRID_CLASS_HIDKEYSTATUS_frame);
      grid_msg_set_parameter(&msg, INSTR, GRID_INSTR_REPORT_code);
      grid_msg_set_parameter(&msg, CLASS_HIDKEYSTATUS_ISENABLED, usb_keyboard->isenabled);

      if (grid_msg_close_brc(&grid_msg_state, &msg) >= 0) {
        grid_transport_send_msg_to_all(&grid_transport_state, &msg);
      }

      return 0;
    }
  }

  return 0;
}

static struct grid_keyboard_report grid_usb_keyboard_report_build(struct grid_keyboard_model* usb_keyboard) {
  assert(usb_keyboard->active_key_count <= GRID_KEYBOARD_KEY_maxcount);

  struct grid_keyboard_report report = {0};
  uint8_t key_idx = 0;

  for (uint8_t i = 0; i < usb_keyboard->active_key_count; i++) {
    if (usb_keyboard->active_key_list[i].type == GRID_MACRO_EVENT_TYPE_MODIFIER) {
      report.modifier_bm |= usb_keyboard->active_key_list[i].keycode;
    } else if (usb_keyboard->active_key_list[i].ispressed && key_idx < 6) {
      report.keycode[key_idx++] = usb_keyboard->active_key_list[i].keycode;
    }
  }

  return report;
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

  if (!grid_usb_hid_ready()) {
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

  assert(grid_usb_gamepad_report_send(usb_gamepad) == 0);
}

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

  if (usb_macro->next.type == GRID_MACRO_EVENT_TYPE_KEY || usb_macro->next.type == GRID_MACRO_EVENT_TYPE_MODIFIER) {
    if (!grid_usb_hid_ready()) {
      return;
    }
    if (grid_usb_keyboard_keychange(usb_macro->keyboard, &usb_macro->next)) {
      return;
    }
  } else if (usb_macro->next.type == GRID_MACRO_EVENT_TYPE_MOUSE_MOVE) {
    if (!grid_usb_hid_ready()) {
      return;
    }
    uint8_t axis = usb_macro->next.keycode;
    int8_t position = usb_macro->next.ispressed - 128;
    if (grid_usb_mouse_move(usb_macro->mouse, position, axis)) {
      return;
    }
  } else if (usb_macro->next.type == GRID_MACRO_EVENT_TYPE_MOUSE_BUTTON) {
    if (!grid_usb_hid_ready()) {
      return;
    }
    uint8_t state = usb_macro->next.ispressed;
    uint8_t button = usb_macro->next.keycode;
    if (grid_usb_mouse_button_change(usb_macro->mouse, state, button)) {
      return;
    }
  } else if (usb_macro->next.type == GRID_MACRO_EVENT_TYPE_DELAY) {
  } else {
  }

  usb_macro->has_next = false;
  usb_macro->tx_rtc_lasttimestamp = grid_platform_rtc_get_micros();
}

bool grid_usb_macro_tx_available(struct grid_macro_model* usb_macro) { return usb_macro->has_next || grid_swsr_readable(&usb_macro->tx, sizeof(struct grid_macro_event_desc)); }

void grid_usb_keyboard_enable(struct grid_keyboard_model* usb_keyboard) { usb_keyboard->isenabled = 1; }

void grid_usb_keyboard_disable(struct grid_keyboard_model* usb_keyboard) { usb_keyboard->isenabled = 0; }

#if CFG_TUD_HID

static const uint8_t s_hid_report_desc[] = {GRID_HID_REPORT_DESC_CONTENT};

uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance) {
  (void)instance;
  return s_hid_report_desc;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;
  return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)bufsize;
}

static bool grid_usb_hid_ready(void) { return tud_hid_ready(); }

static int32_t grid_usb_mouse_button_change(struct grid_mouse_model* usb_mouse, uint8_t b_state, uint8_t type) {
  if (b_state) {
    usb_mouse->buttons |= type;
  } else {
    usb_mouse->buttons &= (uint8_t)~type;
  }
  return 0 == tud_hid_mouse_report(HID_ITF_PROTOCOL_MOUSE, usb_mouse->buttons, 0, 0, 0, 0);
}

static int32_t grid_usb_mouse_move(struct grid_mouse_model* usb_mouse, int8_t position, uint8_t axis) {
  int8_t delta[3] = {0};
  if (axis < MOUSE_AXIS_X || axis >= MOUSE_AXIS_COUNT) {
    return 0;
  }
  delta[axis - 1] = position;
  return 0 == tud_hid_mouse_report(HID_ITF_PROTOCOL_MOUSE, usb_mouse->buttons, delta[0], delta[1], delta[2], 0);
}

// Axes RX/RY/RZ (3,4,5) are passed in reverse order to match the HID descriptor layout.
static int32_t grid_usb_gamepad_report_send(struct grid_gamepad_model* usb_gamepad) {
  return 0 == tud_hid_gamepad_report(3, usb_gamepad->axis[0], usb_gamepad->axis[1], usb_gamepad->axis[2], usb_gamepad->axis[5], usb_gamepad->axis[4], usb_gamepad->axis[3], usb_gamepad->hat,
                                     usb_gamepad->buttons);
}

static int32_t grid_usb_keyboard_report_send(struct grid_keyboard_report* report) { return 0 == tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, report->modifier_bm, report->keycode); }

#else // !CFG_TUD_HID

static bool grid_usb_hid_ready(void) { return true; }

static int32_t grid_usb_mouse_button_change(struct grid_mouse_model* usb_mouse, uint8_t b_state, uint8_t type) {
  (void)usb_mouse;
  (void)b_state;
  (void)type;
  return 0;
}

static int32_t grid_usb_mouse_move(struct grid_mouse_model* usb_mouse, int8_t position, uint8_t axis) {
  (void)usb_mouse;
  (void)position;
  (void)axis;
  return 0;
}

static int32_t grid_usb_gamepad_report_send(struct grid_gamepad_model* usb_gamepad) {
  (void)usb_gamepad;
  return 0;
}

static int32_t grid_usb_keyboard_report_send(struct grid_keyboard_report* report) {
  (void)report;
  return 0;
}

#endif // CFG_TUD_HID
