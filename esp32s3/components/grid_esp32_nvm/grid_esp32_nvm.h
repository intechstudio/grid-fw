#ifndef GRID_ESP32_NVM_H
#define GRID_ESP32_NVM_H

#include <stdint.h>

#include "grid_ui.h"

#include "grid_esp32_littlefs.h"

struct grid_esp32_nvm_model {
  struct esp_littlefs_t efs;
};

extern struct grid_esp32_nvm_model grid_esp32_nvm_state;

void grid_esp32_nvm_mount(struct grid_esp32_nvm_model* nvm, bool force_format);

void grid_esp32_nvm_task(void* arg);

#endif /* GRID_ESP32_NVM_H */
