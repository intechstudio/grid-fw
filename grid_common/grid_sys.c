/*
 * grid_sys.c
 *
 * Created: 4/12/2019 5:27:13 PM
 * Author : SUKU WC
 */

#include "grid_sys.h"

struct grid_sys_model grid_sys_state;

void grid_sys_init(struct grid_sys_model *mod) {

  // PLATFORM SPECIFIC INITIALIZERS

  mod->reset_cause = grid_platform_get_reset_cause();
  mod->hwfcg = grid_platform_get_hwcfg();

  uint32_t uniqueid[4] = {0};
  grid_platform_get_id(uniqueid);

  mod->uniqueid_array[0] = uniqueid[0];
  mod->uniqueid_array[1] = uniqueid[1];
  mod->uniqueid_array[2] = uniqueid[2];
  mod->uniqueid_array[3] = uniqueid[3];

  // LOCAL INITIALIZERS

  mod->midirx_any_enabled = 1;
  mod->midirx_sync_enabled = 0;

  mod->module_x = 0; // 0 because this is signed int
  mod->module_y = 0; // 0 because this is signed int
  mod->module_rot = GRID_PARAMETER_DEFAULT_ROTATION;

  mod->bank_color_r[0] = 0;
  mod->bank_color_g[0] = 100;
  mod->bank_color_b[0] = 200;

  mod->bank_color_r[1] = 200;
  mod->bank_color_g[1] = 100;
  mod->bank_color_b[1] = 0;

  mod->bank_color_r[2] = 50;
  mod->bank_color_g[2] = 200;
  mod->bank_color_b[2] = 50;

  mod->bank_color_r[3] = 100;
  mod->bank_color_g[3] = 0;
  mod->bank_color_b[3] = 200;

  mod->bank_enabled[0] = 1;
  mod->bank_enabled[1] = 1;
  mod->bank_enabled[2] = 1;
  mod->bank_enabled[3] = 1;

  mod->bank_activebank_color_r = 0;
  mod->bank_activebank_color_g = 0;
  mod->bank_activebank_color_b = 0;

  mod->mapmodestate = 1;

  mod->bank_active_changed = 0;
  mod->bank_setting_changed_flag = 0;

  mod->bank_init_flag = 0;

  mod->bank_activebank_number = 0;

  mod->editor_connected = 0;
}

uint8_t grid_sys_get_bank_num(struct grid_sys_model *mod) {

  return mod->bank_activebank_number;
}

uint8_t grid_sys_get_editor_connected_state(struct grid_sys_model *mod) {
  return mod->editor_connected;
}
void grid_sys_set_editor_connected_state(struct grid_sys_model *mod,
                                         uint8_t state) {
  mod->editor_connected = state;
}

uint8_t grid_sys_get_midirx_any_state(struct grid_sys_model *mod) {
  return mod->midirx_any_enabled;
}
uint8_t grid_sys_get_midirx_sync_state(struct grid_sys_model *mod) {
  return mod->midirx_sync_enabled;
}

void grid_sys_set_midirx_any_state(struct grid_sys_model *mod, uint8_t state) {
  mod->midirx_any_enabled = state;
}
void grid_sys_set_midirx_sync_state(struct grid_sys_model *mod, uint8_t state) {
  mod->midirx_sync_enabled = state;
}

uint8_t grid_sys_get_module_x(struct grid_sys_model *mod) {
  return mod->module_x;
}
uint8_t grid_sys_get_module_y(struct grid_sys_model *mod) {
  return mod->module_y;
}
uint8_t grid_sys_get_module_rot(struct grid_sys_model *mod) {
  return mod->module_rot;
}

void grid_sys_set_module_x(struct grid_sys_model *mod, uint8_t x) {
  mod->module_x = x;
}
void grid_sys_set_module_y(struct grid_sys_model *mod, uint8_t y) {
  mod->module_y = y;
}
void grid_sys_set_module_rot(struct grid_sys_model *mod, uint8_t rot) {
  mod->module_rot = rot;
}

