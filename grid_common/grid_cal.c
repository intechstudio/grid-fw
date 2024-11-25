/*
 * grid_cal.c
 *
 * Created: 25/11/2024 3:01:37 PM
 * Author : BENB
 */

#include "grid_cal.h"

struct grid_cal_model grid_cal_state;

uint8_t grid_cal_init(struct grid_cal_model* cal, uint8_t length, uint8_t resolution) {

  cal->length = length;

  cal->value = malloc(cal->length * sizeof(uint16_t));
  cal->center = malloc(cal->length * sizeof(uint16_t));

  const uint16_t half_value = (1 << resolution) >> 1;

  for (uint8_t i = 0; i < length; ++i) {
    cal->value[i] = half_value;
    cal->center[i] = half_value;
  }

  return 0;
}
