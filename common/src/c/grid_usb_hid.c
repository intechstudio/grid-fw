#include <assert.h>

#include "tusb.h"

#include "grid_usb_hid.h"

#include "grid_msg.h"
#include "grid_platform.h"
#include "grid_usb.h"

// Forward declarations for static functions
static uint8_t grid_keyboard_isenabled(struct grid_keyboard_model* kb);
static int32_t grid_keyboard_keys_state_change(struct grid_macro_event_desc* active_key_list, uint8_t keys_count);
static bool grid_usb_hid_ready(void);
static int32_t grid_usb_mouse_button_change(struct grid_mouse_model* mouse, uint8_t b_state, uint8_t type);
static int32_t grid_usb_mouse_move(struct grid_mouse_model* mouse, int8_t position, uint8_t axis);

struct grid_macro_model grid_macro_state;
struct grid_keyboard_model grid_keyboard_state;
struct grid_mouse_model grid_mouse_state;
struct grid_gamepad_model grid_gamepad_state;

void grid_usb_keyboard_init(struct grid_keyboard_model* kb) {

  for (uint8_t i = 0; i < GRID_KEYBOARD_KEY_maxcount; i++) {
    kb->active_key_list[i].ismodifier = 0;
    kb->active_key_list[i].keycode = 255;
    kb->active_key_list[i].ispressed = 0;
  }

  kb->active_key_count = 0;
  kb->isenabled = 1;
}

void grid_usb_macro_init(struct grid_macro_model* macro, uint16_t buffer_size, struct grid_keyboard_model* keyboard, struct grid_mouse_model* mouse) {

  macro->tx_rtc_lasttimestamp = grid_platform_rtc_get_micros();
  macro->has_next = false;
  macro->keyboard = keyboard;
  macro->mouse = mouse;

  assert(grid_swsr_malloc(&macro->tx, buffer_size) == 0);
}

void grid_usb_mouse_init(struct grid_mouse_model* mouse) { mouse->buttons = 0; }

void grid_usb_gamepad_init(struct grid_gamepad_model* gamepad) {
  gamepad->buttons = 0;
  gamepad->hat = 0;
  for (uint8_t i = 0; i < GAMEPAD_AXIS_COUNT; i++) {
    gamepad->axis[i] = 0;
  }
}

static uint8_t grid_keyboard_cleanup(struct grid_keyboard_model* kb) {

  uint8_t changed_flag = 0;

  for (uint8_t i = 0; i < kb->active_key_count; i++) {

    if (kb->active_key_list[i].ispressed == false) {

      changed_flag = 1;

      kb->active_key_list[i].ismodifier = 0;
      kb->active_key_list[i].ispressed = 0;
      kb->active_key_list[i].keycode = 255;

      for (uint8_t j = i + 1; j < kb->active_key_count; j++) {

        kb->active_key_list[j - 1] = kb->active_key_list[j];

        kb->active_key_list[j].ismodifier = 0;
        kb->active_key_list[j].ispressed = 0;
        kb->active_key_list[j].keycode = 255;
      }

      kb->active_key_count--;
      i--;
    }
  }

  return changed_flag;
}

