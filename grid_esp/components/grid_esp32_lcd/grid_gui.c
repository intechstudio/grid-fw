#include "grid_gui.h"
struct grid_gui_model grid_gui_state;



int grid_gui_init(struct grid_gui_model* gui, void* screen_handle, uint8_t* framebuffer, uint32_t framebuffer_size, uint8_t bits_per_pixel, uint16_t width, uint16_t height) {
    grid_gui_state.screen_handle = screen_handle;
    grid_gui_state.framebuffer = framebuffer;
    grid_gui_state.framebuffer_size = framebuffer_size;
    grid_gui_state.bits_per_pixel = bits_per_pixel;
    grid_gui_state.width = width;
    grid_gui_state.height = height;
    return 0;
}

int grid_gui_draw_pixel(struct grid_gui_model* gui, uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b) {


    if (x >= gui->width || y >= gui->height){

        return 1; // out of bounds
    }


    uint8_t* pixel = gui->framebuffer + ((gui->width * y + x) * gui->bits_per_pixel / 8);
    pixel[0] = r;
    pixel[1] = g;
    pixel[2] = b;

    return 0;

}


void grid_gui_draw_demo(struct grid_gui_model* gui, uint8_t loopcounter) {

  

  for (int i = 0; i < 320; i++) {

    for (int j = 0; j < 240; j++) {

        grid_gui_draw_pixel(gui, i, j, loopcounter, 0, 0);
    }

  }

    

}