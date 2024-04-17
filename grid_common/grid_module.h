#pragma once

#ifndef GRID_MODULE_H_INCLUDED
#define GRID_MODULE_H_INCLUDED

// only for uint definitions
#include <stdint.h>
// only for malloc
#include <math.h>
#include <stdlib.h>

#include "grid_ain.h"
#include "grid_led.h"
#include "grid_lua_api.h"
#include "grid_protocol.h"
#include "grid_ui.h"

void grid_module_po16_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui);
void grid_module_bu16_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui);
void grid_module_pbf4_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui);
void grid_module_en16_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui);
void grid_module_ef44_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui);
void grid_module_tek2_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui);
void grid_module_pb44_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui);

extern uint64_t grid_platform_rtc_get_elapsed_time(uint64_t told);

#endif
