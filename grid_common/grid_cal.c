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
  cal->enable = (uint8_t*)malloc(cal->length * sizeof(uint8_t));

  const uint16_t half_value = cal->maximum / 2;
  const uint16_t default_offset = +32 * 4.5;

  for (uint8_t i = 0; i < cal->length; ++i) {
    cal->value[i] = half_value + default_offset;
    cal->center[i] = cal->value[i];
    cal->enable[i] = 0;
  }

  return 0;
}

int grid_cal_enable_range(struct grid_cal_model* cal, uint8_t start, uint8_t length) {

  if (!(start < cal->length)) {
    return 1;
  }

  uint8_t end = start + length;

  if (!(end < cal->length)) {
    return 1;
  }

  for (uint8_t i = start; i < end; ++i) {
    cal->enable[i] = 1;
  }

  return 0;
}

int grid_cal_center_get(struct grid_cal_model* cal, uint8_t channel, uint16_t* center) {

  if (!(channel < cal->length)) {
    return 1;
  }

  if (cal->enable[channel]) {
    *center = cal->center[channel];
  } else {
    *center = 0;
  }

  return 0;
}

int grid_cal_center_set(struct grid_cal_model* cal, uint8_t channel, uint16_t center) {

  if (!(channel < cal->length)) {
    return 1;
  }

  if (!cal->enable[channel]) {
    return 1;
  }

  cal->center[channel] = center;

  return 0;
}

int grid_cal_value_get(struct grid_cal_model* cal, uint8_t channel, uint16_t* value) {

  if (!(channel < cal->length)) {
    return 1;
  }

  if (!cal->enable[channel]) {
    return 1;
  }

  *value = cal->value[channel];

  return 0;
}

int grid_cal_enable_get(struct grid_cal_model* cal, uint8_t channel, uint8_t* enable) {

  if (!(channel < cal->length)) {
    return 1;
  }

  *enable = cal->enable[channel];

  return 0;
}

static double lerp(double a, double b, double x) { return a * (1.0 - x) + (b * x); }

static int32_t inverse_error_centering(int32_t a, int32_t b, double x, double c, uint8_t iter) {

  for (uint8_t i = 0; i < iter; ++i) {

    if (x < c) {
      b = (a + b) / 2;
      x = x / c;
    } else {
      a = (a + b) / 2;
      x = (x - c) / (1.0 - c);
    }
  }

  return lerp(a, b, x);
}

int grid_cal_next(struct grid_cal_model* cal, uint8_t channel, uint16_t in, uint16_t* out) {

  if (!(channel < cal->length)) {
    return 1;
  }

  if (!cal->enable[channel]) {
    *out = in;
    return 0;
  }

  cal->value[channel] = in;

  double in_norm = in / (double)cal->maximum;
  double center_norm = cal->center[channel] / (double)cal->maximum;
  *out = inverse_error_centering(0, cal->maximum, in_norm, center_norm, 2);

  return 0;
}
