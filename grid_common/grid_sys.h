#ifndef GRID_SYS_H_INCLUDED
#define GRID_SYS_H_INCLUDED

#include "grid_protocol.h"

#include <stdint.h>

extern uint32_t grid_platform_get_id(uint32_t* return_array);
extern uint32_t grid_platform_get_hwcfg();
extern uint8_t grid_platform_get_random_8();
extern uint8_t grid_platform_get_reset_cause();

#define MS_TO_US 1000

#define GRID_SYS_BANK_MAXNUMBER 4

struct grid_sys_model {

  uint8_t reset_cause;

  uint8_t editor_connected;

  uint8_t midirx_any_enabled;
  uint8_t midirx_sync_enabled;

  uint8_t bank_activebank_number;

  uint8_t mapmodestate;

  uint32_t uniqueid_array[4];

  uint8_t bank_active_changed;

  uint8_t bank_setting_changed_flag;

  uint8_t bank_enabled[GRID_SYS_BANK_MAXNUMBER];

  uint8_t bank_color_r[GRID_SYS_BANK_MAXNUMBER];
  uint8_t bank_color_g[GRID_SYS_BANK_MAXNUMBER];
  uint8_t bank_color_b[GRID_SYS_BANK_MAXNUMBER];

  uint8_t bank_activebank_valid;

  uint8_t bank_activebank_color_r;
  uint8_t bank_activebank_color_g;
  uint8_t bank_activebank_color_b;

  uint8_t bank_init_flag;

  uint32_t hwfcg;

  int8_t module_x;
  int8_t module_y;
  uint8_t module_rot;
};

extern struct grid_sys_model grid_sys_state;

void grid_sys_init(struct grid_sys_model* sys);

uint8_t grid_sys_get_bank_num(struct grid_sys_model* sys);
uint8_t grid_sys_get_bank_next(struct grid_sys_model* sys);

uint8_t grid_sys_get_editor_connected_state(struct grid_sys_model* sys);
void grid_sys_set_editor_connected_state(struct grid_sys_model* sys, uint8_t state);

uint8_t grid_sys_get_midirx_any_state(struct grid_sys_model* sys);
uint8_t grid_sys_get_midirx_sync_state(struct grid_sys_model* sys);

void grid_sys_set_midirx_any_state(struct grid_sys_model* sys, uint8_t state);
void grid_sys_set_midirx_sync_state(struct grid_sys_model* sys, uint8_t state);

uint8_t grid_sys_get_module_x(struct grid_sys_model* sys);
uint8_t grid_sys_get_module_y(struct grid_sys_model* sys);
uint8_t grid_sys_get_module_rot(struct grid_sys_model* sys);

void grid_sys_set_module_x(struct grid_sys_model* sys, uint8_t x);
void grid_sys_set_module_y(struct grid_sys_model* sys, uint8_t y);
void grid_sys_set_module_rot(struct grid_sys_model* sys, uint8_t rot);

void grid_sys_set_module_absolute_position(struct grid_sys_model* sys, uint8_t sx, uint8_t sy, uint8_t rot, uint8_t portrot);

uint8_t grid_sys_get_bank_red(struct grid_sys_model* sys);
uint8_t grid_sys_get_bank_gre(struct grid_sys_model* sys);
uint8_t grid_sys_get_bank_blu(struct grid_sys_model* sys);

void grid_sys_set_bank_red(struct grid_sys_model* sys, uint8_t red);
void grid_sys_set_bank_gre(struct grid_sys_model* sys, uint8_t gre);
void grid_sys_set_bank_blu(struct grid_sys_model* sys, uint8_t blu);

void grid_sys_set_bank(struct grid_sys_model* sys, uint8_t value);

uint32_t grid_sys_get_hwcfg(struct grid_sys_model* sys);
uint32_t grid_sys_get_id(struct grid_sys_model* sys, uint32_t* return_array);

int grid_hwcfg_module_is_vsnx_rev_a(struct grid_sys_model* sys);

#endif
