#ifndef GRID_CAL_H
#define GRID_CAL_H

#include <stdint.h>

struct grid_cal_model {

  uint8_t resolution;
  uint8_t length;
  uint16_t maximum;
  uint16_t* value;
  uint16_t* center;
  uint8_t* enable;
};

extern struct grid_cal_model grid_cal_state;

int grid_cal_init(struct grid_cal_model* cal, uint8_t resolution, uint8_t length);
int grid_cal_enable_range(struct grid_cal_model* cal, uint8_t start, uint8_t length);
int grid_cal_center_get(struct grid_cal_model* cal, uint8_t channel, uint16_t* center);
int grid_cal_center_set(struct grid_cal_model* cal, uint8_t channel, uint16_t center);
int grid_cal_value_get(struct grid_cal_model* cal, uint8_t channel, uint16_t* value);
int grid_cal_enable_get(struct grid_cal_model* cal, uint8_t channel, uint8_t* enable);
int grid_cal_next(struct grid_cal_model* cal, uint8_t channel, uint16_t in, uint16_t* out);

#endif /* GRID_CAL_H */
