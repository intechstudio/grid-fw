/*
 * grid_cal.c
 *
 * Created: 25/11/2024 3:01:37 PM
 * Author : BENB
 */

#include "grid_cal.h"

#include <stdlib.h>

struct grid_cal_model grid_cal_state = {0};

int grid_cal_pot_init(struct grid_cal_pot* cal, uint8_t resolution, uint8_t length) {

  cal->resolution = resolution;
  cal->length = length;

  cal->maximum = 1 << cal->resolution;

  cal->value = (uint16_t*)malloc(cal->length * sizeof(uint16_t));
  cal->center = (uint16_t*)malloc(cal->length * sizeof(uint16_t));
  cal->detentlo = (uint16_t*)malloc(cal->length * sizeof(uint16_t));
  cal->detenthi = (uint16_t*)malloc(cal->length * sizeof(uint16_t));
  cal->enable = (uint8_t*)malloc(cal->length * sizeof(uint8_t));

  const uint16_t half_value = cal->maximum / 2;
  const uint16_t default_offset = +32 * 4.5;

  for (uint8_t i = 0; i < cal->length; ++i) {
    cal->value[i] = half_value + default_offset;
    cal->center[i] = cal->value[i];
    cal->detentlo[i] = cal->maximum;
    cal->detenthi[i] = 0;
    cal->enable[i] = 0;
  }

  return 0;
}

int grid_cal_pot_enable_range(struct grid_cal_pot* cal, uint8_t start, uint8_t length) {

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

int grid_cal_pot_enable_get(struct grid_cal_pot* cal, uint8_t channel, uint8_t* enable) {

  if (!(channel < cal->length)) {
    return 1;
  }

  *enable = cal->enable[channel];

  return 0;
}

int grid_cal_pot_center_get(struct grid_cal_pot* cal, uint8_t channel, uint16_t* center) {

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

int grid_cal_pot_center_set(struct grid_cal_pot* cal, uint8_t channel, uint16_t center) {

  if (!(channel < cal->length)) {
    return 1;
  }

  if (!cal->enable[channel]) {
    return 1;
  }

  cal->center[channel] = center;

  return 0;
}

int grid_cal_pot_detent_get(struct grid_cal_pot* cal, uint8_t channel, uint16_t* detent, bool high) {

  if (!(channel < cal->length)) {
    return 1;
  }

  if (cal->enable[channel]) {

    uint16_t* target = high ? cal->detenthi : cal->detentlo;
    *detent = target[channel];

  } else {

    *detent = 0;
  }

  return 0;
}

int grid_cal_pot_detent_set(struct grid_cal_pot* cal, uint8_t channel, uint16_t detent, bool high) {

  if (!(channel < cal->length)) {
    return 1;
  }

  if (!cal->enable[channel]) {
    return 1;
  }

  uint16_t* target = high ? cal->detenthi : cal->detentlo;

  target[channel] = detent;

  return 0;
}

int grid_cal_pot_value_get(struct grid_cal_pot* cal, uint8_t channel, uint16_t* value) {

  if (!(channel < cal->length)) {
    return 1;
  }

  if (!cal->enable[channel]) {
    return 1;
  }

  *value = cal->value[channel];

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

static int32_t detent_center_deadzoning(int32_t a, int32_t b, double x, double lo, double hi) {

  int32_t half_value = (a + b) / 2;

  if (x < lo) {

    return lerp(a, half_value, x / lo);
  }

  if (x > hi) {

    // expand and simplify: lerp(half_value, b, (x - hi) / (1 - hi))
    return (half_value * (x - 1) + b * (hi - x)) / (hi - 1);
  }

  return half_value;
}

int grid_cal_pot_next(struct grid_cal_pot* cal, uint8_t channel, uint16_t in, uint16_t* out) {

  if (!(channel < cal->length)) {
    return 1;
  }

  if (!cal->enable[channel]) {
    *out = in;
    return 0;
  }

  cal->value[channel] = in;

  // If the detent interval is valid, use detent calibration
  if (cal->detentlo[channel] < cal->detenthi[channel]) {

    double in_norm = in / (double)cal->maximum;
    double lo_norm = cal->detentlo[channel] / (double)cal->maximum;
    double hi_norm = cal->detenthi[channel] / (double)cal->maximum;
    *out = detent_center_deadzoning(0, cal->maximum, in_norm, lo_norm, hi_norm);

  }
  // Otherwise, use centering calibration
  else {

    double in_norm = in / (double)cal->maximum;
    double center_norm = cal->center[channel] / (double)cal->maximum;
    *out = inverse_error_centering(0, cal->maximum, in_norm, center_norm, 2);
  }

  return 0;
}

int grid_cal_but_init(struct grid_cal_but* cal, uint8_t length) {

  cal->length = length;

  cal->enable = (uint8_t*)malloc(cal->length * sizeof(uint8_t));
  cal->states = (struct grid_ui_button_state**)malloc(cal->length * sizeof(struct grid_ui_button_state*));

  for (uint8_t i = 0; i < cal->length; ++i) {
    cal->enable[i] = 0;
    cal->states[i] = NULL;
  }

  return 0;
}

int grid_cal_but_enable_get(struct grid_cal_but* cal, uint8_t channel, uint8_t* enable) {

  if (!(channel < cal->length)) {
    return 1;
  }

  *enable = cal->enable[channel];

  return 0;
}

int grid_cal_but_enable_set(struct grid_cal_but* cal, uint8_t channel, struct grid_ui_button_state* state) {

  if (!(channel < cal->length)) {
    return 1;
  }

  cal->enable[channel] = 1;
  cal->states[channel] = state;

  return 0;
}

int grid_cal_but_minmax_get(struct grid_cal_but* cal, uint8_t channel, uint16_t* min, uint16_t* max) {

  if (!(channel < cal->length)) {
    return 1;
  }

  if (cal->enable[channel]) {

    *min = grid_ui_button_state_get_min(cal->states[channel]);
    *max = grid_ui_button_state_get_max(cal->states[channel]);
  } else {

    *min = 0;
    *max = 0;
  }

  return 0;
}

int grid_cal_but_min_set(struct grid_cal_but* cal, uint8_t channel, uint16_t min) {

  if (!(channel < cal->length)) {
    return 1;
  }

  if (!cal->enable[channel]) {
    return 1;
  }

  grid_ui_button_state_value_update(cal->states[channel], min, 0);

  return 0;
}

int grid_cal_but_max_set(struct grid_cal_but* cal, uint8_t channel, uint16_t max) {

  if (!(channel < cal->length)) {
    return 1;
  }

  if (!cal->enable[channel]) {
    return 1;
  }

  grid_ui_button_state_value_update(cal->states[channel], max, 0);

  return 0;
}

struct grid_ui_button_state* grid_cal_but_state_get(struct grid_cal_but* cal, uint8_t channel) {

  if (!(channel < cal->length)) {
    return NULL;
  }

  if (!cal->enable[channel]) {
    return NULL;
  }

  assert(cal->states[channel]);

  return cal->states[channel];
}
