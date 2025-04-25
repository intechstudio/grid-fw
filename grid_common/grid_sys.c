#include "grid_sys.h"

struct grid_sys_model grid_sys_state;

void grid_sys_init(struct grid_sys_model* sys) {

  // PLATFORM SPECIFIC INITIALIZERS

  sys->reset_cause = grid_platform_get_reset_cause();
  sys->hwfcg = grid_platform_get_hwcfg();

  uint32_t uniqueid[4] = {0};
  grid_platform_get_id(uniqueid);

  sys->uniqueid_array[0] = uniqueid[0];
  sys->uniqueid_array[1] = uniqueid[1];
  sys->uniqueid_array[2] = uniqueid[2];
  sys->uniqueid_array[3] = uniqueid[3];

  // LOCAL INITIALIZERS

  sys->midirx_any_enabled = 1;
  sys->midirx_sync_enabled = 0;

  sys->module_x = 0; // 0 because this is signed int
  sys->module_y = 0; // 0 because this is signed int
  sys->module_rot = GRID_PARAMETER_DEFAULT_ROTATION;

  sys->bank_color_r[0] = 0;
  sys->bank_color_g[0] = 100;
  sys->bank_color_b[0] = 200;

  sys->bank_color_r[1] = 200;
  sys->bank_color_g[1] = 100;
  sys->bank_color_b[1] = 0;

  sys->bank_color_r[2] = 50;
  sys->bank_color_g[2] = 200;
  sys->bank_color_b[2] = 50;

  sys->bank_color_r[3] = 100;
  sys->bank_color_g[3] = 0;
  sys->bank_color_b[3] = 200;

  sys->bank_enabled[0] = 1;
  sys->bank_enabled[1] = 1;
  sys->bank_enabled[2] = 1;
  sys->bank_enabled[3] = 1;

  sys->bank_activebank_color_r = 0;
  sys->bank_activebank_color_g = 0;
  sys->bank_activebank_color_b = 0;

  sys->mapmodestate = 1;

  sys->bank_active_changed = 0;
  sys->bank_setting_changed_flag = 0;

  sys->bank_init_flag = 0;

  sys->bank_activebank_number = 0;

  sys->editor_connected = 0;
}

uint8_t grid_sys_get_bank_num(struct grid_sys_model* sys) { return sys->bank_activebank_number; }

uint8_t grid_sys_get_editor_connected_state(struct grid_sys_model* sys) { return sys->editor_connected; }
void grid_sys_set_editor_connected_state(struct grid_sys_model* sys, uint8_t state) { sys->editor_connected = state; }

uint8_t grid_sys_get_midirx_any_state(struct grid_sys_model* sys) { return sys->midirx_any_enabled; }
uint8_t grid_sys_get_midirx_sync_state(struct grid_sys_model* sys) { return sys->midirx_sync_enabled; }

void grid_sys_set_midirx_any_state(struct grid_sys_model* sys, uint8_t state) { sys->midirx_any_enabled = state; }
void grid_sys_set_midirx_sync_state(struct grid_sys_model* sys, uint8_t state) { sys->midirx_sync_enabled = state; }

int8_t grid_sys_get_module_x(struct grid_sys_model* sys) { return sys->module_x; }
int8_t grid_sys_get_module_y(struct grid_sys_model* sys) { return sys->module_y; }
uint8_t grid_sys_get_module_rot(struct grid_sys_model* sys) { return sys->module_rot; }

void grid_sys_set_module_x(struct grid_sys_model* sys, int8_t x) { sys->module_x = x; }
void grid_sys_set_module_y(struct grid_sys_model* sys, int8_t y) { sys->module_y = y; }
void grid_sys_set_module_rot(struct grid_sys_model* sys, uint8_t rot) { sys->module_rot = rot; }

