#pragma once

#include <stdint.h>

void grid_d51_usb_acm_init(void);

int32_t grid_platform_usb_serial_ready(void);
int32_t grid_platform_usb_serial_write(char* buffer, uint32_t length);
