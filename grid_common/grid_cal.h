#pragma once

#ifndef GRID_CAL_H_INCLUDED
#define GRID_CAL_H_INCLUDED

// only for uint definitions
#include <stdint.h>
// only for malloc
#include <stdlib.h>

struct grid_cal_model {

  uint8_t resolution;
  uint8_t length;
  uint16_t maximum;
  uint16_t* value;
  uint16_t* center;
  uint8_t* enable;
};

extern struct grid_cal_model grid_cal_state;

uint8_t grid_cal_init(struct grid_cal_model* cal, uint8_t resolution, uint8_t length);
uint8_t grid_cal_enable_range(struct grid_cal_model* cal, uint8_t start, uint8_t length);
uint8_t grid_cal_center_set(struct grid_cal_model* cal, uint8_t channel, uint16_t center);
uint16_t grid_cal_value_get(struct grid_cal_model* cal, uint8_t channel);
uint16_t grid_cal_next(struct grid_cal_model* cal, uint8_t channel, uint16_t value);

#endif /* GRID_CAL_H_INCLUDED */