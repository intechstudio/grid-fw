#pragma once

#ifndef GRID_CAL_H_INCLUDED
#define GRID_CAL_H_INCLUDED

// only for uint definitions
#include <stdint.h>
// only for malloc
#include <stdlib.h>

struct grid_cal_model {

  uint8_t length;
  uint16_t* value;
  uint16_t* center;
};

extern struct grid_cal_model grid_cal_state;

uint8_t grid_cal_init(struct grid_cal_model* cal, uint8_t length, uint8_t resolution);

#endif /* GRID_CAL_H_INCLUDED */
