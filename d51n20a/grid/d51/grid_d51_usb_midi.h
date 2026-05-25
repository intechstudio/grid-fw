#pragma once

#include <stdint.h>

void grid_d51_midi_rx_poll(void);

int32_t grid_platform_usb_midi_write(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3);
int32_t grid_platform_usb_midi_write_status(void);
