#include "grid_rp2350_platform.h"

#include <string.h>

#include "pico/time.h"
#include "pico/unique_id.h"

#include "grid_platform.h"

// RP2040/RP2350 only expose a 64-bit flash unique ID (vs. the 128-bit IDs D51
// and ESP32 read), so the upper two words of the shared 4x uint32_t shape are
// left zeroed.
uint32_t grid_platform_get_id(uint32_t* return_array) {
  pico_unique_board_id_t board_id;
  pico_get_unique_board_id(&board_id);

  memset(return_array, 0, 4 * sizeof(uint32_t));
  memcpy(return_array, board_id.id, PICO_UNIQUE_BOARD_ID_SIZE_BYTES);

  return 0;
}

uint64_t grid_platform_rtc_get_micros() { return time_us_64(); }

uint64_t grid_platform_rtc_get_elapsed_time(uint64_t told) { return time_us_64() - told; }
