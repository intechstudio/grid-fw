#ifndef GRID_D51_MODULE_EN16_H_INCLUDED
#define GRID_D51_MODULE_EN16_H_INCLUDED

#include "grid_d51_module.h"

struct grid_sys_model;
struct grid_ui_model;
struct grid_d51_encoder_model;

void grid_d51_module_en16_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_d51_encoder_model* enc);

#endif
