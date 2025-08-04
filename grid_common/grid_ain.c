/*
 * grid_ain.c
 *
 * Created: 4/12/2019 5:27:13 PM
 * Author : SUKU WC
 */

#include "grid_ain.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "grid_platform.h"

struct grid_ain_model grid_ain_state;

void grid_ain_channel_reset(struct ain_chan_t* chan) { chan->size = chan->beg = chan->end = chan->sum = chan->output = chan->delta = 0; }

void grid_ain_channel_init(struct ain_chan_t* chan, uint8_t capacity) {

  assert(capacity > 0);
  chan->capa = capacity;

  chan->data = grid_platform_allocate_volatile(chan->capa * sizeof(uint16_t));
  assert(chan->data);
  memset(chan->data, 0, chan->capa * sizeof(uint16_t));

  grid_ain_channel_reset(chan);
}

void grid_ain_init(struct grid_ain_model* ain, uint8_t channel_count, uint8_t capacity) {

  assert(channel_count > 0);
  ain->channel_count = channel_count;

  ain->channels = grid_platform_allocate_volatile(ain->channel_count * sizeof(struct ain_chan_t));
  assert(ain->channels);

  for (uint8_t i = 0; i < ain->channel_count; ++i) {
    grid_ain_channel_init(&ain->channels[i], capacity);
  }
}

void grid_ain_add_sample(struct grid_ain_model* ain, uint8_t channel, uint16_t value, uint8_t src_res, uint8_t dst_res) {

  assert(channel < ain->channel_count);
  struct ain_chan_t* chan = &ain->channels[channel];

  // Take to-be-overwritten value out of sum if there is one
  chan->sum -= chan->data[chan->beg] * (chan->size == chan->capa);

  // Write value into the end position of the circular buffer
  chan->data[chan->end] = value;

  // Update indices of the circular buffer
  chan->end = (chan->end + 1) % chan->capa;
  chan->beg = (chan->beg + (chan->size == chan->capa)) % chan->capa;
  chan->size += chan->size != chan->capa;

  // Add new value to sum
  chan->sum += value;

  assert(src_res >= dst_res);
  uint8_t diff_res = src_res - dst_res;

  // Quantize the average value
  uint16_t average = chan->sum / chan->capa;
  uint16_t quantized = (average >> diff_res) << diff_res;

  // Figure out if the quantized value and old output value differ meaningfully
  uint8_t delta;
  switch (diff_res) {
  case 0: {

    // If the resolutions match, any difference is a delta
    delta = quantized != chan->output;

  } break;
  default: {

    // Difference at the destination resolution
    uint16_t diff_quant = (quantized - chan->output) >> diff_res;

    // The most significant bit lost at the destination resolution
    uint8_t msb_lost = (average >> (diff_res - 1)) & 0x1;

    // Hysteresis logic against flicker
    switch (diff_quant) {
    case 1:
      delta = msb_lost == 1;
      break;
    case UINT16_MAX:
      delta = msb_lost == 0;
      break;
    default:
      delta = diff_quant != 0;
    }
  }
  }

  // Update output value and delta if necessary
  if (delta) {

    chan->output = quantized;
    chan->delta = delta;
  }
}

bool grid_ain_stabilized(struct grid_ain_model* ain, uint8_t channel) {

  assert(channel < ain->channel_count);
  struct ain_chan_t* chan = &ain->channels[channel];

  return chan->size == chan->capa;
}

bool grid_ain_get_changed(struct grid_ain_model* ain, uint8_t channel) {

  assert(channel < ain->channel_count);
  struct ain_chan_t* chan = &ain->channels[channel];

  return chan->delta;
}

static double lerp(double a, double b, double x) { return a * (1.0 - x) + (b * x); }

int32_t grid_ain_get_average_scaled(struct grid_ain_model* ain, uint8_t channel, uint8_t src_res, uint8_t dst_res, int32_t min, int32_t max) {

  assert(channel < ain->channel_count);
  struct ain_chan_t* chan = &ain->channels[channel];

  chan->delta = 0;

  assert(src_res >= dst_res);
  int bits_diff = src_res - dst_res;

  uint16_t value = chan->output >> bits_diff;

  int32_t next = lerp(min, max + 1, value / (double)(1 << dst_res));

  if (next > max) {
    next = max;
  }

  return next;
}
