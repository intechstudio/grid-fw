#ifndef GRID_LED_H_INCLUDED
#define GRID_LED_H_INCLUDED

#include "sam.h"
#include <stdlib.h>


static uint32_t led_number;

static uint32_t led_frame_buffer_size;
static uint8_t* led_frame_buffer; // The frame buffer is used to send data to the LEDs
static uint32_t* led_frame_buffer_usable; // The part of the frame buffer that contains LED data
static uint32_t led_color_code[256];

// INITIALIZING THE GRID_LED LIBRARY
uint8_t grid_led_init(uint8_t num);

// ACCESSING THE FRAME BUFFER (READ)
uint8_t* grid_led_frame_buffer_pointer(void);
uint32_t grid_led_frame_buffer_size(void);

// RENDERING FROM SMART BUFFER TO FRAME BUFFER
void grid_led_render(uint32_t num);
void grid_led_render_all(void);

// ACCESSING THE FRAME BUFFER (WRITE)
uint8_t grid_led_set_color(uint32_t led_index, uint8_t led_r, uint8_t led_g, uint8_t led_b);

// TIME TICK FOR ANIMATIONS
void grid_led_tick(void);

/** ======================== SMART BUFFER  ========================== */
//Declaring 
struct  LED_layer;

//Smart buffer
struct LED_layer* led_smart_buffer; // 2D array if LED_Layers: Smart buffer contains the usable data coming from the API

// WRITING THE SMART BUFFER
void grid_led_set_min(uint8_t num, uint8_t layer, uint8_t r, uint8_t g, uint8_t b);
void grid_led_set_mid(uint8_t num, uint8_t layer, uint8_t r, uint8_t g, uint8_t b);
void grid_led_set_max(uint8_t num, uint8_t layer, uint8_t r, uint8_t g, uint8_t b);
void grid_led_set_phase(uint8_t num, uint8_t layer, uint8_t val);
void grid_led_set_frequency(uint8_t num, uint8_t layer, uint8_t val);

/** ======================== SMART BUFFER  ========================== */


#endif /* GRID_LED_H_INCLUDED */