#pragma once

#include "lfs.h"

#include "grid_esp32_littlefs_api.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t grid_esp32_littlefs_mount(struct esp_littlefs_t* efs);

#ifdef __cplusplus
}
#endif
