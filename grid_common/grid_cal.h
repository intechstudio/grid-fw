#ifndef GRID_CAL_H
#define GRID_CAL_H

#include <stdbool.h>
#include <stdint.h>

#include "grid_asc.h"
#include "grid_math.h"

enum grid_cal_type {
  GRID_CAL_LIMITS = 0,
  GRID_CAL_CENTER,
  GRID_CAL_DETENT,
  GRID_CAL_COUNT,
};

struct grid_cal_limits {
  uint16_t min;
  uint16_t max;
  uint16_t deadzone;
};

void grid_cal_limits_init(struct grid_cal_limits* limits, uint16_t deadzone, uint8_t resolution);
void grid_cal_limits_reset(struct grid_cal_limits* limits, uint8_t resolution);
bool grid_cal_limits_range_valid(struct grid_cal_limits* limits);
void grid_cal_limits_value_update(struct grid_cal_limits* limits, uint16_t value);
uint16_t grid_cal_limits_min_get(struct grid_cal_limits* limits);
uint16_t grid_cal_limits_max_get(struct grid_cal_limits* limits);

struct grid_cal_center {
  uint16_t value;
  uint16_t center;
  uint16_t initial;
};

void grid_cal_center_init(struct grid_cal_center* center, uint16_t initial);
void grid_cal_center_reset(struct grid_cal_center* center);
void grid_cal_center_value_update(struct grid_cal_center* center, uint16_t value);
uint16_t grid_cal_center_value_get(struct grid_cal_center* center);
uint16_t grid_cal_center_center_get(struct grid_cal_center* center);

struct grid_cal_detent {
  uint16_t value;
  uint16_t lo;
  uint16_t hi;
};

void grid_cal_detent_init(struct grid_cal_detent* detent);
void grid_cal_detent_reset(struct grid_cal_detent* detent);
void grid_cal_detent_value_update(struct grid_cal_detent* detent, uint16_t value);
uint16_t grid_cal_detent_lo_get(struct grid_cal_detent* detent);
uint16_t grid_cal_detent_hi_get(struct grid_cal_detent* detent);

struct grid_cal_model {

  uint8_t length;
  uint8_t resolution;
  struct grid_cal_limits** limits;
  struct grid_cal_center** center;
  struct grid_cal_detent** detent;
  struct grid_asc* sigcond;
};

int grid_cal_init(struct grid_cal_model* cal, uint8_t length, uint8_t resolution);
void grid_cal_reset(struct grid_cal_model* cal);
int grid_cal_set(struct grid_cal_model* cal, uint8_t channel, enum grid_cal_type type, void* src);
int grid_cal_get(struct grid_cal_model* cal, uint8_t channel, enum grid_cal_type type, void** dest);
uint16_t grid_cal_next(struct grid_cal_model* cal, uint8_t channel, uint16_t in);

extern struct grid_cal_model grid_cal_state;

#endif /* GRID_CAL_H */