void grid_sys_set_module_absolute_position(struct grid_sys_model* sys, uint8_t sx, uint8_t sy, uint8_t rot, uint8_t portrot) {

  assert(portrot < 4);

  // Convert to signed int
  int8_t recv_sx = sx - GRID_PARAMETER_DEFAULT_POSITION;
  int8_t recv_sy = sy - GRID_PARAMETER_DEFAULT_POSITION;

  int8_t sign_x[4] = {-1, -1, 1, 1};
  int8_t sign_y[4] = {-1, 1, 1, -1};

  uint8_t cross = portrot % 2;
  int8_t rot_sx = sign_x[portrot] * (recv_sx * !cross + recv_sy * cross);
  int8_t rot_sy = sign_y[portrot] * (recv_sy * !cross + recv_sx * cross);

  grid_sys_set_module_x(&grid_sys_state, rot_sx);
  grid_sys_set_module_y(&grid_sys_state, rot_sy);
  grid_sys_set_module_rot(&grid_sys_state, rot);
}

uint8_t grid_sys_get_bank_red(struct grid_sys_model* sys) { return sys->bank_activebank_color_r; }

uint8_t grid_sys_get_bank_gre(struct grid_sys_model* sys) { return sys->bank_activebank_color_g; }

uint8_t grid_sys_get_bank_blu(struct grid_sys_model* sys) { return sys->bank_activebank_color_b; }

void grid_sys_set_bank_red(struct grid_sys_model* sys, uint8_t red) { sys->bank_activebank_color_r = red; }
void grid_sys_set_bank_gre(struct grid_sys_model* sys, uint8_t gre) { sys->bank_activebank_color_g = gre; }
void grid_sys_set_bank_blu(struct grid_sys_model* sys, uint8_t blu) { sys->bank_activebank_color_b = blu; }

uint8_t grid_sys_get_bank_next(struct grid_sys_model* sys) {

  uint8_t current_active = grid_sys_get_bank_num(sys);

  for (uint8_t i = 0; i < GRID_SYS_BANK_MAXNUMBER; i++) {

    uint8_t bank_check = (current_active + i + 1) % GRID_SYS_BANK_MAXNUMBER;

    if (sys->bank_enabled[bank_check] == 1) {

      return bank_check;
    }
  }

  return current_active;
}

void grid_sys_set_bank(struct grid_sys_model* sys, uint8_t banknumber) {

  if (banknumber == 255) {

    // mod->bank_activebank_number = 0;
    sys->bank_activebank_valid = 0;

    sys->bank_active_changed = 1;

    sys->bank_activebank_color_r = 127;
    sys->bank_activebank_color_g = 127;
    sys->bank_activebank_color_b = 127;
  } else if (banknumber < GRID_SYS_BANK_MAXNUMBER) {

    sys->bank_init_flag = 1;

    if (sys->bank_enabled[banknumber] == 1) {

      sys->bank_activebank_number = banknumber;
      sys->bank_activebank_valid = 1;

      sys->bank_active_changed = 1;

      sys->bank_activebank_color_r = sys->bank_color_r[sys->bank_activebank_number];
      sys->bank_activebank_color_g = sys->bank_color_g[sys->bank_activebank_number];
      sys->bank_activebank_color_b = sys->bank_color_b[sys->bank_activebank_number];
    } else {

      // grid_debug_print_text("NOT ENABLED");
    }
  } else {

    // grid_debug_print_text("Invalid Bank Number");
  }
}

uint32_t grid_sys_get_hwcfg(struct grid_sys_model* sys) { return sys->hwfcg; }

uint32_t grid_sys_get_id(struct grid_sys_model* sys, uint32_t* return_array) {

  return_array[0] = sys->uniqueid_array[0];
  return_array[1] = sys->uniqueid_array[1];
  return_array[2] = sys->uniqueid_array[2];
  return_array[3] = sys->uniqueid_array[3];

  return 1;
}

int grid_hwcfg_module_is_vsnx_rev_a(struct grid_sys_model* sys) {

  if (grid_sys_get_hwcfg(sys) == GRID_MODULE_TEK1_RevA) {
    return 1;
  }
  if (grid_sys_get_hwcfg(sys) == GRID_MODULE_VSN1L_RevA) {
    return 1;
  }
  if (grid_sys_get_hwcfg(sys) == GRID_MODULE_VSN1R_RevA) {
    return 1;
  }
  if (grid_sys_get_hwcfg(sys) == GRID_MODULE_VSN2_RevA) {
    return 1;
  }

  return 0;
}
