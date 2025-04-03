/*
 * grid_ain.c
 *
 * Created: 4/12/2019 5:27:13 PM
 * Author : SUKU WC
 */

#include "grid_ain.h"

void* grid_platform_allocate_volatile(size_t size);

struct grid_ain_model grid_ain_state;

uint32_t grid_ain_abs(int32_t value) {

  if (value > 0) {
    return value;
  } else {
    return -value;
  }
}

uint8_t grid_ain_channel_init(struct grid_ain_model* ain, uint8_t channel, uint8_t buffer_depth) {

  ain->channel_buffer[channel].buffer_depth = buffer_depth;

  ain->channel_buffer[channel].buffer_samples = 0;

  ain->channel_buffer[channel].result_average = 0;

  ain->channel_buffer[channel].buffer = grid_platform_allocate_volatile(ain->channel_buffer[channel].buffer_depth * sizeof(uint16_t));

  // Init the whole buffer with zeros
  for (uint8_t i = 0; i < ain->channel_buffer[channel].buffer_depth; i++) {
    ain->channel_buffer[channel].buffer[i] = 0;
  }

  ain->channel_buffer[channel].result_changed = 0;
  ain->channel_buffer[channel].result_value = 0;

  return 0;
}

uint8_t grid_ain_channel_deinit(struct grid_ain_model* ain, uint8_t channel) {

  while (1) {
    // TRAP
  }
}

/** Initialize ain buffer for a given number of analog channels */
uint8_t grid_ain_init(struct grid_ain_model* ain, uint8_t length, uint8_t depth) {

  // 2D buffer, example: 16 potentiometers, last 32 samples stored for each
  ain->channel_buffer = grid_platform_allocate_volatile(length * sizeof(struct AIN_Channel));
  ain->channel_buffer_length = length;

  for (uint8_t i = 0; i < length; i++) {
    grid_ain_channel_init(ain, i, depth);
  }

  return 0;
}

uint8_t grid_ain_add_sample(struct grid_ain_model* ain, uint8_t channel, uint16_t value, uint8_t source_resolution, uint8_t result_resolution) {

  struct AIN_Channel* instance = &ain->channel_buffer[channel];

  instance->buffer_samples += (instance->buffer_samples < instance->buffer_depth * 2);

  uint32_t sum = 0;
  uint16_t minimum = -1; // -1 trick to get the largest possible number
  uint16_t maximum = 0;

  uint8_t minimum_index = 0;
  uint8_t maximum_index = 0;

  for (uint8_t i = 0; i < instance->buffer_depth; i++) {

    uint16_t current = instance->buffer[i];

    sum += current;

    if (current > maximum) {
      maximum = current;
      maximum_index = i;
    }

    if (current < minimum) {
      minimum = current;
      minimum_index = i;
    }
  }

  uint16_t average = sum / instance->buffer_depth;

  if (value > average) {
    // Replace minimum in the buffer and recalculate sum
    sum = sum - instance->buffer[minimum_index] + value;
    instance->buffer[minimum_index] = value;
  } else {
    // Replace maximum in the buffer and recalculate sum
    sum = sum - instance->buffer[maximum_index] + value;
    instance->buffer[maximum_index] = value;
  }

  // Recalculate average
  average = sum / instance->buffer_depth;

  // up until here all looks good, everything is 16 bit

  uint8_t downscale_factor = (source_resolution - result_resolution);
  uint8_t upscale_factor = (source_resolution - result_resolution);

  uint16_t downsampled = (average >> downscale_factor);
  uint16_t upscaled = downsampled << upscale_factor;

  uint8_t criteria_a = instance->result_value != upscaled;

  uint8_t criteria_b = grid_ain_abs(instance->result_average - average) > (1 << downscale_factor);
  uint8_t criteria_c = upscaled > ((1 << source_resolution) - (1 << upscale_factor) - 1);
  uint8_t criteria_d = upscaled == 0;

  if (criteria_a && (criteria_b || criteria_c || criteria_d)) {

    // printf("%d: %d %d\r\n", channel, average, upscaled);
    instance->result_average = average;
    instance->result_value = upscaled;
    instance->result_changed = 1;
    return 1;
  } else {
    return 0;
  }
}

int grid_ain_stabilized(struct grid_ain_model* ain, uint8_t channel) {

  struct AIN_Channel* instance = &ain->channel_buffer[channel];

  return instance->buffer_samples >= instance->buffer_depth * 2;
}

uint8_t grid_ain_get_changed(struct grid_ain_model* ain, uint8_t channel) {

  struct AIN_Channel* instance = &ain->channel_buffer[channel];

  return instance->result_changed != 0;
}

uint16_t grid_ain_get_average(struct grid_ain_model* ain, uint8_t channel) {

  struct AIN_Channel* instance = &ain->channel_buffer[channel];

  instance->result_changed = 0;

  return instance->result_value;
}

static double lerp(double a, double b, double x) { return a * (1.0 - x) + (b * x); }

int32_t grid_ain_get_average_scaled(struct grid_ain_model* ain, uint8_t channel, uint8_t source_resolution, uint8_t result_resolution, int32_t min, int32_t max) {

  struct AIN_Channel* instance = &ain->channel_buffer[channel];

  instance->result_changed = 0;

  int bits_diff = source_resolution - result_resolution;

  uint16_t value = instance->result_value >> bits_diff;

  int32_t next = lerp(min, max + 1, value / (double)(1 << result_resolution));

  if (next > max) {
    next = max;
  }

  return next;
}