static int32_t grid_keyboard_keychange(struct grid_keyboard_model* kb, struct grid_macro_event_desc* key) {

  uint8_t item_index = 255;
  uint8_t changed_flag = 0;

  grid_keyboard_cleanup(kb);

  for (uint8_t i = 0; i < kb->active_key_count; i++) {

    if (kb->active_key_list[i].keycode == key->keycode && kb->active_key_list[i].ismodifier == key->ismodifier) {
      item_index = i;

      if (kb->active_key_list[i].ispressed == true) {

        if (key->ispressed == true) {
        } else {
          kb->active_key_list[i].ispressed = false;
          changed_flag = 1;
        }
      }
    }
  }

  grid_keyboard_cleanup(kb);

  if (item_index == 255) {

    if (kb->active_key_count < GRID_KEYBOARD_KEY_maxcount) {

      if (key->ispressed == true) {

        kb->active_key_list[kb->active_key_count] = *key;
        kb->active_key_count++;
        changed_flag = 1;
      }
    } else {
    }
  }

  if (changed_flag == 1) {

    if (grid_keyboard_isenabled(kb)) {

      int32_t result = grid_keyboard_keys_state_change(kb->active_key_list, kb->active_key_count);
      return result;
    } else {

      grid_port_debug_print_text("KB IS DISABLED");

      struct grid_msg msg;
      uint8_t xy = GRID_PARAMETER_GLOBAL_POSITION;
      grid_msg_init_brc(&grid_msg_state, &msg, xy, xy);

      grid_msg_add_frame(&msg, GRID_CLASS_HIDKEYSTATUS_frame);
      grid_msg_set_parameter(&msg, INSTR, GRID_INSTR_REPORT_code);
      grid_msg_set_parameter(&msg, CLASS_HIDKEYSTATUS_ISENABLED, kb->isenabled);

      if (grid_msg_close_brc(&grid_msg_state, &msg) >= 0) {
        grid_transport_send_msg_to_all(&grid_transport_state, &msg);
      }

      return 0;
    }
  }

  return 0;
}

uint8_t grid_usb_macro_tx_push(struct grid_macro_model* macro, struct grid_macro_event_desc event) {

  uint8_t dropped = 0;

  if (!grid_swsr_writable(&macro->tx, sizeof(struct grid_macro_event_desc))) {
    grid_swsr_read(&macro->tx, NULL, sizeof(struct grid_macro_event_desc));
    dropped = 1;
  }

  grid_swsr_write(&macro->tx, &event, sizeof(struct grid_macro_event_desc));

  return dropped;
}

void grid_usb_macro_tx_flush(struct grid_macro_model* macro) {

  if (!macro->has_next) {
    if (!grid_swsr_readable(&macro->tx, sizeof(struct grid_macro_event_desc))) {
      return;
    }
    grid_swsr_read(&macro->tx, &macro->next, sizeof(struct grid_macro_event_desc));
    macro->has_next = true;
  }

  if (macro->next.ismodifier == 0 || macro->next.ismodifier == 1) {
    if (!grid_usb_hid_ready()) {
      return;
    }
    if (grid_keyboard_keychange(macro->keyboard, &macro->next)) {
      return;
    }
  } else if (macro->next.ismodifier == 2) {
    uint8_t axis = macro->next.keycode;
    int8_t position = macro->next.ispressed - 128;
    if (grid_usb_mouse_move(macro->mouse, position, axis)) {
      return;
    }
  } else if (macro->next.ismodifier == 3) {
    uint8_t state = macro->next.ispressed;
    uint8_t button = macro->next.keycode;
    if (grid_usb_mouse_button_change(macro->mouse, state, button)) {
      return;
    }
  } else if (macro->next.ismodifier == 0xf) {
  } else {
  }

  macro->has_next = false;
  macro->tx_rtc_lasttimestamp = grid_platform_rtc_get_micros();
}

bool grid_usb_macro_tx_available(struct grid_macro_model* macro) {

  if (!macro->has_next) {
    if (!grid_swsr_readable(&macro->tx, sizeof(struct grid_macro_event_desc))) {
      return false;
    }
    grid_swsr_read(&macro->tx, &macro->next, sizeof(struct grid_macro_event_desc));
    macro->has_next = true;
  }

  uint64_t elapsed = grid_platform_rtc_get_elapsed_time(macro->tx_rtc_lasttimestamp);

  return elapsed > macro->next.delay * MS_TO_US;
}

void grid_usb_keyboard_enable(struct grid_keyboard_model* kb) { kb->isenabled = 1; }

void grid_usb_keyboard_disable(struct grid_keyboard_model* kb) { kb->isenabled = 0; }

