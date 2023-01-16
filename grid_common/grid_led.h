#pragma once

#ifndef GRID_LED_H_INCLUDED
#define GRID_LED_H_INCLUDED

// only for uint definitions
#include  <stdint.h>
// only for malloc
#include  <stdlib.h>

// only for grid_protocol_... specific functions
#include "grid_msg.h"

#define GRID_LED_LAYER_ALERT	0
#define GRID_LED_LAYER_UI_A		1
#define GRID_LED_LAYER_UI_B		2

#define GRID_LED_LAYER_COUNT 3


#define GRID_LED_COLOR_PURPLE	255,0,255
#define GRID_LED_COLOR_YELLOW	255,255,0
#define GRID_LED_COLOR_YELLOW_DIM	127,127,0
#define GRID_LED_COLOR_RED		255,0,0
#define GRID_LED_COLOR_GREEN	0,255,0
#define GRID_LED_COLOR_BLUE		0,0,255
#define GRID_LED_COLOR_WHITE	255,255,255
#define GRID_LED_COLOR_WHITE_DIM	127,127,127

struct LED_color{
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

struct LED_layer
{
	struct LED_color color_min;
	struct LED_color color_mid;
	struct LED_color color_max;
	
	uint8_t pha; //PHASE
	uint8_t fre; //FREQUENCY
	uint8_t sha; //SHAPE
	uint16_t timeout; //ANIMATION TIMEOUT
};


struct grid_led_model{

	uint8_t led_count;
	uint8_t* led_frame_buffer; // The frame buffer is used to send data to the LEDs

	uint8_t* led_changed_flag_array;

	uint32_t tick_lastrealtime;

	struct LED_layer* led_smart_buffer; // 2D array if LED_Layers: Smart buffer contains the usable data coming from the API


};


uint8_t* grid_led_get_framebuffer_pointer(struct grid_led_model* mod);
uint32_t grid_led_get_framebuffer_size(struct grid_led_model* mod);

extern struct grid_led_model grid_led_state;

void grid_led_init(struct grid_led_model* mod, uint32_t length);

uint8_t grid_led_change_flag_reset(struct grid_led_model* mod);
uint8_t grid_led_change_flag_count(struct grid_led_model* mod);


// RENDERING FROM SMART BUFFER TO FRAME BUFFER
void grid_led_render_framebuffer_one(struct grid_led_model* mod, uint32_t num);
void grid_led_render_framebuffer(struct grid_led_model* mod);

// ACCESSING THE FRAME BUFFER (WRITE)
uint8_t grid_led_framebuffer_set_color(struct grid_led_model* mod, uint32_t led_index, uint16_t led_r, uint16_t led_g, uint16_t led_b);

// TIME TICK FOR ANIMATIONS
void grid_led_tick(struct grid_led_model* mod);

void grid_led_set_tick_lastrealtime(struct grid_led_model* mod, uint32_t lastrealtime);
uint32_t grid_led_get_tick_lastrealtime(struct grid_led_model* mod);

void grid_led_set_alert(struct grid_led_model* mod, uint8_t r, uint8_t g, uint8_t b, uint16_t duration);

void grid_led_set_alert_timeout(struct grid_led_model* mod, uint8_t timeout);
void grid_led_set_alert_timeout_automatic(struct grid_led_model* mod);
void grid_led_set_alert_frequency(struct grid_led_model* mod, uint8_t frequency);
void grid_led_set_alert_phase(struct grid_led_model* mod, uint8_t phase);

// reset the state of the led smartbuffer
void grid_led_reset(struct grid_led_model* mod);

// WRITING THE SMART BUFFER
void grid_led_set_layer_color(struct grid_led_model* mod, uint8_t num, uint8_t layer, uint8_t r, uint8_t g, uint8_t b);

void grid_led_set_layer_min(struct grid_led_model* mod, uint8_t num, uint8_t layer, uint8_t r, uint8_t g, uint8_t b);
void grid_led_set_layer_mid(struct grid_led_model* mod, uint8_t num, uint8_t layer, uint8_t r, uint8_t g, uint8_t b);
void grid_led_set_layer_max(struct grid_led_model* mod, uint8_t num, uint8_t layer, uint8_t r, uint8_t g, uint8_t b);

uint8_t  grid_led_get_layer_phase(struct grid_led_model* mod, uint8_t num, uint8_t layer);
void grid_led_set_layer_phase(struct grid_led_model* mod, uint8_t num, uint8_t layer, uint8_t val);

void grid_led_set_layer_frequency(struct grid_led_model* mod, uint8_t num, uint8_t layer, uint8_t val);
void grid_led_set_layer_shape(struct grid_led_model* mod, uint8_t num, uint8_t layer, uint8_t val);
void grid_led_set_layer_timeout(struct grid_led_model* mod, uint8_t num, uint8_t layer, uint16_t val);

uint32_t grid_led_get_led_count(struct grid_led_model* mod);

/** ======================== SMART BUFFER  ========================== */




uint16_t grid_protocol_led_change_report_length(struct grid_led_model* mod);
uint16_t grid_protocol_led_change_report_generate(struct grid_led_model* mod, uint16_t maxlength, char* output);



#endif /* GRID_LED_H_INCLUDED */