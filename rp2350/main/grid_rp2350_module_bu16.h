#ifndef GRID_RP2350_MODULE_BU16_H
#define GRID_RP2350_MODULE_BU16_H

#include "grid_cal.h"
#include "grid_config.h"
#include "grid_sys.h"
#include "grid_ui.h"

#include "grid_rp2350_adc.h"

void grid_rp2350_module_bu16_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_rp2350_adc_model* adc, struct grid_config_model* conf, struct grid_cal_model* cal);

#endif /* GRID_RP2350_MODULE_BU16_H */
