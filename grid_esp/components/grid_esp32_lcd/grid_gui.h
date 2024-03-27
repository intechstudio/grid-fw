
#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>


#ifdef __cplusplus
extern "C" {
#endif


struct grid_gui_model {

    void* screen_handle;
    uint8_t* framebuffer;
    uint32_t framebuffer_size;
    uint8_t bits_per_pixel;
    uint16_t width;
    uint16_t height;

};

extern struct grid_gui_model grid_gui_state;

int grid_gui_init(struct grid_gui_model* gui, void* screen_handle, uint8_t* framebuffer, uint32_t framebuffer_size, uint8_t bits_per_pixel, uint16_t width, uint16_t height);

int grid_gui_draw_pixel(struct grid_gui_model* gui, uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b);

void grid_gui_draw_demo(struct grid_gui_model* gui, uint8_t loopcounter);

#ifdef __cplusplus
}
#endif