/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_usb_midi.h"

#include "tinyusb.h"
#include "grid_usb.h"

#if CFG_TUD_MIDI

void tud_midi_rx_cb(uint8_t itf) {

  (void)itf;

  // The MIDI interface always creates input and output port/jack descriptors
  // regardless of these being used or not. Therefore incoming traffic should be
  // read (possibly just discarded) to avoid the sender blocking in IO
  uint8_t packet[4];

  while (tud_midi_available()) {

    if (!grid_midi_rx_writable()) {
      break;
    }

    if (tud_midi_packet_read(packet)) {

      uint8_t channel = packet[1] & 0x0f;
      uint8_t command = packet[1] & 0xf0;
      uint8_t param1 = packet[2];
      uint8_t param2 = packet[3];

      struct grid_midi_event_desc midi_ev;

      midi_ev.byte0 = channel;
      midi_ev.byte1 = command;
      midi_ev.byte2 = param1;
      midi_ev.byte3 = param2;

      grid_midi_rx_push(midi_ev);
    }
  }
}

int32_t grid_platform_usb_midi_write(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3) {

  const uint8_t buffer[] = {byte0, byte1, byte2, byte3};

  if (tud_midi_mounted()) {
    tud_midi_packet_write(buffer);
  }

  return 0;
}

int32_t grid_platform_usb_midi_write_status(void) {
  return 0;
}

#else // !CFG_TUD_MIDI - stub implementations

int32_t grid_platform_usb_midi_write(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3) {
  (void)byte0;
  (void)byte1;
  (void)byte2;
  (void)byte3;
  return 0;
}

int32_t grid_platform_usb_midi_write_status(void) {
  return 0;
}

#endif // CFG_TUD_MIDI
