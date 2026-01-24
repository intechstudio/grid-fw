#pragma once

#include "esp_err.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t grid_esp32_http_init(void);
esp_err_t grid_esp32_ws_broadcast(const char* data, size_t len);

#ifdef __cplusplus
}
#endif
