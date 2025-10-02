/*
 * grid_led.c
 *
 * Created: 4/12/2019 5:27:13 PM
 * Author : SUKU WC

 // ===================== WS2812B-Mini SETUP ========================= //

 // The aim is to create a fast WS2812 library optimized for DMA transfers
 through the SPI interface.
 // Primary goal is to reduce CPU load allowing efficient animations.
 // Memory usage is less of a concern in this implementation.
 //
 // * Each symbol is encoded into a 4bits of serial data.
 //
 // * One LED uses 12 bytes of memory
 // * The reset pulse uses 24 bytes of memory (50us reset)
 // * The reset pulse uses 144 bytes of memory (300us reset) for WS2812-2020
 */

#include "grid_led.h"

#include <stdlib.h>
#include <string.h>

#include "grid_msg.h"

#define GRID_LED_RESET_LENGTH 144

// FRAME FORMAT for WS2812B-Mini or WS2812-2020
//
// Memory map =>   | <------------------------------------ FRAME BUFFER
// ------------------------------------------> | Memory map =>   |       |
// <---------------------------- LED DATA BUFFER
// ---------------------------------------> | Size in bytes=> |24(144)| 4      |
// 4      | 4      | 4      | 4      | 4      | 4      | 4      | 4      | 4 |
// Frame format => | RESET | LED1:G | LED1:R | LED1:B | LED2:G | LED2:R | LED2:B
// | LED3:G | LED3:R | LED3:B | IDLE  | Frame timing => | 60us  | 10us   | 10us
// | 10us   | 10us   | 10us   | 10us   | 10us   | 10us   | 10us   | N/A   |

struct grid_led_model grid_led_state;

// THE LOOKUP TABLES ARE QUITE SIMPLE, MAYBE CALCULATE THEM ON THE FLY;

