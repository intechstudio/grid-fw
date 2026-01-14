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

// New buffers for SysEx and RTM
struct grid_swsr_t grid_midi_sysex_rx;
struct grid_swsr_t grid_midi_rtm_rx;

// State variable for SysEx mode
static uint8_t midi_rx_state_is_sysex = 0;

// SysEx assembly buffer for accumulating bytes until complete message
static uint8_t sysex_assembly_buffer[GRID_MIDI_SYSEX_RX_BUFFER_length];
static uint16_t sysex_assembly_index = 0;

struct grid_usb_keyboard_model grid_usb_keyboard_state;

void grid_usb_midi_buffer_init() {

  size_t typesize = sizeof(struct grid_midi_event_desc);

  assert(grid_swsr_malloc(&grid_midi_tx, GRID_MIDI_TX_BUFFER_length * typesize) == 0);
  assert(grid_swsr_malloc(&grid_midi_rx, GRID_MIDI_RX_BUFFER_length * typesize) == 0);

  // Initialize new buffers for SysEx and RTM (store raw bytes)
  assert(grid_swsr_malloc(&grid_midi_sysex_rx, GRID_MIDI_SYSEX_RX_BUFFER_length) == 0);
  assert(grid_swsr_malloc(&grid_midi_rtm_rx, GRID_MIDI_RTM_RX_BUFFER_length) == 0);
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

// Helper: Push Real-Time Message to RTM buffer
static void grid_midi_rx_push_rtm(uint8_t rtm_byte) {
  if (grid_swsr_writable(&grid_midi_rtm_rx, 1)) {
    grid_swsr_write(&grid_midi_rtm_rx, &rtm_byte, 1);
  }
}

// SysEx state machine: returns true if packet was SysEx (caller should early return)
static bool grid_midi_rx_process_sysex(uint8_t cin, uint8_t byte1, uint8_t byte2, uint8_t byte3) {

  bool is_sysex_start = (cin == GRID_MIDI_CIN_SYSEX_START && byte1 == GRID_MIDI_SYSEX_START);

  // Guard: not SysEx if not in SysEx mode and not a SysEx start
  if (!midi_rx_state_is_sysex && !is_sysex_start) {
    return false;
  }

  // Enter SysEx mode on start
  if (!midi_rx_state_is_sysex) {
    midi_rx_state_is_sysex = 1;
  }

  // Copy parameters into buffer
  uint8_t sysex_bytes[3] = {byte1, byte2, byte3};
  uint8_t bytes_to_write = 0;

  // Determine bytes to write based on CIN
  switch (cin) {
  case GRID_MIDI_CIN_SYSEX_START:
    bytes_to_write = 3;
    break;
  case GRID_MIDI_CIN_SYSEX_END_1BYTE:
    bytes_to_write = 1;
    break;
  case GRID_MIDI_CIN_SYSEX_END_2BYTE:
    bytes_to_write = 2;
    break;
  case GRID_MIDI_CIN_SYSEX_END_3BYTE:
    bytes_to_write = 3;
    break;
  }

  // Stay in SysEx mode only for start/continue packets (CIN 0x04)
  if (cin != GRID_MIDI_CIN_SYSEX_START) {
    midi_rx_state_is_sysex = 0;
  }

  // Write bytes to SysEx SWSR buffer
  if (bytes_to_write > 0 && grid_swsr_writable(&grid_midi_sysex_rx, bytes_to_write)) {
    grid_swsr_write(&grid_midi_sysex_rx, sysex_bytes, bytes_to_write);
  }

  return true;
}

// Helper: Push normal MIDI message (notes, CC, etc.)
static void grid_midi_rx_push_normal(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3) {

  // Check if MIDI RX is globally enabled
  if (grid_sys_get_midirx_any_state(&grid_sys_state) == 0) {
    return;
  }

  // Push to normal MIDI RX buffer
  struct grid_midi_event_desc event;
  event.byte0 = byte0;
  event.byte1 = byte1;
  event.byte2 = byte2;
  event.byte3 = byte3;

  if (grid_swsr_writable(&grid_midi_rx, sizeof(struct grid_midi_event_desc))) {
    grid_swsr_write(&grid_midi_rx, &event, sizeof(struct grid_midi_event_desc));
  }
}

void grid_midi_rx_push(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3) {

  uint8_t cin = byte0 & 0x0F;

  // 1. REAL-TIME MESSAGES (highest priority, processed even during SysEx)
  if (cin == GRID_MIDI_CIN_SINGLE_BYTE && byte1 >= GRID_MIDI_RTM_TIMING_CLOCK) {
    grid_midi_rx_push_rtm(byte1);
    return;
  }

  // 2. SYSEX MESSAGES
  if (grid_midi_rx_process_sysex(cin, byte1, byte2, byte3)) {
    return;
  }

  // 3. NORMAL MIDI MESSAGES
  grid_midi_rx_push_normal(byte0, byte1, byte2, byte3);
}

void grid_midi_sysex_rx_pop();

void grid_midi_rx_pop() {

  grid_midi_sysex_rx_pop();

  if (!grid_swsr_readable(&grid_midi_rx, sizeof(struct grid_midi_event_desc))) {
    return;
  }

  struct grid_msg msg = {0};
  uint8_t xy = GRID_PARAMETER_GLOBAL_POSITION;
  grid_msg_init_brc(&grid_msg_state, &msg, xy, xy);

  grid_msg_set_parameter_raw((uint8_t*)msg.data, BRC_SX, xy);
  grid_msg_set_parameter_raw((uint8_t*)msg.data, BRC_SY, xy);

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

// Helper: Process complete SysEx message and forward to modules
static void grid_midi_sysex_process_complete(uint8_t* sysex_data, uint16_t length) {

  // Validate SysEx boundaries
  if (length < 2 || sysex_data[0] != GRID_MIDI_SYSEX_START || sysex_data[length - 1] != GRID_MIDI_SYSEX_END) {
    return; // Invalid SysEx message
  }

  // Initialize broadcast message
  struct grid_msg msg = {0};
  uint8_t xy = GRID_PARAMETER_GLOBAL_POSITION;
  grid_msg_init_brc(&grid_msg_state, &msg, xy, xy);

  grid_msg_set_parameter_raw((uint8_t*)msg.data, BRC_SX, xy);
  grid_msg_set_parameter_raw((uint8_t*)msg.data, BRC_SY, xy);

  // Add MIDISYSEX frame
  grid_msg_add_frame(&msg, GRID_CLASS_MIDISYSEX_frame_start);
  grid_msg_set_parameter(&msg, INSTR, GRID_INSTR_REPORT_code);

  // Set length parameter
  grid_msg_set_parameter(&msg, CLASS_MIDISYSEX_LENGTH, length);

  // Encode each SysEx byte as 2-char hex in payload
  for (uint16_t i = 0; i < length; i++) {
    uint16_t offset = msg.offset + GRID_CLASS_MIDISYSEX_PAYLOAD_offset + i * 2;
    grid_frame_set_parameter((uint8_t*)msg.data, offset, 2, sysex_data[i]);
  }

  // Update msg.length to account for the payload we just wrote (2 hex chars per byte)
  msg.length += length * 2;

  grid_msg_add_frame(&msg, GRID_CLASS_MIDISYSEX_frame_end);

  // Send to all connected modules
  if (grid_msg_close_brc(&grid_msg_state, &msg) >= 0) {
    grid_transport_send_msg_to_all(&grid_transport_state, &msg);
  }
}

void grid_midi_sysex_rx_pop() {

  // Read byte by byte from SWSR, accumulate until 0xF7
  while (grid_swsr_readable(&grid_midi_sysex_rx, 1)) {

    uint8_t byte;
    grid_swsr_read(&grid_midi_sysex_rx, &byte, 1);

    sysex_assembly_buffer[sysex_assembly_index++] = byte;

    // Check for SysEx end marker
    if (byte == GRID_MIDI_SYSEX_END) {
      // Complete message! Process it
      grid_midi_sysex_process_complete(sysex_assembly_buffer, sysex_assembly_index);

      // Reset for next message
      sysex_assembly_index = 0;

      break; // Process only ONE message per call
    }

    // Buffer overflow protection
    if (sysex_assembly_index >= GRID_MIDI_SYSEX_RX_BUFFER_length) {
      // Message too large, discard
      sysex_assembly_index = 0;
      break;
    }
  }
}

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

  if (!grid_usb_keyboard_tx_readable(kb)) {
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
    if (grid_usb_keyboard_keychange(&grid_usb_keyboard_state, &key)) {
      return; // USB busy, keep event in buffer for retry
    }
  } else if (key.ismodifier == 2) {
    // Mouse move
    uint8_t axis = key.keycode;
    int8_t position = key.ispressed - 128;
    if (grid_platform_usb_mouse_move(position, axis)) {
      return; // USB busy, keep event in buffer for retry
    }
  } else if (key.ismodifier == 3) {
    // Mouse button
    uint8_t state = key.ispressed;
    uint8_t button = key.keycode;
    if (grid_platform_usb_mouse_button_change(state, button)) {
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

  uint64_t elapsed = grid_platform_rtc_get_elapsed_time(kb->tx_rtc_lasttimestamp);

  return elapsed > kb->tx_buffer[kb->tx_read_index].delay * MS_TO_US;
}

void grid_usb_gamepad_axis_move(uint8_t axis, int32_t move) { grid_platform_usb_gamepad_axis_move(axis, move); }

void grid_usb_gamepad_button_change(uint8_t button, uint8_t value) { grid_platform_usb_gamepad_button_change(button, value); }

void grid_usb_keyboard_enable(struct grid_usb_keyboard_model* kb) { kb->isenabled = 1; }

void grid_usb_keyboard_disable(struct grid_usb_keyboard_model* kb) { kb->isenabled = 0; }

uint8_t grid_usb_keyboard_isenabled(struct grid_usb_keyboard_model* kb) { return kb->isenabled; }
