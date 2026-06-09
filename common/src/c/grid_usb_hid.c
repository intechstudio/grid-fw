#include <assert.h>

#include "tusb.h"

#include "grid_usb_hid.h"

#include "grid_msg.h"
#include "grid_platform.h"
#include "grid_port.h"
#include "grid_sys.h"
#include "grid_transport.h"
#include "grid_usb.h"
#include "grid_usb_mouse.h"

struct grid_macro_model grid_macro_state = {0};
struct grid_keyboard_model grid_keyboard_state = {0};

//--------------------------------------------------------------------+
// Static helpers — defined before callers to avoid forward declarations
//--------------------------------------------------------------------+

#if CFG_TUD_HID

static bool grid_usb_hid_ready(void) { return tud_hid_ready(); }

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

static int32_t grid_usb_keyboard_report_send(struct grid_keyboard_report* report) { return 0 == tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, report->modifier_bm, report->keycode); }

#else // !CFG_TUD_HID

static bool grid_usb_hid_ready(void) { return true; }

static struct grid_keyboard_report grid_usb_keyboard_report_build(struct grid_keyboard_model* usb_keyboard) {
  (void)usb_keyboard;
  struct grid_keyboard_report report = {0};
  return report;
}

static int32_t grid_usb_keyboard_report_send(struct grid_keyboard_report* report) {
  (void)report;
  return 0;
}

#endif // CFG_TUD_HID

//--------------------------------------------------------------------+
// Keyboard internals
//--------------------------------------------------------------------+

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

//--------------------------------------------------------------------+
// Init
//--------------------------------------------------------------------+

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

//--------------------------------------------------------------------+
// Connect / disconnect
//--------------------------------------------------------------------+

void grid_usb_hid_on_connect(struct grid_macro_model* usb_macro, struct grid_keyboard_model* usb_keyboard) {
  (void)usb_macro;
  (void)usb_keyboard;
}

void grid_usb_hid_on_disconnect(struct grid_macro_model* usb_macro, struct grid_keyboard_model* usb_keyboard) {
  grid_swsr_read(&usb_macro->tx, NULL, grid_swsr_size(&usb_macro->tx));
  usb_macro->has_next = false;
  usb_keyboard->active_key_count = 0;
}

//--------------------------------------------------------------------+
// TX push / flush / available
//--------------------------------------------------------------------+

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

//--------------------------------------------------------------------+
// Enable / disable
//--------------------------------------------------------------------+

void grid_usb_keyboard_enable(struct grid_keyboard_model* usb_keyboard) { usb_keyboard->isenabled = 1; }

void grid_usb_keyboard_disable(struct grid_keyboard_model* usb_keyboard) { usb_keyboard->isenabled = 0; }

//--------------------------------------------------------------------+
// TinyUSB callbacks
//--------------------------------------------------------------------+

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

#endif // CFG_TUD_HID
