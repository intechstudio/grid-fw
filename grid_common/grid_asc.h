#ifndef GRID_ASC_H
#define GRID_ASC_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

struct grid_asc {
  uint32_t sum;
  uint8_t count;
  uint8_t factor;
};

void grid_asc_array_set_factors(struct grid_asc* asc, size_t capacity, uint8_t start, uint8_t length, uint8_t factor);
bool grid_asc_process(struct grid_asc* ads, uint16_t rx, uint16_t* tx);

#endif /* GRID_ASC_H */
