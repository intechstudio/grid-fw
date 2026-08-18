#include "grid_rp2350_usb.h"

#include "grid_platform.h"
#include "grid_usb.h"

// Required extern from grid_usb.h. Unused hook on every platform today (D51
// increments a counter nothing external reads; ESP32 stubs it the same way).
void grid_platform_sync1_pulse_send() {}

void grid_rp2350_usb_init(void) {
  uint32_t id[4] = {0};
  grid_platform_get_id(id);

  static char serial[33];
  grid_platform_id_to_hex((const uint8_t*)id, 16, serial);

  // VID 0x2E8A is Raspberry Pi Trading Ltd's (TinyUSB's own default for
  // RP2040/RP2350 boards); PID 0x000A is a placeholder to revisit before
  // production, matching D51 (0x03eb) and ESP32-S3 (0x303a) using their own
  // chip vendor's VID as a placeholder.
  grid_usb_init(0x2E8A, 0x000A, serial);
}
