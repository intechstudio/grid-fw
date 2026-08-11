#include <assert.h>
#include <string.h>

#include "tusb.h"

#include "grid_usb_keyboard.h"

static struct grid_keyboard_report grid_usb_keyboard_report_build(struct grid_keyboard_model* keyboard) {
  assert(keyboard->active_key_count <= GRID_KEYBOARD_KEY_maxcount);

  struct grid_keyboard_report report = {0};
  uint8_t key_idx = 0;

  for (uint8_t i = 0; i < keyboard->active_key_count; i++) {
    if (keyboard->active_key_list[i].type == GRID_MACRO_EVENT_TYPE_MODIFIER) {
      report.modifier_bm |= keyboard->active_key_list[i].keycode;
    } else if (keyboard->active_key_list[i].ispressed && key_idx < 6) {
      report.keycode[key_idx++] = keyboard->active_key_list[i].keycode;
    }
  }

  return report;
}

static int32_t grid_usb_keyboard_report_send(struct grid_keyboard_report* report) { return 0 == tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, report->modifier_bm, report->keycode); }

static uint8_t grid_usb_keyboard_cleanup(struct grid_keyboard_model* keyboard) {

  uint8_t changed_flag = 0;

  for (uint8_t i = 0; i < keyboard->active_key_count; i++) {

    if (keyboard->active_key_list[i].ispressed == false) {

      changed_flag = 1;
      uint8_t count = keyboard->active_key_count - i - 1;
      memmove(&keyboard->active_key_list[i], &keyboard->active_key_list[i + 1], count * sizeof(struct grid_macro_event_desc));
      keyboard->active_key_count--;
      i--;
    }
  }

  return changed_flag;
}

int32_t grid_usb_keyboard_keychange(struct grid_keyboard_model* keyboard, struct grid_macro_event_desc* key) {

  if (!tud_hid_ready()) {
    return 1;
  }

  grid_usb_keyboard_cleanup(keyboard);

  uint8_t idx = keyboard->active_key_count;

  for (uint8_t i = 0; i < keyboard->active_key_count; i++) {
    if (keyboard->active_key_list[i].keycode == key->keycode && keyboard->active_key_list[i].type == key->type) {
      idx = i;
      break;
    }
  }

  assert(idx <= GRID_KEYBOARD_KEY_maxcount);
  bool full = idx == GRID_KEYBOARD_KEY_maxcount;
  bool found = idx != keyboard->active_key_count;

  if (full && !found) {
    return 0;
  }

  keyboard->active_key_count = idx + !found;
  keyboard->active_key_list[idx] = *key;

  struct grid_keyboard_report report = grid_usb_keyboard_report_build(keyboard);

  return grid_usb_keyboard_report_send(&report);
}

void grid_usb_keyboard_init(struct grid_keyboard_model* keyboard) {

  for (uint8_t i = 0; i < GRID_KEYBOARD_KEY_maxcount; i++) {
    keyboard->active_key_list[i].type = GRID_MACRO_EVENT_TYPE_KEY;
    keyboard->active_key_list[i].ispressed = 0;
  }

  keyboard->active_key_count = 0;
}

void grid_usb_keyboard_on_connect(struct grid_keyboard_model* keyboard) { (void)keyboard; }

void grid_usb_keyboard_on_disconnect(struct grid_keyboard_model* keyboard) { keyboard->active_key_count = 0; }
