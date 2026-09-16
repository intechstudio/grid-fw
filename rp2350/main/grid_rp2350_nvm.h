#ifndef GRID_RP2350_NVM_H
#define GRID_RP2350_NVM_H

#include "grid_rp2350_littlefs_api.h"

struct grid_rp2350_nvm_model {
  struct rp2350_littlefs_t rfs;
};

extern struct grid_rp2350_nvm_model grid_rp2350_nvm_state;

void grid_rp2350_nvm_mount(struct grid_rp2350_nvm_model* nvm, bool force_format);

#endif /* GRID_RP2350_NVM_H */
