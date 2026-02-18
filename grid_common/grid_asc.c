#include "grid_asc.h"

void grid_asc_set_factor(struct grid_asc* asc, uint8_t index, uint8_t factor) { asc[index].factor = factor; }

bool grid_asc_process(struct grid_asc* asc, uint8_t index, uint16_t rx, uint16_t* tx) {

  struct grid_asc* entry = &asc[index];

  entry->sum += rx;

  *tx = entry->sum / (entry->count + 1);

  entry->count = (entry->count + 1) % entry->factor;

  entry->sum *= entry->count != 0;

  return entry->count == 0;
}
