#ifndef GRID_USB_MIDI_H
#define GRID_USB_MIDI_H

#include <stdbool.h>
#include <stdint.h>

#include "grid_swsr.h"

// USB MIDI Code Index Number (CIN) values
enum grid_midi_cin_type {
  GRID_MIDI_CIN_MISC = 0x00,
  GRID_MIDI_CIN_CABLE_EVENT = 0x01,
  GRID_MIDI_CIN_SYSCOM_2BYTE = 0x02,
  GRID_MIDI_CIN_SYSCOM_3BYTE = 0x03,
  GRID_MIDI_CIN_SYSEX_START = 0x04,
  GRID_MIDI_CIN_SYSEX_END_1BYTE = 0x05,
  GRID_MIDI_CIN_SYSEX_END_2BYTE = 0x06,
  GRID_MIDI_CIN_SYSEX_END_3BYTE = 0x07,
  GRID_MIDI_CIN_NOTE_OFF = 0x08,
  GRID_MIDI_CIN_NOTE_ON = 0x09,
  GRID_MIDI_CIN_POLY_KEYPRESS = 0x0A,
  GRID_MIDI_CIN_CONTROL_CHANGE = 0x0B,
  GRID_MIDI_CIN_PROGRAM_CHANGE = 0x0C,
  GRID_MIDI_CIN_CHANNEL_PRESSURE = 0x0D,
  GRID_MIDI_CIN_PITCHBEND = 0x0E,
  GRID_MIDI_CIN_SINGLE_BYTE = 0x0F
};

enum grid_midi_system_type { GRID_MIDI_SYSEX_START = 0xF0, GRID_MIDI_SYSEX_END = 0xF7 };

enum grid_midi_rtm_type {
  GRID_MIDI_RTM_TIMING_CLOCK = 0xF8,
  GRID_MIDI_RTM_UNDEFINED_F9 = 0xF9,
  GRID_MIDI_RTM_START = 0xFA,
  GRID_MIDI_RTM_CONTINUE = 0xFB,
  GRID_MIDI_RTM_STOP = 0xFC,
  GRID_MIDI_RTM_UNDEFINED_FD = 0xFD,
  GRID_MIDI_RTM_ACTIVE_SENSING = 0xFE,
  GRID_MIDI_RTM_SYSTEM_RESET = 0xFF
};

struct grid_midi_event_desc {
  uint8_t byte0;
  uint8_t byte1;
  uint8_t byte2;
  uint8_t byte3;
};

#define GRID_MIDI_TX_BUFFER_SIZE 512
#define GRID_MIDI_VOICE_RX_BUFFER_SIZE 512
#define GRID_MIDI_SYSEX_BUFFER_SIZE 256
#define GRID_MIDI_RTM_BUFFER_SIZE 32

#define GRID_MIDI_VOICE_BATCH_MAX 8
#define GRID_MIDI_RTM_BATCH_MAX 16

struct grid_usb_midi_model {
  struct grid_swsr_t tx;
  struct grid_swsr_t voice_rx;
  struct grid_swsr_t sysex_rx;
  struct grid_swsr_t rtm_rx;
  uint32_t tx_dropped;
  uint8_t sysex_assembly_buffer[GRID_MIDI_SYSEX_BUFFER_SIZE];
  uint16_t sysex_assembly_index;
  bool sysex_in_progress;
};

extern struct grid_usb_midi_model grid_usb_midi_state;

void grid_usb_midi_init(struct grid_usb_midi_model* usb_midi, uint16_t tx_buffer_size, uint16_t rx_buffer_size, uint16_t sysex_buffer_size, uint16_t rtm_buffer_size);

uint8_t grid_usb_midi_tx_push(struct grid_usb_midi_model* usb_midi, struct grid_midi_event_desc midi_event);
void grid_usb_midi_tx_flush(struct grid_usb_midi_model* usb_midi);
bool grid_usb_midi_tx_available(struct grid_usb_midi_model* usb_midi);

void grid_usb_midi_rx_queue(struct grid_usb_midi_model* usb_midi, uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3);
void grid_usb_midi_rx_voice_process(struct grid_usb_midi_model* usb_midi);

void grid_usb_midi_rx_rtm_process(struct grid_usb_midi_model* usb_midi);
void grid_usb_midi_rx_sysex_process(struct grid_usb_midi_model* usb_midi);

void grid_usb_midi_rx_poll(struct grid_usb_midi_model* usb_midi);

#endif /* GRID_USB_MIDI_H */
