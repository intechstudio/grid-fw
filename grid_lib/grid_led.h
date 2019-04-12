#ifndef GRID_LED_H_INCLUDED
#define GRID_LED_H_INCLUDED

#include "sam.h"
#include <stdlib.h>


static uint32_t led_number;
static uint32_t led_buffer_size;
static uint8_t* led_buffer;
static uint32_t* led_buffer_usable;
static uint32_t led_color_code[256];
static uint8_t led_code_z;
static uint8_t led_code_o;
static uint8_t led_code_r;

uint32_t grid_led_buffer_requirement(const uint32_t led_num);
uint8_t grid_led_buffer_register(uint8_t *buffer, const uint32_t buffersize);

uint8_t grid_led_set_color(uint32_t led_index, uint8_t led_r, uint8_t led_g, uint8_t led_b);


uint8_t grid_led_init(uint8_t num);

#endif /* GRID_LED_H_INCLUDED */