#ifndef GRID_ESP32_LED_H
#define GRID_ESP32_LED_H

#include <stdint.h>

#include "esp_check.h"
#include "esp_log.h"

#include <stdlib.h>
#include <string.h>

#include "grid_led.h"
#include "grid_utask.h"

#include "driver/rmt_tx.h"
#include "led_strip_encoder.h"

void grid_esp32_led_start(uint8_t led_gpio);
void grid_esp32_utask_led(struct grid_utask_timer* timer);

#endif /* GRID_ESP32_LED_H */
