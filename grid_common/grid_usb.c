/*
 * grid_usb.c
 *
 * Created: 7/6/2020 12:07:54 PM
 *  Author: suku
 */

#include "grid_usb.h"

#include <stdlib.h>

#include "grid_msg.h"
#include "grid_platform.h"
#include "grid_swsr.h"

struct grid_swsr_t grid_midi_tx;
struct grid_swsr_t grid_midi_rx;

struct grid_usb_keyboard_model grid_usb_keyboard_state;

void grid_usb_midi_buffer_init() {

  size_t typesize = sizeof(struct grid_midi_event_desc);

  assert(grid_swsr_malloc(&grid_midi_tx, GRID_MIDI_TX_BUFFER_length * typesize) == 0);
  assert(grid_swsr_malloc(&grid_midi_rx, GRID_MIDI_RX_BUFFER_length * typesize) == 0);
}

void grid_usb_keyboard_model_init(struct grid_usb_keyboard_model* kb, uint8_t buffer_length) {

  kb->tx_rtc_lasttimestamp = grid_platform_rtc_get_micros();
  kb->tx_write_index = 0;
  kb->tx_read_index = 0;

  kb->tx_buffer_length = buffer_length;

  kb->tx_buffer = (struct grid_usb_keyboard_event_desc*)malloc(buffer_length * sizeof(struct grid_usb_keyboard_event_desc));

  for (uint16_t i = 0; i < kb->tx_buffer_length; i++) {
    kb->tx_buffer[i].ismodifier = 0;
    kb->tx_buffer[i].keycode = 0;
    kb->tx_buffer[i].ispressed = 0;
    kb->tx_buffer[i].delay = 0;
  }

  for (uint8_t i = 0; i < GRID_KEYBOARD_KEY_maxcount; i++) {

    kb->active_key_list[i].ismodifier = 0;
    kb->active_key_list[i].keycode = 255;
    kb->active_key_list[i].ispressed = 0;
  }

  kb->active_key_count = 0;

  grid_usb_keyboard_enable(kb);
}

uint8_t grid_usb_keyboard_cleanup(struct grid_usb_keyboard_model* kb) {

  uint8_t changed_flag = 0;

  // Remove all inactive (released) keys
  for (uint8_t i = 0; i < kb->active_key_count; i++) {

    if (kb->active_key_list[i].ispressed == false) {

      changed_flag = 1;

      kb->active_key_list[i].ismodifier = 0;
      kb->active_key_list[i].ispressed = 0;
      kb->active_key_list[i].keycode = 255;

      // Pop item, move each remaining after this forvard one index
      for (uint8_t j = i + 1; j < kb->active_key_count; j++) {

        kb->active_key_list[j - 1] = kb->active_key_list[j];

        kb->active_key_list[j].ismodifier = 0;
        kb->active_key_list[j].ispressed = 0;
        kb->active_key_list[j].keycode = 255;
      }

      kb->active_key_count--;
      i--; // Retest this index, because it now points to a new item
    }
  }

  if (changed_flag == 1) {

    //		uint8_t debugtext[100] = {0};
    //		snprintf(debugtext, 99, "count: %d | activekeys: %d, %d, %d, %d,
    //%d, %d", kb->active_key_count, kb->active_key_list[0].keycode,
    // kb->active_key_list[1].keycode, kb->active_key_list[2].keycode,
    // kb->active_key_list[3].keycode, kb->active_key_list[4].keycode,
    // kb->active_key_list[5].keycode);
    // grid_port_debug_print_text(debugtext);

    // USB SEND
  }

  return changed_flag;
}

extern int32_t grid_platform_usb_keyboard_keys_state_change(struct grid_usb_keyboard_event_desc* active_key_list, uint8_t keys_count);

int32_t grid_usb_keyboard_keychange(struct grid_usb_keyboard_model* kb, struct grid_usb_keyboard_event_desc* key) {

  uint8_t item_index = 255;
  uint8_t changed_flag = 0;

  grid_usb_keyboard_cleanup(kb);

  for (uint8_t i = 0; i < kb->active_key_count; i++) {

    if (kb->active_key_list[i].keycode == key->keycode && kb->active_key_list[i].ismodifier == key->ismodifier) {
      // key is already in the list
      item_index = i;

      if (kb->active_key_list[i].ispressed == true) {

        if (key->ispressed == true) {
          // OK nothing to do here
        } else {
          // Release the damn key
          kb->active_key_list[i].ispressed = false;
          changed_flag = 1;
        }
      }
    }
  }

  grid_usb_keyboard_cleanup(kb);

  if (item_index == 255) {

    // item not in list

    if (kb->active_key_count < GRID_KEYBOARD_KEY_maxcount) {

      if (key->ispressed == true) {

        kb->active_key_list[kb->active_key_count] = *key;
        kb->active_key_count++;
        changed_flag = 1;
      }
    } else {
      // grid_port_debug_print_text("activekeys limit hit!");
    }
  }

  if (changed_flag == 1) {

    if (grid_usb_keyboard_isenabled(kb)) {

      int32_t result = grid_platform_usb_keyboard_keys_state_change(kb->active_key_list, kb->active_key_count);
      return result; // Return USB status (0=success, non-zero=busy/error)
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

      return 0; // Keyboard disabled, but not an error
    }
  }

  return 0; // No change, nothing to send
}

