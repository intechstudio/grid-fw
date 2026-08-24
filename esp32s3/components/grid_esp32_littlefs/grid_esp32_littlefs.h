#ifndef GRID_ESP32_LITTLEFS_H
#define GRID_ESP32_LITTLEFS_H

#include "lfs.h"

#include "grid_esp32_littlefs_api.h"

esp_err_t grid_esp32_littlefs_mount(struct esp_littlefs_t* efs, bool force_format);
esp_err_t grid_esp32_littlefs_unmount(struct esp_littlefs_t* efs);

#endif /* GRID_ESP32_LITTLEFS_H */
