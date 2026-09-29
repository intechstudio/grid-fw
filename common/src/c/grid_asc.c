#include "grid_asc.h"

#include "grid_platform.h"

GRID_IRAM_ATTR void grid_asc_set_factor(struct grid_asc* asc, uint8_t factor) { asc->factor = factor; }

GRID_IRAM_ATTR bool grid_asc_process(struct grid_asc* asc, uint16_t rx, uint16_t* tx) {

  struct grid_asc* entry = asc;

  entry->sum += rx;

  *tx = entry->sum / (entry->count + 1);

  entry->count = (entry->count + 1) % entry->factor;

  entry->sum *= entry->count != 0;

  return entry->count == 0;
}
