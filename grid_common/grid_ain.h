#ifndef GRID_AIN_H
#define GRID_AIN_H

#include <stdbool.h>
#include <stdint.h>

struct ain_chan_t {

  uint16_t capa;
  uint16_t size;
  uint16_t beg;
  uint16_t end;
  uint16_t* data;

  uint32_t sum;
  uint16_t output;
  uint8_t delta;
};

void grid_ain_channel_reset(struct ain_chan_t* chan);
void grid_ain_channel_init(struct ain_chan_t* chan, uint8_t capacity);

struct grid_ain_model {

  uint8_t initialized;
  uint8_t channel_count;
  struct ain_chan_t* channels;
};

extern struct grid_ain_model grid_ain_state;

void grid_ain_init(struct grid_ain_model* ain, uint8_t channel_count, uint8_t capacity);
void grid_ain_add_sample(struct grid_ain_model* ain, uint8_t channel, uint16_t value, uint8_t src_res, uint8_t dst_res);
bool grid_ain_stabilized(struct grid_ain_model* ain, uint8_t channel);
bool grid_ain_get_changed(struct grid_ain_model* ain, uint8_t channel);
int32_t grid_ain_get_average_scaled(struct grid_ain_model* ain, uint8_t channel, uint8_t src_res, uint8_t dst_res, int32_t min, int32_t max);

#endif /* GRID_AIN_H */
