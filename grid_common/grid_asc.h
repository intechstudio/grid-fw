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

void grid_asc_set_factor(struct grid_asc* asc, uint8_t factor);
bool grid_asc_process(struct grid_asc* asc, uint16_t rx, uint16_t* tx);

#endif /* GRID_ASC_H */