uint8_t grid_midi_tx_push(struct grid_midi_event_desc event) {

  uint8_t dropped = 0;

  if (!grid_swsr_writable(&grid_midi_tx, sizeof(struct grid_midi_event_desc))) {

    // Pop as many bytes as we would like to push, to make space
    // (this modifies the read address, and is done under the assumption that
    // there are no concurrent reads and writes in the context of midi tx)
    grid_swsr_read(&grid_midi_tx, NULL, sizeof(struct grid_midi_event_desc));

    dropped = 1;
  }

  grid_swsr_write(&grid_midi_tx, &event, sizeof(struct grid_midi_event_desc));

  return dropped;
}

void grid_midi_tx_pop() {

  if (!grid_swsr_readable(&grid_midi_tx, sizeof(struct grid_midi_event_desc))) {
    return;
  }

  if (grid_platform_usb_midi_write_status() == 1) {
    return;
  }

  struct grid_midi_event_desc event;
  grid_swsr_read(&grid_midi_tx, &event, sizeof(struct grid_midi_event_desc));

  grid_platform_usb_midi_write(event.byte0, event.byte1, event.byte2, event.byte3);
}

bool grid_midi_tx_readable() { return grid_swsr_readable(&grid_midi_tx, sizeof(struct grid_midi_event_desc)); }

void grid_midi_rx_push(struct grid_midi_event_desc event) {

  // Factored into here from calling contexts, even if part of a deprecated feature
  if ((event.byte0 == 8 || event.byte0 == 10 || event.byte0 == 12) && event.byte1 == 240) {
    grid_platform_sync1_pulse_send();
  }

  if (grid_sys_get_midirx_any_state(&grid_sys_state) == 0) {
    return;
  }

  if (grid_sys_get_midirx_sync_state(&grid_sys_state) == 0) {

    // midi clock message
    if (event.byte0 == 8 && event.byte1 == 240) {
      return;
    }

    // midi start message
    if (event.byte0 == 10 && event.byte1 == 240) {
      return;
    }

    // midi stop message
    if (event.byte0 == 12 && event.byte1 == 240) {
      return;
    }
  }

  if (!grid_swsr_writable(&grid_midi_rx, sizeof(struct grid_midi_event_desc))) {
    return;
  }

  grid_swsr_write(&grid_midi_rx, &event, sizeof(struct grid_midi_event_desc));
}

void grid_midi_rx_pop() {

  if (!grid_swsr_readable(&grid_midi_rx, sizeof(struct grid_midi_event_desc))) {
    return;
  }

  struct grid_msg msg;
  uint8_t xy = GRID_PARAMETER_GLOBAL_POSITION;
  grid_msg_init_brc(&grid_msg_state, &msg, xy, xy);

  grid_msg_set_parameter(&msg, BRC_SX, xy);
  grid_msg_set_parameter(&msg, BRC_SY, xy);

  // Combine up to 8 midi messages into a packet
  for (uint8_t i = 0; i < 8; ++i) {

    if (!grid_swsr_readable(&grid_midi_rx, sizeof(struct grid_midi_event_desc))) {
      break;
    }

    struct grid_midi_event_desc event;
    grid_swsr_read(&grid_midi_rx, &event, sizeof(struct grid_midi_event_desc));

    grid_msg_add_frame(&msg, GRID_CLASS_MIDI_frame);
    grid_msg_set_parameter(&msg, INSTR, GRID_INSTR_REPORT_code);

    grid_msg_set_parameter(&msg, CLASS_MIDI_CHANNEL, event.byte0);
    grid_msg_set_parameter(&msg, CLASS_MIDI_COMMAND, event.byte1);
    grid_msg_set_parameter(&msg, CLASS_MIDI_PARAM1, event.byte2);
    grid_msg_set_parameter(&msg, CLASS_MIDI_PARAM2, event.byte3);
  }

  if (grid_msg_close_brc(&grid_msg_state, &msg) >= 0) {
    grid_transport_send_msg_to_all(&grid_transport_state, &msg);
  }
}

