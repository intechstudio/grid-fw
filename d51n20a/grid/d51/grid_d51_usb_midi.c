#include "grid_d51_usb_midi.h"

#include "tusb.h"

#include "grid_usb.h"

void grid_d51_midi_rx_poll(void) {
  uint8_t packet[4];

  while (tud_midi_available()) {
    if (!grid_midi_rx_writable()) {
      break;
    }
    if (tud_midi_packet_read(packet)) {
      grid_midi_rx_push(packet[0], packet[1], packet[2], packet[3]);
    }
  }
}

int32_t grid_platform_usb_midi_write(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3) {
  const uint8_t buf[4] = {byte0, byte1, byte2, byte3};
  if (tud_midi_mounted()) {
    tud_midi_packet_write(buf);
  }
  return 0;
}

int32_t grid_platform_usb_midi_write_status(void) { return 0; }
