/*
 * grid_cal.c
 *
 * Created: 25/11/2024 3:01:37 PM
 * Author : BENB
 */

#include "grid_cal.h"

struct grid_cal_model grid_cal_state;

uint8_t grid_cal_init(struct grid_cal_model* cal, uint8_t resolution, uint8_t length) {

  cal->resolution = resolution;
  cal->length = length;

  cal->maximum = 1 << cal->resolution;

  cal->value = malloc(cal->length * sizeof(uint16_t));
  cal->center = malloc(cal->length * sizeof(uint16_t));
  cal->enable = malloc(cal->length * sizeof(uint8_t));

  const uint16_t half_value = cal->maximum >> 1;

  const uint16_t default_offset = +32 * 4.5;

  for (uint8_t i = 0; i < length; ++i) {
    cal->value[i] = half_value + default_offset;
    cal->center[i] = cal->value[i];
    cal->enable[i] = 0;
  }

  return 0;
}

uint8_t grid_cal_enable_range(struct grid_cal_model* cal, uint8_t start, uint8_t length) {

  for (uint8_t i = start; i < length; ++i) {
    cal->enable[i] = 1;
  }

  return 0;
}

uint8_t grid_cal_center_set(struct grid_cal_model* cal, uint8_t channel, uint16_t center) {

  if (!cal->enable[channel]) {
    return 1;
  }

  cal->center[channel] = center;

  return 0;
}

uint16_t grid_cal_value_get(struct grid_cal_model* cal, uint8_t channel) {

  if (!cal->enable[channel]) {
    return 0;
  }

  return cal->value[channel];
}

uint16_t restrict_to_range(uint16_t x, uint16_t min, uint16_t max) {

  const uint16_t t = x < min ? min : x;

  return t > max ? max : t;
}

uint16_t quadratic_error_centering(uint16_t value, uint16_t center, uint16_t max) {

  const double offset = center / (double)max - 0.5;
  const double x = value / (double)max;
  const double tmp = 2.0 * x - 1.0;
  const double result = (x - (1.0 - tmp * tmp) * offset);

  return restrict_to_range(result * max, 0, max);
}

uint16_t grid_cal_next(struct grid_cal_model* cal, uint8_t channel, uint16_t value) {

  if (!cal->enable[channel]) {
    return value;
  }

  cal->value[channel] = value;

  return quadratic_error_centering(value, cal->center[channel], cal->maximum);
}
