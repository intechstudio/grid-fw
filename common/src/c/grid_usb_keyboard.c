#include <assert.h>
#include <string.h>

#include "tusb.h"

#include "grid_usb_keyboard.h"

#include "grid_usb.h"

#include "grid_msg.h"
#include "grid_platform.h"
#include "grid_port.h"
#include "grid_sys.h"
#include "grid_transport.h"

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

static uint8_t grid_usb_keyboard_cleanup(struct grid_keyboard_model* usb_keyboard) {

  uint8_t changed_flag = 0;

  for (uint8_t i = 0; i < usb_keyboard->active_key_count; i++) {

    if (usb_keyboard->active_key_list[i].ispressed == false) {

      changed_flag = 1;
      uint8_t count = usb_keyboard->active_key_count - i - 1;
      memmove(&usb_keyboard->active_key_list[i], &usb_keyboard->active_key_list[i + 1], count * sizeof(struct grid_macro_event_desc));
      usb_keyboard->active_key_count--;
      i--;
    }
  }

  return changed_flag;
}

int32_t grid_usb_keyboard_keychange(struct grid_keyboard_model* usb_keyboard, struct grid_macro_event_desc* key) {

  if (!usb_keyboard->tx_interface_ready()) {
    return 1;
  }

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

void grid_usb_keyboard_init(struct grid_keyboard_model* usb_keyboard, bool (*tx_interface_ready)(void)) {

  for (uint8_t i = 0; i < GRID_KEYBOARD_KEY_maxcount; i++) {
    usb_keyboard->active_key_list[i].type = GRID_MACRO_EVENT_TYPE_KEY;
    usb_keyboard->active_key_list[i].ispressed = 0;
  }

  usb_keyboard->active_key_count = 0;
  usb_keyboard->isenabled = 1;
  usb_keyboard->tx_interface_ready = tx_interface_ready;
}

void grid_usb_keyboard_on_connect(struct grid_keyboard_model* usb_keyboard) { (void)usb_keyboard; }

void grid_usb_keyboard_on_disconnect(struct grid_keyboard_model* usb_keyboard) { usb_keyboard->active_key_count = 0; }

void grid_usb_keyboard_enable(struct grid_keyboard_model* usb_keyboard) { usb_keyboard->isenabled = 1; }

void grid_usb_keyboard_disable(struct grid_keyboard_model* usb_keyboard) { usb_keyboard->isenabled = 0; }
