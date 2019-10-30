#ifndef GRID_LED_H_INCLUDED
#define GRID_LED_H_INCLUDED

#include "grid_module.h"

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
	
	uint8_t top; //TOP LEVEL
	uint8_t bot; //BOTTOM LEVEL
	
	uint8_t pha; //PHASE
	uint8_t fre; //FREQUENCY
	
};


struct grid_led_model{

	uint8_t status;
	
	uint8_t led_number;
	uint32_t led_frame_buffer_size;
	uint8_t* led_frame_buffer; // The frame buffer is used to send data to the LEDs
	uint32_t* led_frame_buffer_usable; // The part of the frame buffer that contains LED data


	struct LED_layer* led_smart_buffer; // 2D array if LED_Layers: Smart buffer contains the usable data coming from the API

	
	struct io_descriptor* hardware_io_descriptor;

};

struct grid_led_model grid_led_state;

uint32_t grid_led_color_code[256];
volatile uint8_t grid_led_hardware_transfer_done;

static void grid_led_hardware_transfer_complete_cb(struct _dma_resource *resource);

void grid_led_hardware_start_transfer_blocking(struct grid_led_model* mod);

void grid_led_hardware_start_transfer(struct grid_led_model* mod);

uint8_t grid_led_hardware_is_transfer_completed(struct grid_led_model* mod);


// INITIALIZING THE GRID_LED LIBRARY
uint8_t grid_led_init(struct grid_led_model* mod, uint8_t num);

// ACCESSING THE FRAME BUFFER (READ)
uint8_t* grid_led_get_frame_buffer_pointer(struct grid_led_model* mod);
uint32_t grid_led_get_frame_buffer_size(struct grid_led_model* mod);

// RENDERING FROM SMART BUFFER TO FRAME BUFFER
void grid_led_render(struct grid_led_model* mod, uint32_t num);
void grid_led_render_all(struct grid_led_model* mod);

// ACCESSING THE FRAME BUFFER (WRITE)
uint8_t grid_led_set_color(struct grid_led_model* mod, uint32_t led_index, uint8_t led_r, uint8_t led_g, uint8_t led_b);

// TIME TICK FOR ANIMATIONS
void grid_led_tick(struct grid_led_model* mod);


// WRITING THE SMART BUFFER
void grid_led_set_min(struct grid_led_model* mod, uint8_t num, uint8_t layer, uint8_t r, uint8_t g, uint8_t b);
void grid_led_set_mid(struct grid_led_model* mod, uint8_t num, uint8_t layer, uint8_t r, uint8_t g, uint8_t b);
void grid_led_set_max(struct grid_led_model* mod, uint8_t num, uint8_t layer, uint8_t r, uint8_t g, uint8_t b);
void grid_led_set_phase(struct grid_led_model* mod, uint8_t num, uint8_t layer, uint8_t val);
void grid_led_set_frequency(struct grid_led_model* mod, uint8_t num, uint8_t layer, uint8_t val);

uint32_t grid_led_get_led_number(struct grid_led_model* mod);

/** ======================== SMART BUFFER  ========================== */







#endif /* GRID_LED_H_INCLUDED */