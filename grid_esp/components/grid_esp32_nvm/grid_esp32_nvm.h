/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>

#include "grid_ui.h"

#include "grid_esp32_littlefs.h"

#ifdef __cplusplus
extern "C" {
#endif

struct grid_esp32_nvm_model {
  struct esp_littlefs_t efs;
};

extern struct grid_esp32_nvm_model grid_esp32_nvm_state;

void grid_esp32_nvm_mount(struct grid_esp32_nvm_model* nvm);

void grid_esp32_nvm_task(void* arg);

#ifdef __cplusplus
}
#endif
