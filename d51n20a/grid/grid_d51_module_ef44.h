#ifndef GRID_D51_MODULE_EF44_H_INCLUDED
#define GRID_D51_MODULE_EF44_H_INCLUDED

#include "grid_d51_module.h"

struct grid_sys_model;
struct grid_ui_model;
struct grid_d51_adc_model;
struct grid_d51_encoder_model;
struct grid_config_model;
struct grid_cal_model;

void grid_d51_module_ef44_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_d51_adc_model* adc, struct grid_d51_encoder_model* enc, struct grid_config_model* conf,
                               struct grid_cal_model* cal);

#endif
