#ifndef GRID_D51_MODULE_PBF4_H_INCLUDED
#define GRID_D51_MODULE_PBF4_H_INCLUDED

#include "grid_d51_module.h"

struct grid_sys_model;
struct grid_ui_model;
struct grid_config_model;
struct grid_cal_model;

void grid_d51_module_pbf4_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_config_model* conf, struct grid_cal_model* cal);

#endif
