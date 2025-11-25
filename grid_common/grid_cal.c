/*
 * grid_cal.c
 *
 * Created: 25/11/2024 3:01:37 PM
 * Author : BENB
 */

#include "grid_cal.h"

#include <stdlib.h>

struct grid_cal_model grid_cal_state = {0};

void grid_cal_limits_init(struct grid_cal_limits* limits, uint16_t deadzone, uint8_t resolution) {

  limits->deadzone = deadzone;
  grid_cal_limits_reset(limits, resolution);
}

void grid_cal_limits_reset(struct grid_cal_limits* limits, uint8_t resolution) {

  if (limits->deadzone) {
    limits->min = limits->deadzone;
    limits->max = (1 << resolution) - limits->deadzone;
  } else {
    limits->min = UINT16_MAX;
    limits->max = 0;
  }
}

bool grid_cal_limits_range_valid(struct grid_cal_limits* limits) { return limits->min < limits->max; }

void grid_cal_limits_value_update(struct grid_cal_limits* limits, uint16_t value) {

  if (value < limits->min) {
    limits->min = value;
  }

  if (value > limits->max) {
    limits->max = value;
  }
}

uint16_t grid_cal_limits_min_get(struct grid_cal_limits* limits) { return limits ? limits->min : 0; }

uint16_t grid_cal_limits_max_get(struct grid_cal_limits* limits) { return limits ? limits->max : 0; }

void grid_cal_center_init(struct grid_cal_center* center, uint16_t initial) {

  center->initial = initial;
  grid_cal_center_reset(center);
}

void grid_cal_center_reset(struct grid_cal_center* center) { center->center = center->initial; }

void grid_cal_center_value_update(struct grid_cal_center* center, uint16_t value) { center->value = value; }

uint16_t grid_cal_center_value_get(struct grid_cal_center* center) { return center ? center->value : 0; }

uint16_t grid_cal_center_center_get(struct grid_cal_center* center) { return center ? center->center : 0; }

void grid_cal_detent_init(struct grid_cal_detent* detent) { grid_cal_detent_reset(detent); }

void grid_cal_detent_reset(struct grid_cal_detent* detent) {

  detent->value = 0;
  detent->lo = UINT16_MAX;
  detent->hi = 0;
}

void grid_cal_detent_value_update(struct grid_cal_detent* detent, uint16_t value) { detent->value = value; }

uint16_t grid_cal_detent_lo_get(struct grid_cal_detent* detent) { return detent ? detent->lo : 0; }

uint16_t grid_cal_detent_hi_get(struct grid_cal_detent* detent) { return detent ? detent->hi : 0; }

int grid_cal_init(struct grid_cal_model* cal, uint8_t length, uint8_t resolution) {

  assert(length);

  cal->length = length;
  cal->resolution = resolution;
  cal->limits = (struct grid_cal_limits**)malloc(cal->length * sizeof(struct grid_cal_limits*));
  cal->center = (struct grid_cal_center**)malloc(cal->length * sizeof(struct grid_cal_center*));
  cal->detent = (struct grid_cal_detent**)malloc(cal->length * sizeof(struct grid_cal_detent*));
  cal->sigcond = (struct grid_asc*)malloc(cal->length * sizeof(struct grid_asc));

  for (uint8_t i = 0; i < cal->length; ++i) {
    cal->limits[i] = NULL;
    cal->center[i] = NULL;
    cal->detent[i] = NULL;
  }

  memset(cal->sigcond, 0, cal->length * sizeof(struct grid_asc));

  grid_asc_array_set_factors(cal->sigcond, cal->length, 0, cal->length, 64);

  return 0;
}

void grid_cal_reset(struct grid_cal_model* cal) {

  for (uint8_t i = 0; i < cal->length; ++i) {

    if (cal->limits[i]) {
      grid_cal_limits_reset(cal->limits[i], cal->resolution);
    }

    if (cal->center[i]) {
      grid_cal_center_reset(cal->center[i]);
    }

    if (cal->detent[i]) {
      grid_cal_detent_reset(cal->detent[i]);
    }
  }
}

int grid_cal_set(struct grid_cal_model* cal, uint8_t channel, enum grid_cal_type type, void* src) {

  if (!(channel < cal->length)) {
    return 1;
  }

  switch (type) {
  case GRID_CAL_LIMITS:
    cal->limits[channel] = src;
    break;
  case GRID_CAL_CENTER:
    cal->center[channel] = src;
    break;
  case GRID_CAL_DETENT:
    cal->detent[channel] = src;
    break;
  default:
    assert(0);
    break;
  }

  return 0;
}

int grid_cal_get(struct grid_cal_model* cal, uint8_t channel, enum grid_cal_type type, void** dest) {

  if (!(channel < cal->length)) {
    return 1;
  }

  switch (type) {
  case GRID_CAL_LIMITS: {
    *(struct grid_cal_limits**)dest = cal->limits[channel];
  } break;
  case GRID_CAL_CENTER: {
    *(struct grid_cal_center**)dest = cal->center[channel];
  } break;
  case GRID_CAL_DETENT: {
    *(struct grid_cal_detent**)dest = cal->detent[channel];
  } break;
  default: {
    assert(0);
  } break;
  }

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

static uint16_t detent_center_deadzoning(struct grid_cal_limits* lim, struct grid_cal_detent* det, uint16_t x, uint16_t resolution) {

  uint16_t half_value = 1 << (resolution - 1);

  if (x < det->lo) {
    return ((x - lim->min) << (resolution - 1)) / (det->lo - lim->min + 1);
  }

  if (x > det->hi) {
    return ((x - det->hi) << (resolution - 1)) / (lim->max - det->hi + 1) + half_value;
  }

  return half_value;
}

uint16_t grid_cal_next(struct grid_cal_model* cal, uint8_t channel, uint16_t in) {

  assert(channel < cal->length);

  struct grid_cal_limits* lim = cal->limits[channel];
  struct grid_cal_center* ctr = cal->center[channel];
  struct grid_cal_detent* det = cal->detent[channel];

  if (!lim && !ctr && !det) {
    return in;
  }

  if (lim && !ctr && !det) {

    in = clampu16(in, lim->min, lim->max);

    return ((in - lim->min) << cal->resolution) / (lim->max - lim->min + 1);

  } else if (lim && ctr) {

    in = clampu16(in, lim->min, lim->max);

    if (det && det->lo < det->hi) {

      return detent_center_deadzoning(lim, det, in, cal->resolution);

    } else {

      double in_norm = (in - lim->min) / (double)(lim->max - lim->min);
      double center_norm = (ctr->center - lim->min) / (double)(lim->max - lim->min);
      uint32_t maximum = (1 << cal->resolution) - 1;
      return inverse_error_centering(0, maximum, in_norm, center_norm, 2);
    }
  }

  assert(0);
}
