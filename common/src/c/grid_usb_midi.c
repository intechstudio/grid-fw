#include "grid_usb_midi.h"

#include <assert.h>

#include "tusb.h"

#include "grid_msg.h"
#include "grid_platform.h"
#include "grid_swsr.h"
#include "grid_transport.h"
#include "grid_usb.h"

struct grid_usb_midi_model grid_usb_midi_state;

void grid_usb_midi_init(struct grid_usb_midi_model* usb_midi, uint16_t tx_buffer_size, uint16_t rx_buffer_size, uint16_t sysex_buffer_size, uint16_t rtm_buffer_size) {

  usb_midi->has_next = false;

  assert(grid_swsr_malloc(&usb_midi->tx, tx_buffer_size) == 0);
  assert(grid_swsr_malloc(&usb_midi->rx, rx_buffer_size) == 0);

  assert(grid_swsr_malloc(&usb_midi->sysex_rx, sysex_buffer_size) == 0);
  assert(grid_swsr_malloc(&usb_midi->rtm_rx, rtm_buffer_size) == 0);
}

uint8_t grid_usb_midi_tx_queue(struct grid_usb_midi_model* usb_midi, struct grid_midi_event_desc event) {

  uint8_t dropped = 0;

  if (!grid_swsr_writable(&usb_midi->tx, sizeof(struct grid_midi_event_desc))) {

    grid_swsr_read(&usb_midi->tx, NULL, sizeof(struct grid_midi_event_desc));

    dropped = 1;
  }

  grid_swsr_write(&usb_midi->tx, &event, sizeof(struct grid_midi_event_desc));

  return dropped;
}

void grid_usb_midi_tx_flush(struct grid_usb_midi_model* usb_midi) {

  if (!usb_midi->has_next) {
    if (!grid_swsr_readable(&usb_midi->tx, sizeof(struct grid_midi_event_desc))) {
      return;
    }
    grid_swsr_read(&usb_midi->tx, &usb_midi->next, sizeof(struct grid_midi_event_desc));
    usb_midi->has_next = true;
  }

  if (!tud_midi_mounted()) {
    return;
  }

  const uint8_t buffer[] = {usb_midi->next.byte0, usb_midi->next.byte1, usb_midi->next.byte2, usb_midi->next.byte3};
  if (!tud_midi_packet_write(buffer)) {
    grid_port_debug_printf("MIDI TX FIFO full, retrying event %02x%02x%02x%02x", usb_midi->next.byte0, usb_midi->next.byte1, usb_midi->next.byte2, usb_midi->next.byte3);
    return;
  }

  usb_midi->has_next = false;
}

bool grid_usb_midi_tx_available(struct grid_usb_midi_model* usb_midi) { return usb_midi->has_next || grid_swsr_readable(&usb_midi->tx, sizeof(struct grid_midi_event_desc)); }

static void grid_usb_midi_rx_queue_rtm(struct grid_usb_midi_model* usb_midi, uint8_t rtm_byte) {
  if (!(grid_sys_get_rx_mode(&grid_sys_state, GRID_RX_TYPE_MIDIRTM) & GRID_RX_MODE_FORWARD_FROM_USB)) {
    return;
  }
  if (grid_swsr_writable(&usb_midi->rtm_rx, 1)) {
    grid_swsr_write(&usb_midi->rtm_rx, &rtm_byte, 1);
  }
}

static int grid_midi_rx_process_sysex(struct grid_usb_midi_model* usb_midi, uint8_t cin, uint8_t byte1, uint8_t byte2, uint8_t byte3) {

  bool is_sysex_start = (cin == GRID_MIDI_CIN_SYSEX_START && byte1 == GRID_MIDI_SYSEX_START);

  if (!usb_midi->sysex_in_progress && !is_sysex_start) {
    return 0;
  }

  usb_midi->sysex_in_progress = (cin == GRID_MIDI_CIN_SYSEX_START);

  switch (cin) {
  case GRID_MIDI_CIN_SYSEX_START:
    return 3;
  case GRID_MIDI_CIN_SYSEX_END_1BYTE:
    return 1;
  case GRID_MIDI_CIN_SYSEX_END_2BYTE:
    return 2;
  case GRID_MIDI_CIN_SYSEX_END_3BYTE:
    return 3;
  }

  return 0;
}

static void grid_usb_midi_rx_queue_normal(struct grid_usb_midi_model* usb_midi, uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3) {

  if (!(grid_sys_get_rx_mode(&grid_sys_state, GRID_RX_TYPE_MIDIVOICE) & GRID_RX_MODE_FORWARD_FROM_USB)) {
    return;
  }

  struct grid_midi_event_desc event = {byte0, byte1, byte2, byte3};
  if (grid_swsr_writable(&usb_midi->rx, sizeof(struct grid_midi_event_desc))) {
    grid_swsr_write(&usb_midi->rx, &event, sizeof(struct grid_midi_event_desc));
  }
}

