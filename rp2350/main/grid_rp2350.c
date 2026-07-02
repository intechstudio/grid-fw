#include <stdio.h>

#include "pico/stdlib.h"

#include "grid_led.h"
#include "grid_rp2350_led.h"

#define NUM_PIXELS 16
#define BRIGHTNESS 40 // per-channel cap (0-255) for USB power budget

static inline uint8_t dim(uint8_t v) { return (uint16_t)v * BRIGHTNESS / 255; }

static void wheel(uint8_t pos, uint8_t* r, uint8_t* g, uint8_t* b) {
  pos = 255 - pos;
  if (pos < 85) {
    *r = 255 - pos * 3;
    *g = 0;
    *b = pos * 3;
  } else if (pos < 170) {
    pos -= 85;
    *r = 0;
    *g = pos * 3;
    *b = 255 - pos * 3;
  } else {
    pos -= 170;
    *r = pos * 3;
    *g = 255 - pos * 3;
    *b = 0;
  }
}

int main() {
  stdio_init_all();
  printf("grid rp2350 ws2812-over-uart: %d pixels, UART1 TX=GPIO4\n", NUM_PIXELS);

  grid_led_init(&grid_led_state, NUM_PIXELS);
  grid_rp2350_led_init(&grid_rp2350_led_state, &grid_led_state);

  uint8_t frame = 0;
  while (true) {
    for (int i = 0; i < NUM_PIXELS; i++) {
      uint8_t r, g, b;
      wheel((uint8_t)(frame + i * 256 / NUM_PIXELS), &r, &g, &b);
      // Common framebuffer is G, R, B per LED (see grid_d51_led_generate_frame).
      grid_led_state.led_frame_buffer[i * 3 + 0] = dim(g);
      grid_led_state.led_frame_buffer[i * 3 + 1] = dim(r);
      grid_led_state.led_frame_buffer[i * 3 + 2] = dim(b);
    }
    grid_rp2350_led_generate_frame(&grid_rp2350_led_state, &grid_led_state);
    grid_rp2350_led_start_transfer(&grid_rp2350_led_state);
    frame++;
    sleep_ms(20);
  }
}
