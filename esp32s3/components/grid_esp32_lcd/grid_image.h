#ifndef GRID_IMAGE_H
#define GRID_IMAGE_H

#include <stdint.h>
#include <stdlib.h>

#include "grid_gui.h"

enum grid_image_id_t {
  GRID_IMAGE_TEST_BMP = 0,
  GRID_IMAGE_TEST_PNG,
  GRID_IMAGE_TEST_JPG,
  GRID_IMAGE_COUNT,
};

int grid_image_draw(struct grid_gui_model* gui, uint8_t image_id, uint16_t x, uint16_t y);

// Decodes a BMP/PNG/JPEG byte buffer into a flat RGB888 pixel buffer (w*h*3
// bytes), owned by the caller and released via grid_image_free_pixels.
// Returns NULL on failure; grid_image_last_error() gives the reason (which
// can legitimately be empty - not every stb_image failure path sets one).
unsigned char* grid_image_decode(const unsigned char* data, int size, int* w, int* h);

void grid_image_free_pixels(unsigned char* pixels);

const char* grid_image_last_error(void);

// Blits an already-decoded RGB888 pixel buffer (as produced by
// grid_image_decode) into gui->buffer, clipped to the destination bounds.
int grid_image_blit_pixels(struct grid_gui_model* gui, const unsigned char* pixels, int w, int h, uint16_t x, uint16_t y);

#endif /* GRID_IMAGE_H */
