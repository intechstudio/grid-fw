/*
 * grid_d51_nvm.h
 *
 * Created: 9/15/2020 3:41:53 PM
 *  Author: suku
 */

#ifndef GRID_D51_NVM_H_
#define GRID_D51_NVM_H_

#include "grid_d51_module.h"

#include "grid_d51_littlefs_api.h"

#include "grid_ui.h"

struct grid_d51_nvm_model {
  struct d51_littlefs_t dfs;
};

extern struct grid_d51_nvm_model grid_d51_nvm_state;

void grid_d51_nvm_mount(struct grid_d51_nvm_model* nvm, bool force_format);

#endif /* GRID_D51_NVM_H_ */
