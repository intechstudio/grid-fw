#pragma once

#ifndef GRID_AIN_H_INCLUDED
#define GRID_AIN_H_INCLUDED

// only for uint definitions
#include <stdint.h>
// only for malloc
#include <stdlib.h>

#include <stdio.h>

struct AIN_Channel {

  uint16_t *buffer;
  uint8_t buffer_depth;

  uint8_t result_format;
  uint8_t result_resolution;
  uint16_t result_value;
  uint16_t result_average;
  uint16_t result_changed;
};

struct grid_ain_model {
  uint8_t initialized;
  struct AIN_Channel *channel_buffer;
  uint8_t channel_buffer_length;
};

extern struct grid_ain_model grid_ain_state;

uint8_t grid_ain_channel_init(struct grid_ain_model *mod, uint8_t channel,
                              uint8_t buffer_depth);

uint8_t grid_ain_channel_deinit(struct grid_ain_model *mod, uint8_t channel);

/** Initialize ain buffer for a given number of analog channels */
uint8_t grid_ain_init(struct grid_ain_model *mod, uint8_t length,
                      uint8_t depth);
uint8_t grid_ain_add_sample(struct grid_ain_model *mod, uint8_t channel,
                            uint16_t value, uint8_t source_resolution,
                            uint8_t result_resolution);

uint8_t grid_ain_get_changed(struct grid_ain_model *mod, uint8_t channel);
uint16_t grid_ain_get_average(struct grid_ain_model *mod, uint8_t channel);

int32_t grid_ain_get_average_scaled(struct grid_ain_model *mod, uint8_t channel,
                                    uint8_t source_resolution,
                                    uint8_t result_resolution, int32_t min,
                                    int32_t max);

uint32_t grid_ain_abs(int32_t value);

#endif /* GRID_AIN_H_INCLUDED */