uint8_t min_lookup[256] = {

    0xfe, 0xfc, 0xfa, 0xf8, 0xf6, 0xf4, 0xf2, 0xf0, 0xee, 0xec, 0xea, 0xe8, 0xe6, 0xe4, 0xe2, 0xe0, 0xde, 0xdc, 0xda, 0xd8, 0xd6, 0xd4, 0xd2, 0xd0, 0xce, 0xcc, 0xca, 0xc8, 0xc6, 0xc4, 0xc2, 0xc0,
    0xbe, 0xbc, 0xba, 0xb8, 0xb6, 0xb4, 0xb2, 0xb0, 0xae, 0xac, 0xaa, 0xa8, 0xa6, 0xa4, 0xa2, 0xa0, 0x9e, 0x9c, 0x9a, 0x98, 0x96, 0x94, 0x92, 0x90, 0x8e, 0x8c, 0x8a, 0x88, 0x86, 0x84, 0x82, 0x80,
    0x7e, 0x7c, 0x7a, 0x78, 0x76, 0x74, 0x72, 0x70, 0x6e, 0x6c, 0x6a, 0x68, 0x66, 0x64, 0x62, 0x60, 0x5e, 0x5c, 0x5a, 0x58, 0x56, 0x54, 0x52, 0x50, 0x4e, 0x4c, 0x4a, 0x48, 0x46, 0x44, 0x42, 0x40,
    0x3e, 0x3c, 0x3a, 0x38, 0x36, 0x34, 0x32, 0x30, 0x2e, 0x2c, 0x2a, 0x28, 0x26, 0x24, 0x22, 0x20, 0x1e, 0x1c, 0x1a, 0x18, 0x16, 0x14, 0x12, 0x10, 0x0e, 0x0c, 0x0a, 0x08, 0x06, 0x04, 0x02, 0x00,

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

uint8_t mid_lookup[256] = {

    0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e, 0x10, 0x12, 0x14, 0x16, 0x18, 0x1a, 0x1c, 0x1e, 0x20, 0x22, 0x24, 0x26, 0x28, 0x2a, 0x2c, 0x2e, 0x30, 0x32,
    0x34, 0x36, 0x38, 0x3a, 0x3c, 0x3e, 0x40, 0x42, 0x44, 0x46, 0x48, 0x4a, 0x4c, 0x4e, 0x50, 0x52, 0x54, 0x56, 0x58, 0x5a, 0x5c, 0x5e, 0x60, 0x62, 0x64, 0x66,
    0x68, 0x6a, 0x6c, 0x6e, 0x70, 0x72, 0x74, 0x76, 0x78, 0x7a, 0x7c, 0x7e, 0x80, 0x82, 0x84, 0x86, 0x88, 0x8a, 0x8c, 0x8e, 0x90, 0x92, 0x94, 0x96, 0x98, 0x9a,
    0x9c, 0x9e, 0xa0, 0xa2, 0xa4, 0xa6, 0xa8, 0xaa, 0xac, 0xae, 0xb0, 0xb2, 0xb4, 0xb6, 0xb8, 0xba, 0xbc, 0xbe, 0xc0, 0xc2, 0xc4, 0xc6, 0xc8, 0xca, 0xcc, 0xce,
    0xd0, 0xd2, 0xd4, 0xd6, 0xd8, 0xda, 0xdc, 0xde, 0xe0, 0xe2, 0xe4, 0xe6, 0xe8, 0xea, 0xec, 0xee, 0xf0, 0xf2, 0xf4, 0xf6, 0xf8, 0xfa, 0xfc, 0xfe,

    0xfe, 0xfc, 0xfa, 0xf8, 0xf6, 0xf4, 0xf2, 0xf0, 0xee, 0xec, 0xea, 0xe8, 0xe6, 0xe4, 0xe2, 0xe0, 0xde, 0xdc, 0xda, 0xd8, 0xd6, 0xd4, 0xd2, 0xd0, 0xce, 0xcc,
    0xca, 0xc8, 0xc6, 0xc4, 0xc2, 0xc0, 0xbe, 0xbc, 0xba, 0xb8, 0xb6, 0xb4, 0xb2, 0xb0, 0xae, 0xac, 0xaa, 0xa8, 0xa6, 0xa4, 0xa2, 0xa0, 0x9e, 0x9c, 0x9a, 0x98,
    0x96, 0x94, 0x92, 0x90, 0x8e, 0x8c, 0x8a, 0x88, 0x86, 0x84, 0x82, 0x80, 0x7e, 0x7c, 0x7a, 0x78, 0x76, 0x74, 0x72, 0x70, 0x6e, 0x6c, 0x6a, 0x68, 0x66, 0x64,
    0x62, 0x60, 0x5e, 0x5c, 0x5a, 0x58, 0x56, 0x54, 0x52, 0x50, 0x4e, 0x4c, 0x4a, 0x48, 0x46, 0x44, 0x42, 0x40, 0x3e, 0x3c, 0x3a, 0x38, 0x36, 0x34, 0x32, 0x30,
    0x2e, 0x2c, 0x2a, 0x28, 0x26, 0x24, 0x22, 0x20, 0x1e, 0x1c, 0x1a, 0x18, 0x16, 0x14, 0x12, 0x10, 0x0e, 0x0c, 0x0a, 0x08, 0x06, 0x04, 0x02, 0x00

};

uint8_t max_lookup[256] = {

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e, 0x10, 0x12, 0x14, 0x16, 0x18, 0x1a, 0x1c, 0x1e, 0x20, 0x22, 0x24, 0x26, 0x28, 0x2a, 0x2c, 0x2e, 0x30, 0x32, 0x34, 0x36, 0x38, 0x3a, 0x3c, 0x3e,
    0x40, 0x42, 0x44, 0x46, 0x48, 0x4a, 0x4c, 0x4e, 0x50, 0x52, 0x54, 0x56, 0x58, 0x5a, 0x5c, 0x5e, 0x60, 0x62, 0x64, 0x66, 0x68, 0x6a, 0x6c, 0x6e, 0x70, 0x72, 0x74, 0x76, 0x78, 0x7a, 0x7c, 0x7e,
    0x80, 0x82, 0x84, 0x86, 0x88, 0x8a, 0x8c, 0x8e, 0x90, 0x92, 0x94, 0x96, 0x98, 0x9a, 0x9c, 0x9e, 0xa0, 0xa2, 0xa4, 0xa6, 0xa8, 0xaa, 0xac, 0xae, 0xb0, 0xb2, 0xb4, 0xb6, 0xb8, 0xba, 0xbc, 0xbe,
    0xc0, 0xc2, 0xc4, 0xc6, 0xc8, 0xca, 0xcc, 0xce, 0xd0, 0xd2, 0xd4, 0xd6, 0xd8, 0xda, 0xdc, 0xde, 0xe0, 0xe2, 0xe4, 0xe6, 0xe8, 0xea, 0xec, 0xee, 0xf0, 0xf2, 0xf4, 0xf6, 0xf8, 0xfa, 0xfc, 0xfe};

uint8_t sine_lookup[256] = {
    0x80, 0x83, 0x86, 0x89, 0x8c, 0x8f, 0x92, 0x95, 0x98, 0x9c, 0x9f, 0xa2, 0xa5, 0xa8, 0xab, 0xae, 0xb0, 0xb3, 0xb6, 0xb9, 0xbc, 0xbf, 0xc1, 0xc4, 0xc7, 0xc9, 0xcc, 0xce, 0xd1, 0xd3, 0xd5, 0xd8,
    0xda, 0xdc, 0xde, 0xe0, 0xe2, 0xe4, 0xe6, 0xe8, 0xea, 0xeb, 0xed, 0xef, 0xf0, 0xf2, 0xf3, 0xf4, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfb, 0xfc, 0xfd, 0xfd, 0xfe, 0xfe, 0xfe, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xfe, 0xfe, 0xfd, 0xfd, 0xfc, 0xfc, 0xfb, 0xfa, 0xf9, 0xf8, 0xf7, 0xf6, 0xf5, 0xf4, 0xf2, 0xf1, 0xef, 0xee, 0xec, 0xeb, 0xe9, 0xe7, 0xe5, 0xe3, 0xe1, 0xdf, 0xdd, 0xdb,
    0xd9, 0xd7, 0xd4, 0xd2, 0xcf, 0xcd, 0xca, 0xc8, 0xc5, 0xc3, 0xc0, 0xbd, 0xba, 0xb8, 0xb5, 0xb2, 0xaf, 0xac, 0xa9, 0xa6, 0xa3, 0xa0, 0x9d, 0x9a, 0x97, 0x94, 0x91, 0x8e, 0x8a, 0x87, 0x84, 0x81,
    0x7e, 0x7b, 0x78, 0x75, 0x71, 0x6e, 0x6b, 0x68, 0x65, 0x62, 0x5f, 0x5c, 0x59, 0x56, 0x53, 0x50, 0x4d, 0x4a, 0x47, 0x45, 0x42, 0x3f, 0x3c, 0x3a, 0x37, 0x35, 0x32, 0x30, 0x2d, 0x2b, 0x28, 0x26,
    0x24, 0x22, 0x20, 0x1e, 0x1c, 0x1a, 0x18, 0x16, 0x14, 0x13, 0x11, 0x10, 0xe,  0xd,  0xb,  0xa,  0x9,  0x8,  0x7,  0x6,  0x5,  0x4,  0x3,  0x3,  0x2,  0x2,  0x1,  0x1,  0x0,  0x0,  0x0,  0x0,
    0x0,  0x0,  0x0,  0x1,  0x1,  0x1,  0x2,  0x2,  0x3,  0x4,  0x4,  0x5,  0x6,  0x7,  0x8,  0x9,  0xb,  0xc,  0xd,  0xf,  0x10, 0x12, 0x14, 0x15, 0x17, 0x19, 0x1b, 0x1d, 0x1f, 0x21, 0x23, 0x25,
    0x27, 0x2a, 0x2c, 0x2e, 0x31, 0x33, 0x36, 0x38, 0x3b, 0x3e, 0x40, 0x43, 0x46, 0x49, 0x4c, 0x4f, 0x51, 0x54, 0x57, 0x5a, 0x5d, 0x60, 0x63, 0x67, 0x6a, 0x6d, 0x70, 0x73, 0x76, 0x79, 0x7c, 0x80};

/** Get pointer of buffer array */
uint8_t* grid_led_get_framebuffer_pointer(struct grid_led_model* led) { return led->led_frame_buffer; }

/** Get led buffer size */
uint32_t grid_led_get_framebuffer_size(struct grid_led_model* led) { return led->led_count * 3; }

void grid_led_init(struct grid_led_model* led, uint32_t length) {

  led->led_pin = -1;
  led->tick_lastrealtime = 0;

  led->led_count = length;

  // Allocating memory for the smart buffer (2D array)
  led->led_smart_buffer = (struct LED_layer*)malloc(led->led_count * GRID_LED_LAYER_COUNT * sizeof(struct LED_layer));
  led->led_frame_buffer = (uint8_t*)malloc(3 * led->led_count * sizeof(uint8_t));

  // Allocating memory for low level color buffer for reporting
  led->led_changed_flag_array = (uint8_t*)malloc(led->led_count * sizeof(uint8_t));

  // Allocating memory for the lookup table
  led->led_lookup_sizes = (uint8_t*)malloc(led->led_count * sizeof(uint8_t));
  memset(led->led_lookup_sizes, 0, led->led_count * sizeof(uint8_t));
  led->led_lookup_table = (uint8_t**)malloc(led->led_count * sizeof(uint8_t*));
  memset(led->led_lookup_table, 0, led->led_count * sizeof(uint8_t*));

  // Clear changed flag array
  memset(led->led_changed_flag_array, 0, led->led_count * sizeof(uint8_t));

  // DEFAULT CONFIG
  grid_led_reset(led);
}

void grid_led_reset(struct grid_led_model* led) {

  for (size_t i = 0; i < led->led_count; ++i) {

    for (size_t j = 0; j < GRID_LED_LAYER_COUNT; ++j) {

      grid_led_set_layer_color(led, i, j, 0, 0, 0);
      grid_led_set_layer_frequency(led, i, j, 0);
      grid_led_set_layer_phase(led, i, j, 0);
      grid_led_set_layer_shape(led, i, j, 0);
      grid_led_set_layer_timeout(led, i, j, 0);
    }
  }
}

uint32_t grid_led_get_led_count(struct grid_led_model* led) { return led->led_count; }

void grid_led_lookup_alloc_multi(struct grid_led_model* led, uint8_t index, uint8_t length, uint8_t* values) {

  assert(index < led->led_count);
  assert(led->led_lookup_sizes[index] == 0);
  assert(led->led_lookup_table[index] == NULL);

  led->led_lookup_table[index] = malloc(length * sizeof(uint8_t));

  assert(led->led_lookup_table[index]);

  led->led_lookup_sizes[index] = length;
  memcpy(led->led_lookup_table[index], values, length * sizeof(uint8_t));
}

void grid_led_lookup_alloc_single(struct grid_led_model* led, uint8_t index, uint8_t value) { grid_led_lookup_alloc_multi(led, index, 1, &value); }

void grid_led_lookup_alloc_identity(struct grid_led_model* led, uint8_t index, uint8_t length) {

  for (uint16_t i = index; i < index + length; ++i) {

    grid_led_lookup_alloc_single(led, i, i);
  }
}

uint8_t* grid_led_lookup_get(struct grid_led_model* led, uint8_t element, uint8_t* length) {

  if (element >= led->led_count || led->led_lookup_sizes[element] == 0) {

    *length = 0;
    return NULL;
  }

  assert(led->led_lookup_table[element]);

  *length = led->led_lookup_sizes[element];
  return led->led_lookup_table[element];
}

/** ================== ANIMATION ==================  */

uint8_t grid_led_get_pin(struct grid_led_model* led) { return led->led_pin; }

void grid_led_set_pin(struct grid_led_model* led, uint8_t gpio_pin) { led->led_pin = gpio_pin; }

void grid_led_tick(struct grid_led_model* led) {

  for (uint8_t j = 0; j < led->led_count; j++) {

    for (uint8_t i = 0; i < GRID_LED_LAYER_COUNT; i++) {

      struct LED_layer* ledbuf = &led->led_smart_buffer[j + (led->led_count * i)];

      if (ledbuf->timeout != 0) {

        ledbuf->timeout--;

        if (ledbuf->timeout == 0) {
          ledbuf->fre = 0;
        }
      }

      ledbuf->pha += ledbuf->fre; // PHASE + = FREQUENCY
    }
  }
}

void grid_alert_all_set(struct grid_led_model* led, uint8_t r, uint8_t g, uint8_t b, uint16_t duration) {

  for (uint8_t i = 0; i < led->led_count; i++) {

    grid_alert_one_set(led, i, r, g, b, duration);
  }
}

void grid_alert_all_set_timeout_automatic(struct grid_led_model* led) {

  for (uint8_t i = 0; i < led->led_count; i++) {

    grid_alert_one_set_timeout_automatic(led, i);
  }
}

void grid_alert_all_set_timeout(struct grid_led_model* led, uint8_t timeout) {

  for (uint8_t i = 0; i < led->led_count; i++) {
    grid_alert_one_set_timeout(led, i, timeout);
  }
}

void grid_alert_all_set_frequency(struct grid_led_model* led, uint8_t frequency) {

  for (uint8_t i = 0; i < led->led_count; i++) {
    grid_alert_one_set_frequency(led, i, frequency);
  }
}

void grid_alert_all_set_phase(struct grid_led_model* led, uint8_t phase) {

  for (uint8_t i = 0; i < led->led_count; i++) {
    grid_alert_one_set_phase(led, i, phase);
  }
}

void grid_alert_one_set(struct grid_led_model* led, uint8_t num, uint8_t r, uint8_t g, uint8_t b, uint16_t duration) {

  grid_led_set_layer_color(led, num, GRID_LED_LAYER_ALERT, r, g, b);

  // just to make sure that minimum is 0
  grid_led_set_layer_min(led, num, GRID_LED_LAYER_ALERT, 0, 0, 0);

  grid_led_set_layer_shape(led, num, GRID_LED_LAYER_ALERT, 0);

  grid_led_set_layer_timeout(led, num, GRID_LED_LAYER_ALERT, duration);
  grid_led_set_layer_phase(led, num, GRID_LED_LAYER_ALERT, (uint8_t)duration);
  grid_led_set_layer_frequency(led, num, GRID_LED_LAYER_ALERT, -1);
}

void grid_alert_one_set_timeout_automatic(struct grid_led_model* led, uint8_t num) {

  uint8_t old_phase = grid_led_get_layer_phase(led, num, GRID_LED_LAYER_ALERT);

  uint8_t new_phase = (old_phase / 2) * 2;

  grid_led_set_layer_min(led, num, GRID_LED_LAYER_ALERT, 0, 0, 0);

  grid_alert_one_set_phase(led, num, new_phase);
  grid_alert_one_set_frequency(led, num, -2);

  if (new_phase / 2 == 0) {
    grid_alert_one_set_frequency(led, num, 0);
  } else {
    grid_alert_one_set_timeout(led, num, new_phase / 2);
  }
}

void grid_alert_one_set_timeout(struct grid_led_model* led, uint8_t num, uint8_t timeout) { grid_led_set_layer_timeout(led, num, GRID_LED_LAYER_ALERT, timeout); }

void grid_alert_one_set_frequency(struct grid_led_model* led, uint8_t num, uint8_t frequency) { grid_led_set_layer_frequency(led, num, GRID_LED_LAYER_ALERT, frequency); }

void grid_alert_one_set_phase(struct grid_led_model* led, uint8_t num, uint8_t phase) { grid_led_set_layer_phase(led, num, GRID_LED_LAYER_ALERT, phase); }

void grid_led_set_layer_color(struct grid_led_model* led, uint8_t num, uint8_t layer, uint8_t r, uint8_t g, uint8_t b) {

  grid_led_set_layer_min(led, num, layer, r / 20, g / 20, b / 20);
  grid_led_set_layer_mid(led, num, layer, r / 2, g / 2, b / 2);
  grid_led_set_layer_max(led, num, layer, r, g, b);
}

void grid_led_set_layer_min(struct grid_led_model* led, uint8_t num, uint8_t layer, uint8_t r, uint8_t g, uint8_t b) {

  if (layer < GRID_LED_LAYER_COUNT && num < led->led_count) {

    led->led_smart_buffer[num + (led->led_count * layer)].color_min.r = r;
    led->led_smart_buffer[num + (led->led_count * layer)].color_min.g = g;
    led->led_smart_buffer[num + (led->led_count * layer)].color_min.b = b;
  }
}

void grid_led_set_layer_mid(struct grid_led_model* led, uint8_t num, uint8_t layer, uint8_t r, uint8_t g, uint8_t b) {

  if (layer < GRID_LED_LAYER_COUNT && num < led->led_count) {

    led->led_smart_buffer[num + (led->led_count * layer)].color_mid.r = r;
    led->led_smart_buffer[num + (led->led_count * layer)].color_mid.g = g;
    led->led_smart_buffer[num + (led->led_count * layer)].color_mid.b = b;
  }
}

void grid_led_set_layer_max(struct grid_led_model* led, uint8_t num, uint8_t layer, uint8_t r, uint8_t g, uint8_t b) {

  if (layer < GRID_LED_LAYER_COUNT && num < led->led_count) {

    led->led_smart_buffer[num + (led->led_count * layer)].color_max.r = r;
    led->led_smart_buffer[num + (led->led_count * layer)].color_max.g = g;
    led->led_smart_buffer[num + (led->led_count * layer)].color_max.b = b;
  }
}

void grid_led_set_layer_phase(struct grid_led_model* led, uint8_t num, uint8_t layer, uint8_t val) {

  if (layer < GRID_LED_LAYER_COUNT && num < led->led_count) {

    led->led_smart_buffer[num + (led->led_count * layer)].pha = val;
  }
}

uint8_t grid_led_get_layer_phase(struct grid_led_model* led, uint8_t num, uint8_t layer) {

  if (layer < GRID_LED_LAYER_COUNT && num < led->led_count) {

    return led->led_smart_buffer[num + (led->led_count * layer)].pha;
  }

  return 0;
}

void grid_led_set_layer_frequency(struct grid_led_model* led, uint8_t num, uint8_t layer, uint8_t val) {

  if (layer < GRID_LED_LAYER_COUNT && num < led->led_count) {

    led->led_smart_buffer[num + (led->led_count * layer)].fre = val;
  }
}

void grid_led_set_layer_shape(struct grid_led_model* led, uint8_t num, uint8_t layer, uint8_t val) {

  if (layer < GRID_LED_LAYER_COUNT && num < led->led_count) {

    led->led_smart_buffer[num + (led->led_count * layer)].sha = val;
  }
}

void grid_led_set_layer_timeout(struct grid_led_model* led, uint8_t num, uint8_t layer, uint16_t val) {

  if (layer < GRID_LED_LAYER_COUNT && num < led->led_count) {

    led->led_smart_buffer[num + (led->led_count * layer)].timeout = val;
  }
}

void grid_led_framebuffer_set_color(struct grid_led_model* led, uint32_t led_index, uint16_t led_r, uint16_t led_g, uint16_t led_b) {

  if (led_r > 255) {
    led_r = 255;
  }
  if (led_g > 255) {
    led_g = 255;
  }
  if (led_b > 255) {
    led_b = 255;
  }

  if (led_index < led->led_count) {

    if (led->led_frame_buffer[led_index * 3 + 0] != led_g) {
      led->led_changed_flag_array[led_index] = 1;
      led->led_frame_buffer[led_index * 3 + 0] = led_g;
    }

    if (led->led_frame_buffer[led_index * 3 + 1] != led_r) {
      led->led_changed_flag_array[led_index] = 1;
      led->led_frame_buffer[led_index * 3 + 1] = led_r;
    }

    if (led->led_frame_buffer[led_index * 3 + 2] != led_b) {
      led->led_changed_flag_array[led_index] = 1;
      led->led_frame_buffer[led_index * 3 + 2] = led_b;
    }
  }
}

void grid_led_render_framebuffer_one(struct grid_led_model* led, uint32_t num) {

  uint32_t mix_r = 0;
  uint32_t mix_g = 0;
  uint32_t mix_b = 0;

  // Sum all layers for one LED
  for (uint8_t i = 0; i < GRID_LED_LAYER_COUNT; ++i) {

    uint8_t layer = i;

    uint8_t phase = led->led_smart_buffer[num + (led->led_count * layer)].pha;

    // SHAPES
    // 0:  ramp up
    // 1:  ramp down
    // 2:  square
    // 3:  sine
    uint8_t shape = led->led_smart_buffer[num + (led->led_count * layer)].sha;

    uint8_t intensity = phase;
    switch (shape) {
    case 0:
      intensity = phase;
      break;
    case 1:
      intensity = 255 - phase;
      break;
    case 2:
      intensity = (phase < 128) * 255;
      break;
    case 3:
      intensity = sine_lookup[phase];
      break;
    }

    size_t offset = num + led->led_count * layer;
    struct LED_color* min = &led->led_smart_buffer[offset].color_min;
    struct LED_color* mid = &led->led_smart_buffer[offset].color_mid;
    struct LED_color* max = &led->led_smart_buffer[offset].color_max;

    uint8_t min_a = min_lookup[intensity];
    uint8_t mid_a = mid_lookup[intensity];
    uint8_t max_a = max_lookup[intensity];

    mix_r += min->r * min_a + mid->r * mid_a + max->r * max_a;
    mix_g += min->g * min_a + mid->g * mid_a + max->g * max_a;
    mix_b += min->b * min_a + mid->b * mid_a + max->b * max_a;
  }

  mix_r = mix_r / 2 / 256;
  mix_g = mix_g / 2 / 256;
  mix_b = mix_b / 2 / 256;

  grid_led_framebuffer_set_color(led, num, mix_r, mix_g, mix_b);
}

void grid_led_render_framebuffer(struct grid_led_model* led) {

  for (uint32_t i = 0; i < led->led_count; i++) {

    grid_led_render_framebuffer_one(led, i);
  }
}

uint8_t grid_led_change_flag_reset(struct grid_led_model* led) {

  for (uint8_t i = 0; i < led->led_count; i++) {
    led->led_changed_flag_array[i] = 1;
  }

  return 0;
}

uint8_t grid_led_change_flag_count(struct grid_led_model* led) {

  uint8_t count = 0;

  for (uint8_t i = 0; i < led->led_count; i++) {
    if (led->led_changed_flag_array[i] != 0) {
      count++;
    }
  }

  return count;
}

uint16_t grid_protocol_led_change_report_length(struct grid_led_model* led) { return grid_led_change_flag_count(led) * 8; }

uint16_t grid_protocol_led_change_report_generate(struct grid_led_model* led, uint16_t maxlength, char* output) {

  uint16_t length = 0;

  for (uint8_t i = 0; i < led->led_count; i++) {

    if (led->led_changed_flag_array[i] == 0) {
      continue;
    }

    if (length + 8 > maxlength) {
      break;
    }

    grid_frame_set_parameter((uint8_t*)&output[length], 0, 2, i);
    grid_frame_set_parameter((uint8_t*)&output[length], 2, 2, led->led_frame_buffer[i * 3 + 1]);
    grid_frame_set_parameter((uint8_t*)&output[length], 4, 2, led->led_frame_buffer[i * 3 + 0]);
    grid_frame_set_parameter((uint8_t*)&output[length], 6, 2, led->led_frame_buffer[i * 3 + 2]);

    led->led_changed_flag_array[i] = 0;

    length += 8;
  }

  return length;
}