static void grid_usb_midi_rx_queue_sysex(struct grid_usb_midi_model* usb_midi, uint8_t sysex_length, uint8_t byte1, uint8_t byte2, uint8_t byte3) {

  if (!(grid_sys_get_rx_mode(&grid_sys_state, GRID_RX_TYPE_MIDISYSEX) & GRID_RX_MODE_FORWARD_FROM_USB)) {
    return;
  }

  if (!grid_swsr_writable(&usb_midi->sysex_rx, sysex_length)) {
    return;
  }

  uint8_t bytes[3] = {byte1, byte2, byte3};
  grid_swsr_write(&usb_midi->sysex_rx, bytes, sysex_length);
}

void grid_usb_midi_rx_queue(struct grid_usb_midi_model* usb_midi, uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3) {

  uint8_t cin = byte0 & 0x0F;

  if (byte1 >= GRID_MIDI_RTM_TIMING_CLOCK) {
    grid_usb_midi_rx_queue_rtm(usb_midi, byte1);
    return;
  }

  int sysex_length = grid_midi_rx_process_sysex(usb_midi, cin, byte1, byte2, byte3);
  if (sysex_length) {
    grid_usb_midi_rx_queue_sysex(usb_midi, sysex_length, byte1, byte2, byte3);
    return;
  }

  grid_usb_midi_rx_queue_normal(usb_midi, byte0, byte1, byte2, byte3);
}

void grid_usb_midi_rx_voice_process(struct grid_usb_midi_model* usb_midi) {

  if (!grid_swsr_readable(&usb_midi->rx, sizeof(struct grid_midi_event_desc))) {
    return;
  }

  struct grid_msg msg = {0};
  uint8_t xy = GRID_PARAMETER_GLOBAL_POSITION;
  grid_msg_init_brc(&grid_msg_state, &msg, xy, xy);

  grid_msg_set_parameter_raw((uint8_t*)msg.data, BRC_SX, xy);
  grid_msg_set_parameter_raw((uint8_t*)msg.data, BRC_SY, xy);

  for (uint8_t i = 0; i < GRID_MIDI_VOICE_BATCH_MAX; ++i) {

    if (!grid_swsr_readable(&usb_midi->rx, sizeof(struct grid_midi_event_desc))) {
      break;
    }

    struct grid_midi_event_desc event;
    grid_swsr_read(&usb_midi->rx, &event, sizeof(struct grid_midi_event_desc));

    grid_msg_add_frame(&msg, GRID_CLASS_MIDI_frame);
    grid_msg_set_parameter(&msg, INSTR, GRID_INSTR_REPORT_code);

    grid_msg_set_parameter(&msg, CLASS_MIDI_CHANNEL, event.byte1 & 0x0f);
    grid_msg_set_parameter(&msg, CLASS_MIDI_COMMAND, event.byte1 & 0xf0);
    grid_msg_set_parameter(&msg, CLASS_MIDI_PARAM1, event.byte2);
    grid_msg_set_parameter(&msg, CLASS_MIDI_PARAM2, event.byte3);
  }

  if (grid_msg_close_brc(&grid_msg_state, &msg) >= 0) {
    uint8_t mode = grid_sys_get_rx_mode(&grid_sys_state, GRID_RX_TYPE_MIDIVOICE);
    if (mode & GRID_RX_MODE_FORWARD_FROM_USB) {
      grid_transport_send_msg_to_all(&grid_transport_state, &msg);
    } else {
      grid_transport_send_msg_to_ui(&grid_transport_state, &msg);
    }
  }
}

bool grid_usb_midi_rx_writable(struct grid_usb_midi_model* usb_midi) { return grid_swsr_writable(&usb_midi->rx, sizeof(struct grid_midi_event_desc)); }