void grid_sys_set_module_absolute_position(struct grid_sys_model *mod,
                                           uint8_t sx, uint8_t sy, uint8_t rot,
                                           uint8_t portrot) {

  // from usb connected module
  int8_t received_sx =
      sx - GRID_PARAMETER_DEFAULT_POSITION; // convert to signed ind
  int8_t received_sy =
      sy - GRID_PARAMETER_DEFAULT_POSITION; // convert to signed ind
  int8_t rotated_sx = 0;
  int8_t rotated_sy = 0;

  // APPLY THE 2D ROTATION MATRIX

  // printf("Protrot %d \r\n", portrot);

  if (portrot == 0) { // 0 deg

    rotated_sx -= received_sx;
    rotated_sy -= received_sy;
  } else if (portrot == 1) { // 90 deg

    rotated_sx -= received_sy;
    rotated_sy += received_sx;
  } else if (portrot == 2) { // 180 deg

    rotated_sx += received_sx;
    rotated_sy += received_sy;
  } else if (portrot == 3) { // 270 deg

    rotated_sx += received_sy;
    rotated_sy -= received_sx;
  } else {
    // TRAP INVALID MESSAGE
  }

  grid_sys_set_module_x(&grid_sys_state, rotated_sx);
  grid_sys_set_module_y(&grid_sys_state, rotated_sy);
  grid_sys_set_module_rot(&grid_sys_state, rot);
}

uint8_t grid_sys_get_bank_red(struct grid_sys_model *mod) {

  return mod->bank_activebank_color_r;
}

uint8_t grid_sys_get_bank_gre(struct grid_sys_model *mod) {

  return mod->bank_activebank_color_g;
}

uint8_t grid_sys_get_bank_blu(struct grid_sys_model *mod) {

  return mod->bank_activebank_color_b;
}

void grid_sys_set_bank_red(struct grid_sys_model *mod, uint8_t red) {

  mod->bank_activebank_color_r = red;
}
void grid_sys_set_bank_gre(struct grid_sys_model *mod, uint8_t gre) {

  mod->bank_activebank_color_g = gre;
}
void grid_sys_set_bank_blu(struct grid_sys_model *mod, uint8_t blu) {

  mod->bank_activebank_color_b = blu;
}

uint8_t grid_sys_get_bank_next(struct grid_sys_model *mod) {

  uint8_t current_active = grid_sys_get_bank_num(mod);

  for (uint8_t i = 0; i < GRID_SYS_BANK_MAXNUMBER; i++) {

    uint8_t bank_check = (current_active + i + 1) % GRID_SYS_BANK_MAXNUMBER;

    if (mod->bank_enabled[bank_check] == 1) {

      return bank_check;
    }
  }

  return current_active;
}

void grid_sys_set_bank(struct grid_sys_model *mod, uint8_t banknumber) {

  if (banknumber == 255) {

    // mod->bank_activebank_number = 0;
    mod->bank_activebank_valid = 0;

    mod->bank_active_changed = 1;

    mod->bank_activebank_color_r = 127;
    mod->bank_activebank_color_g = 127;
    mod->bank_activebank_color_b = 127;

  } else if (banknumber < GRID_SYS_BANK_MAXNUMBER) {

    mod->bank_init_flag = 1;

    if (mod->bank_enabled[banknumber] == 1) {

      mod->bank_activebank_number = banknumber;
      mod->bank_activebank_valid = 1;

      mod->bank_active_changed = 1;

      mod->bank_activebank_color_r =
          mod->bank_color_r[mod->bank_activebank_number];
      mod->bank_activebank_color_g =
          mod->bank_color_g[mod->bank_activebank_number];
      mod->bank_activebank_color_b =
          mod->bank_color_b[mod->bank_activebank_number];

    } else {

      // grid_debug_print_text("NOT ENABLED");
    }

  } else {

    // grid_debug_print_text("Invalid Bank Number");
  }
}

uint32_t grid_sys_get_hwcfg(struct grid_sys_model *mod) { return mod->hwfcg; }

uint32_t grid_sys_get_id(struct grid_sys_model *mod, uint32_t *return_array) {

  return_array[0] = mod->uniqueid_array[0];
  return_array[1] = mod->uniqueid_array[1];
  return_array[2] = mod->uniqueid_array[2];
  return_array[3] = mod->uniqueid_array[3];

  return 1;
}