static uint8_t grid_keyboard_isenabled(struct grid_keyboard_model* kb) { return kb->isenabled; }

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

static int32_t grid_usb_mouse_button_change(struct grid_mouse_model* mouse, uint8_t b_state, uint8_t type) {
  if (b_state) {
    mouse->buttons |= type;
  } else {
    mouse->buttons &= (uint8_t)~type;
  }
  return 0 == tud_hid_mouse_report(HID_ITF_PROTOCOL_MOUSE, mouse->buttons, 0, 0, 0, 0);
}

static int32_t grid_usb_mouse_move(struct grid_mouse_model* mouse, int8_t position, uint8_t axis) {
  int8_t delta[3] = {0};
  if (axis < MOUSE_AXIS_X || axis >= MOUSE_AXIS_COUNT) {
    return 0;
  }
  delta[axis - 1] = position;
  return 0 == tud_hid_mouse_report(HID_ITF_PROTOCOL_MOUSE, mouse->buttons, delta[0], delta[1], delta[2], 0);
}

int32_t grid_usb_gamepad_axis_move(struct grid_gamepad_model* gamepad, uint8_t axis, int32_t value) {
  if (axis >= GAMEPAD_AXIS_COUNT) {
    return 0;
  }
  gamepad->axis[axis] = value;
  return 0 == tud_hid_gamepad_report(3, gamepad->axis[0], gamepad->axis[1], gamepad->axis[2], gamepad->axis[5], gamepad->axis[4], gamepad->axis[3], gamepad->hat, gamepad->buttons);
}

int32_t grid_usb_gamepad_button_change(struct grid_gamepad_model* gamepad, uint8_t button, uint8_t value) {
  if (value) {
    gamepad->buttons |= (1 << button);
  } else {
    gamepad->buttons &= ~(1 << button);
  }
  return 0 == tud_hid_gamepad_report(3, gamepad->axis[0], gamepad->axis[1], gamepad->axis[2], gamepad->axis[5], gamepad->axis[4], gamepad->axis[3], gamepad->hat, gamepad->buttons);
}

static int32_t grid_keyboard_keys_state_change(struct grid_macro_event_desc* active_key_list, uint8_t keys_count) {
  uint8_t keycode[6] = {0};
  uint8_t modifier = 0;
  uint8_t key_idx = 0;

  for (uint8_t i = 0; i < keys_count && i < GRID_KEYBOARD_KEY_maxcount; i++) {
    if (active_key_list[i].ismodifier) {
      modifier |= active_key_list[i].keycode;
    } else if (active_key_list[i].ispressed && active_key_list[i].keycode != 255 && key_idx < 6) {
      keycode[key_idx++] = active_key_list[i].keycode;
    }
  }

  return 0 == tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, modifier, keycode);
}

#else // !CFG_TUD_HID

static bool grid_usb_hid_ready(void) { return true; }

static int32_t grid_usb_mouse_button_change(struct grid_mouse_model* mouse, uint8_t b_state, uint8_t type) {
  (void)mouse;
  (void)b_state;
  (void)type;
  return 0;
}

static int32_t grid_usb_mouse_move(struct grid_mouse_model* mouse, int8_t position, uint8_t axis) {
  (void)mouse;
  (void)position;
  (void)axis;
  return 0;
}

int32_t grid_usb_gamepad_axis_move(struct grid_gamepad_model* gamepad, uint8_t axis, int32_t value) {
  (void)gamepad;
  (void)axis;
  (void)value;
  return 0;
}

int32_t grid_usb_gamepad_button_change(struct grid_gamepad_model* gamepad, uint8_t button, uint8_t value) {
  (void)gamepad;
  (void)button;
  (void)value;
  return 0;
}

static int32_t grid_keyboard_keys_state_change(struct grid_macro_event_desc* active_key_list, uint8_t keys_count) {
  (void)active_key_list;
  (void)keys_count;
  return 0;
}

#endif // CFG_TUD_HID
