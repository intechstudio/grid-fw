#ifndef GRID_RP2350_ENCODER_H
#define GRID_RP2350_ENCODER_H

#include <stdint.h>

#include "grid_ui_encoder.h"

// Reads a shift-register chain cascaded behind the HWCFG register (see
// grid_rp2350_platform.c's grid_platform_get_hwcfg) via RP2350's SPI0
// peripheral: GPIO1=CS (the same "SHIFT" line HWCFG uses), GPIO2=SCK (the
// same "CLOCK" line), GPIO4=MISO (the same "DATA2" line). transfer_length is
// the TOTAL bytes to shift, hwcfg byte included -- process_encoder is only
// ever handed the bytes after it. Structured after
// d51n20a/grid/d51/grid_d51_encoder.c's start-once/restart-on-complete model.

struct grid_rp2350_encoder_model {

  uint8_t* rx_buffer;
  uint8_t transfer_length;

  grid_process_encoder_t process_encoder;
};

extern struct grid_rp2350_encoder_model grid_rp2350_encoder_state;

void grid_rp2350_encoder_init(struct grid_rp2350_encoder_model* enc, uint8_t transfer_length, uint32_t clock_rate, grid_process_encoder_t process_encoder);
void grid_rp2350_encoder_start(struct grid_rp2350_encoder_model* enc);

#endif /* GRID_RP2350_ENCODER_H */
