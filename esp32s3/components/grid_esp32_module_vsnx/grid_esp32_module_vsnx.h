#ifndef GRID_ESP32_MODULE_VSNX_H
#define GRID_ESP32_MODULE_VSNX_H

#include "grid_cal.h"
#include "grid_config.h"
#include "grid_sys.h"
#include "grid_ui.h"

#include "grid_esp32_adc.h"
#include "grid_esp32_lcd.h"

void grid_esp32_module_vsnx_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_esp32_adc_model* adc, struct grid_config_model* conf, struct grid_cal_model* cal,
                                 struct grid_esp32_lcd_model* lcds);

#endif /* GRID_ESP32_MODULE_VSNX_H */
