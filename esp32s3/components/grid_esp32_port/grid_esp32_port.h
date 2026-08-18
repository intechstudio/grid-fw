#ifndef GRID_ESP32_PORT_H
#define GRID_ESP32_PORT_H

#include <stdbool.h>
#include <stdint.h>

extern bool rp2040_active;

uint8_t grid_platform_send_grid_message(uint8_t direction, char* buffer, uint16_t length);
void grid_platform_sync1_pulse_send(void);
void grid_esp32_port_task(void* arg);

#endif /* GRID_ESP32_PORT_H */
