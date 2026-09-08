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

#endif /* GRID_IMAGE_H */
