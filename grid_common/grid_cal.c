/*
 * grid_cal.c
 *
 * Created: 25/11/2024 3:01:37 PM
 * Author : BENB
 */

#include "grid_cal.h"

struct grid_cal_model grid_cal_state;

int grid_cal_init(struct grid_cal_model* cal, uint8_t resolution, uint8_t length) {

  cal->resolution = resolution;
  cal->length = length;

  cal->maximum = 1 << cal->resolution;

  cal->value = (uint16_t*)malloc(cal->length * sizeof(uint16_t));
  cal->center = (uint16_t*)malloc(cal->length * sizeof(uint16_t));
  cal->enable = (uint16_t*)malloc(cal->length * sizeof(uint8_t));

  const uint16_t half_value = cal->maximum / 2;

  const uint16_t default_offset = +32 * 4.5;

  for (uint8_t i = 0; i < length; ++i) {
    cal->value[i] = half_value + default_offset;
    cal->center[i] = cal->value[i];
    cal->enable[i] = 0;
  }

  return 0;
}

int grid_cal_enable_range(struct grid_cal_model* cal, uint8_t start, uint8_t length) {

  if (!(start >= 0 && start < cal->length)) {
    return 1;
  }

  uint8_t end = start + length;

  if (!(end >= 0 && end < cal->length)) {
    return 1;
  }

  for (uint8_t i = start; i < end; ++i) {
    cal->enable[i] = 1;
  }

  return 0;
}

int grid_cal_center_set(struct grid_cal_model* cal, uint8_t channel, uint16_t center) {

  if (!(channel >= 0 && channel < cal->length)) {
    return 1;
  }

  if (!cal->enable[channel]) {
    return 1;
  }

  cal->center[channel] = center;

  return 0;
}

int grid_cal_value_get(struct grid_cal_model* cal, uint8_t channel, uint16_t* value) {

  if (!(channel >= 0 && channel < cal->length)) {
    return 1;
  }

  if (!cal->enable[channel]) {
    return 1;
  }

  *value = cal->value[channel];

  return 0;
}

int grid_cal_enable_get(struct grid_cal_model* cal, uint8_t channel, uint8_t* enable) {

  if (!(channel >= 0 && channel < cal->length)) {
    return 1;
  }

  *enable = cal->enable[channel];

  return 0;
}

static uint16_t restrict_to_range(uint16_t x, uint16_t min, uint16_t max) {

  const uint16_t t = x < min ? min : x;

  return t > max ? max : t;
}

static uint16_t quadratic_error_centering(uint16_t value, uint16_t center, uint16_t max) {

  const double offset = center / (double)max - 0.5;
  const double x = value / (double)max;
  const double tmp = 2.0 * x - 1.0;
  const double result = (x - (1.0 - tmp * tmp) * offset);

  return restrict_to_range(result * max, 0, max);
}

int grid_cal_next(struct grid_cal_model* cal, uint8_t channel, uint16_t in, uint16_t* out) {

  if (!(channel >= 0 && channel < cal->length)) {
    return 1;
  }

  if (!cal->enable[channel]) {
    return 1;
  }

  cal->value[channel] = in;

  *out = quadratic_error_centering(in, cal->center[channel], cal->maximum);

  return 0;
}
