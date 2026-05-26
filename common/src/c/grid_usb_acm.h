#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "grid_swsr.h"

struct grid_usb_acm_model {
  struct grid_swsr_t rx;
  bool initialized;
  uint8_t tx_ready;
};

extern struct grid_usb_acm_model grid_usb_acm_state;

void grid_usb_acm_init(struct grid_usb_acm_model* model, uint16_t rx_buffer_size);

int32_t grid_usb_acm_ready(struct grid_usb_acm_model* model);
int32_t grid_usb_acm_write(struct grid_usb_acm_model* model, char* buffer, uint32_t length);
