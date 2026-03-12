/*
 * grid_d51_led.h
 *
 * Created: 6/3/2020 5:02:04 PM
 *  Author: WPC-User
 */

#ifndef GRID_D51_LED_H_
#define GRID_D51_LED_H_

#include "grid_d51_module.h"

/*
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
#define GRID_D51_LED_RESET_LENGTH 144

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

// this is need for types like uint8_t

// Two symbols are encoded in one byte, the 4 possible combinations are defined
// here. THe NZR code specification for WS2812B-Mini allows a 0.150us deviation
// from the nominal pulse widths.
//
// * Zero code nominal: 0.40 + 0.85 (us) WS2812B-Mini
// * Zero code actual:  0.31 + 0.93 (us)
//
// * One code nominal: 0.85 + 0.40 (us) WS2812B-Mini
// * One code actual:  0.93 + 0.31 (us)
#define GRID_D51_LED_CODE_Z 0x08 // 0000 1000
#define GRID_D51_LED_CODE_O 0x0E // 0000 1110

// Reset requires a 50us LOW pulse on the output.
// At the specified 3.2Mbps serial speed this requires 24 consecutive reset
// bytes (0x00) bytes to be sent out

#define GRID_D51_LED_CODE_R 0x00 // 0000 0000

struct grid_d51_led_model {

  uint32_t led_count;
  uint32_t framebuffer_size;
  uint8_t* framebuffer;
};

extern uint32_t grid_led_color_code[256];
extern struct grid_d51_led_model grid_d51_led_state;

void grid_d51_led_init(struct grid_d51_led_model* d51_mod, struct grid_led_model* led_mod);

void grid_d51_led_generate_frame(struct grid_d51_led_model* d51_mod, struct grid_led_model* led_mod);
void grid_d51_led_start_transfer(struct grid_d51_led_model* d51_mod);

void grid_d51_led_set_color(struct grid_d51_led_model* mod, uint32_t led_index, uint8_t r, uint8_t g, uint8_t b);

void grid_led_startup_animation(struct grid_d51_led_model* mod);

#endif /* GRID_D51_LED_H_ */
