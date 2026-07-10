#include <stdio.h>

#include "pico/stdlib.h"

#include "grid_led.h"
#include "grid_rp2350_adc.h"
#include "grid_rp2350_led.h"

#define NUM_PIXELS 16
#define BRIGHTNESS 40 // per-channel cap (0-255) for USB power budget

#define ADC_NUM_CHANNELS 4
#define MUX_POSITIONS 4
#define MUX_POSITIONS_BM 0x0F // all four positions valid
#define TOTAL_INPUTS (ADC_NUM_CHANNELS * MUX_POSITIONS)

static inline uint8_t dim(uint8_t v) { return (uint16_t)v * BRIGHTNESS / 255; }

// Latest reading per logical element, written from the ADC DMA ISR and read by
// the main loop -- hence volatile.
static volatile uint16_t adc_values[TOTAL_INPUTS];

// Maps the physical (ADC channel, mux position) to a logical element index,
// mirroring mux_element_lookup in grid_d51_module_po16.c. Edit these values to
// match the board wiring so element 0..15 land in the intended order.
static const uint8_t mux_element_lookup[ADC_NUM_CHANNELS][MUX_POSITIONS] = {
    {0, 1, 4, 5},     // ADC0 (GPIO26), mux positions 0..3
    {8, 9, 12, 13},   // ADC1 (GPIO27)
    {2, 3, 6, 7},     // ADC2 (GPIO28)
    {10, 11, 14, 15}, // ADC3 (GPIO29)
};

// Called once per sample from the ADC DMA ISR. Resolves the physical
// (channel, mux position) to a logical element and stores the reading.
static void adc_result_store(struct grid_adc_result* result) { adc_values[mux_element_lookup[result->channel][result->mux_state]] = result->value; }

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

  // Interrupt-driven ADC + mux scanner: fills adc_values[] in the background.
  grid_rp2350_adc_init(&grid_rp2350_adc_state, MUX_POSITIONS_BM, adc_result_store);
  grid_rp2350_adc_mux_init(&grid_rp2350_adc_state, MUX_POSITIONS_BM);
  grid_rp2350_adc_start(&grid_rp2350_adc_state);

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

    // Print all 16 muxed analog inputs twice a second (every 25 * 20 ms). The
    // values are filled in the background by the ADC DMA ISR.
    if (frame % 25 == 0) {
      printf("mux:");
      for (int i = 0; i < TOTAL_INPUTS; i++) {
        printf(" %4u", adc_values[i]);
      }
      printf("\n");
    }

    frame++;
    sleep_ms(20);
  }
}
