#ifndef GRID_RP2350_LITTLEFS_H
#define GRID_RP2350_LITTLEFS_H

#include "grid_rp2350_littlefs_api.h"

#include "littlefs/lfs.h"

int grid_rp2350_littlefs_mount(struct rp2350_littlefs_t* rfs, bool force_format);
int grid_rp2350_littlefs_unmount(struct rp2350_littlefs_t* rfs);

#endif /* GRID_RP2350_LITTLEFS_H */