void grid_usb_midi_rx_rtm_process(struct grid_usb_midi_model* usb_midi) {

  if (!grid_swsr_readable(&usb_midi->rtm_rx, 1)) {
    return;
  }

  struct grid_msg msg = {0};
  uint8_t xy = GRID_PARAMETER_GLOBAL_POSITION;
  grid_msg_init_brc(&grid_msg_state, &msg, xy, xy);

  grid_msg_set_parameter_raw((uint8_t*)msg.data, BRC_SX, xy);
  grid_msg_set_parameter_raw((uint8_t*)msg.data, BRC_SY, xy);

  for (uint8_t i = 0; i < GRID_MIDI_RTM_BATCH_MAX; ++i) {

    if (!grid_swsr_readable(&usb_midi->rtm_rx, 1)) {
      break;
    }

    uint8_t rtm_byte;
    grid_swsr_read(&usb_midi->rtm_rx, &rtm_byte, 1);

    grid_msg_add_frame(&msg, GRID_CLASS_MIDIRTM_frame);
    grid_msg_set_parameter(&msg, INSTR, GRID_INSTR_REPORT_code);
    grid_msg_set_parameter(&msg, CLASS_MIDIRTM_BYTE, rtm_byte);
  }

  if (grid_msg_close_brc(&grid_msg_state, &msg) >= 0) {
    uint8_t mode = grid_sys_get_rx_mode(&grid_sys_state, GRID_RX_TYPE_MIDIRTM);
    if (mode & GRID_RX_MODE_FORWARD_FROM_USB) {
      grid_transport_send_msg_to_all(&grid_transport_state, &msg);
    } else {
      grid_transport_send_msg_to_ui(&grid_transport_state, &msg);
    }
  }
}

static void grid_midi_sysex_process_complete(struct grid_usb_midi_model* usb_midi) {

  uint8_t* sysex_data = usb_midi->sysex_assembly_buffer;
  uint16_t length = usb_midi->sysex_assembly_index;

  if (length < 2 || sysex_data[0] != GRID_MIDI_SYSEX_START || sysex_data[length - 1] != GRID_MIDI_SYSEX_END) {
    return;
  }

  struct grid_msg msg = {0};
  uint8_t xy = GRID_PARAMETER_GLOBAL_POSITION;
  grid_msg_init_brc(&grid_msg_state, &msg, xy, xy);

  grid_msg_set_parameter_raw((uint8_t*)msg.data, BRC_SX, xy);
  grid_msg_set_parameter_raw((uint8_t*)msg.data, BRC_SY, xy);

  grid_msg_add_frame(&msg, GRID_CLASS_MIDISYSEX_frame_start);
  grid_msg_set_parameter(&msg, INSTR, GRID_INSTR_REPORT_code);

  grid_msg_set_parameter(&msg, CLASS_MIDISYSEX_LENGTH, length);

  if (grid_msg_add_hex_bytes(&msg, sysex_data, length) < 0) {
    return;
  }

  grid_msg_add_frame(&msg, GRID_CLASS_MIDISYSEX_frame_end);

  if (grid_msg_close_brc(&grid_msg_state, &msg) >= 0) {
    uint8_t mode = grid_sys_get_rx_mode(&grid_sys_state, GRID_RX_TYPE_MIDISYSEX);
    if (mode & GRID_RX_MODE_FORWARD_FROM_USB) {
      grid_transport_send_msg_to_all(&grid_transport_state, &msg);
    } else {
      grid_transport_send_msg_to_ui(&grid_transport_state, &msg);
    }
  }
}

void grid_usb_midi_rx_sysex_process(struct grid_usb_midi_model* usb_midi) {

  uint8_t byte = 0;
  while (byte != GRID_MIDI_SYSEX_END) {

    if (!grid_swsr_readable(&usb_midi->sysex_rx, 1)) {
      return;
    }

    grid_swsr_read(&usb_midi->sysex_rx, &byte, 1);

    if (usb_midi->sysex_assembly_index >= GRID_MIDI_SYSEX_BUFFER_SIZE) {
      usb_midi->sysex_assembly_index = 0;
    }

    assert(usb_midi->sysex_assembly_index < GRID_MIDI_SYSEX_BUFFER_SIZE);
    usb_midi->sysex_assembly_buffer[usb_midi->sysex_assembly_index++] = byte;
  }

  assert(byte == GRID_MIDI_SYSEX_END);
  grid_midi_sysex_process_complete(usb_midi);
  usb_midi->sysex_assembly_index = 0;
}

#if CFG_TUD_MIDI

void tud_midi_rx_cb(uint8_t itf) {
  (void)itf;

  uint8_t packet[4];

  while (tud_midi_available()) {
    if (!grid_usb_midi_rx_writable(&grid_usb_midi_state)) {
      break;
    }
    if (tud_midi_packet_read(packet)) {
      grid_usb_midi_rx_queue(&grid_usb_midi_state, packet[0], packet[1], packet[2], packet[3]);
    }
  }
}

void grid_usb_midi_rx_poll(struct grid_usb_midi_model* usb_midi) {
  tud_midi_rx_cb(0);
  (void)usb_midi;
}

#else // !CFG_TUD_MIDI

void grid_usb_midi_rx_poll(struct grid_usb_midi_model* usb_midi) { (void)usb_midi; }

#endif // CFG_TUD_MIDI