bool grid_midi_rx_writable() { return grid_swsr_writable(&grid_midi_rx, sizeof(struct grid_midi_event_desc)); }

uint8_t grid_usb_keyboard_tx_push(struct grid_usb_keyboard_model* kb, struct grid_usb_keyboard_event_desc keyboard_event) {

  // printf("R: %d, W: %d\r\n", grid_midi_tx_read_index,
  // grid_midi_tx_write_index); printf("kb tx R: %d, W: %d\r\n",
  // kb->tx_read_index, kb->tx_write_index);

  kb->tx_buffer[kb->tx_write_index] = keyboard_event;

  kb->tx_write_index = (kb->tx_write_index + 1) % kb->tx_buffer_length;

  uint32_t space_in_buffer = (kb->tx_read_index - kb->tx_write_index + kb->tx_buffer_length) % kb->tx_buffer_length;

  uint8_t return_packet_was_dropped = 0;

  if (space_in_buffer == 0) {
    return_packet_was_dropped = 1;
    // Increment the read index to drop latest packet and make space for a new
    // one.
    kb->tx_read_index = (kb->tx_read_index + 1) % kb->tx_buffer_length;
  }

  // printf("W: %d %d : %d\r\n", kb->tx_write_index, kb->tx_read_index,
  // space_in_buffer);

  return return_packet_was_dropped;
}

void grid_usb_keyboard_tx_pop(struct grid_usb_keyboard_model* kb) {

  // Guard: return if buffer is empty
  if (kb->tx_read_index == kb->tx_write_index) {
    return;
  }

  uint32_t elapsed = grid_platform_rtc_get_elapsed_time(kb->tx_rtc_lasttimestamp);

  // Guard: wait if not enough time has elapsed
  if (elapsed <= kb->tx_buffer[kb->tx_read_index].delay * MS_TO_US) {
    return;
  }

  struct grid_usb_keyboard_event_desc key;

  key.ismodifier = kb->tx_buffer[kb->tx_read_index].ismodifier;
  key.keycode = kb->tx_buffer[kb->tx_read_index].keycode;
  key.ispressed = kb->tx_buffer[kb->tx_read_index].ispressed;
  key.delay = 0;

  // 0: no, 1: yes, 2: mousemove, 3: mousebutton, f: delay

  if (key.ismodifier == 0 || key.ismodifier == 1) {
    // Keyboard event
    if (0 != grid_usb_keyboard_keychange(&grid_usb_keyboard_state, &key)) {
      return; // USB busy, keep event in buffer for retry
    }
  } else if (key.ismodifier == 2) {
    // Mouse move
    uint8_t axis = key.keycode;
    int8_t position = key.ispressed - 128;
    if (0 != grid_platform_usb_mouse_move(position, axis)) {
      return; // USB busy, keep event in buffer for retry
    }
  } else if (key.ismodifier == 3) {
    // Mouse button
    uint8_t state = key.ispressed;
    uint8_t button = key.keycode;
    if (0 != grid_platform_usb_mouse_button_change(state, button)) {
      return; // USB busy, keep event in buffer for retry
    }
  } else if (key.ismodifier == 0xf) {
    // Delay event, nothing to do
  } else {
    // Invalid event type, discard
  }

  // Event successfully processed, advance read pointer
  kb->tx_read_index = (kb->tx_read_index + 1) % kb->tx_buffer_length;
  kb->tx_rtc_lasttimestamp = grid_platform_rtc_get_micros();
}

bool grid_usb_keyboard_tx_readable(struct grid_usb_keyboard_model* kb) {

  if (kb->tx_read_index == kb->tx_write_index) {
    return false;
  }

  uint32_t elapsed = grid_platform_rtc_get_elapsed_time(kb->tx_rtc_lasttimestamp);

  return elapsed > kb->tx_buffer[kb->tx_read_index].delay * MS_TO_US;
}

void grid_usb_gamepad_axis_move(uint8_t axis, int32_t move) { grid_platform_usb_gamepad_axis_move(axis, move); }

void grid_usb_gamepad_button_change(uint8_t button, uint8_t value) { grid_platform_usb_gamepad_button_change(button, value); }

void grid_usb_keyboard_enable(struct grid_usb_keyboard_model* kb) { kb->isenabled = 1; }

void grid_usb_keyboard_disable(struct grid_usb_keyboard_model* kb) { kb->isenabled = 0; }

uint8_t grid_usb_keyboard_isenabled(struct grid_usb_keyboard_model* kb) { return kb->isenabled; }
