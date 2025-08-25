#ifndef GRID_D51_LITTLEFS_H
#define GRID_D51_LITTLEFS_H

#include "grid_d51_module.h"
// this comment guards the include order from the formatter

#include "grid_d51_littlefs_api.h"

#include "lfs.h"

int grid_d51_littlefs_mount(struct d51_littlefs_t* dfs);

#endif /* GRID_D51_LITTLEFS_H */
