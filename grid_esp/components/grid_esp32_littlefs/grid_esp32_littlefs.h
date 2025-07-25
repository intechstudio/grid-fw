#pragma once

#include "lfs.h"

#include "littlefs_api.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t grid_esp32_littlefs_mount(struct esp_littlefs_t* efs);

// void grid_esp32_littlefs_get_total_and_used_bytes(struct esp_littlefs_t* efs, size_t* total, size_t* used);

#ifdef __cplusplus
}
#endif
