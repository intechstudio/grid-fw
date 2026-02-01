#ifndef GRID_AIN_H
#define GRID_AIN_H

#include <stdbool.h>
#include <stdint.h>

#define GRID_MUX_UNUSED ((uint8_t)-1)

#define GRID_AIN_INTERNAL_RESOLUTION 12

#define GRID_ADC_MAX 4095
#define GRID_POTMETER_DEADZONE 128
#define GRID_POTMETER_CENTER 2192

struct grid_adc_result {
  uint8_t channel;
  uint8_t mux_state;
  uint16_t value;
};

typedef void (*grid_process_analog_t)(struct grid_adc_result* result);

#define GRID_ADC_INVERT_COND(value, element_index, invert_bm) ((value) ^ (GRID_ADC_MAX * (((invert_bm) >> (element_index)) & 1)))

// Find first valid mux position in constant time using count-trailing-zeros
// to locate the lowest set bit in the bitmask.
#define GRID_MUX_FIRST_VALID(index, mask) ((index) = __builtin_ctz(mask))

// Advance mux index to next valid position in constant time. The 8-bit mask
// is duplicated into a 16-bit value so that shifting past the current position
// wraps around naturally. Count-trailing-zeros then finds the distance to the
// next set bit, and the result is masked back to 3 bits (0-7).
#define GRID_MUX_INCREMENT(index, mask)                                                                                                                                                                \
  do {                                                                                                                                                                                                 \
    uint16_t _rotated = ((uint16_t)(mask) | ((uint16_t)(mask) << 8)) >> ((index) + 1);                                                                                                                 \
    (index) = ((index) + 1 + __builtin_ctz(_rotated)) & 0b111;                                                                                                                                         \
  } while (0)


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
