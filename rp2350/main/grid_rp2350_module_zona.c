#include "grid_rp2350_module_zona.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "grid_platform.h"
#include "grid_ui_touch.h"

#include "grid_rp2350_touch.h"

#define RP2350_PIN_ZONA_SDA 14
#define RP2350_PIN_ZONA_SCL 15
#define RP2350_PIN_ZONA_RST 17
#define RP2350_PIN_ZONA_INT 18
#define RP2350_ZONA_I2C_FREQ_HZ 100000

static struct grid_ui_model* ui_ptr = NULL;

GRID_IRAM_ATTR static void zona_process_touch(struct touchinfo_t* info) {

  struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, 0);
  if (!ele) {
    return;
  }

  grid_ui_touch_store_input(grid_ui_touch_get_state(ele), *info);
}

void grid_rp2350_module_zona_init(struct grid_sys_model* sys, struct grid_ui_model* ui) {

  grid_rp2350_touch_init(&grid_rp2350_touch_state, RP2350_PIN_ZONA_SCL, RP2350_PIN_ZONA_SDA, RP2350_PIN_ZONA_RST, RP2350_PIN_ZONA_INT, RP2350_ZONA_I2C_FREQ_HZ, &grid_maxtouch_state.data);
  grid_rp2350_touch_init_chg_cb(&grid_rp2350_touch_state, RP2350_PIN_ZONA_INT, grid_maxtouch_chg_cb, &grid_maxtouch_state);
  grid_rp2350_touch_init_async_cb(&grid_rp2350_touch_state, grid_maxtouch_async_cb, &grid_maxtouch_state);

  grid_maxtouch_init(&grid_maxtouch_state, zona_process_touch);

  grid_maxtouch_update(&grid_maxtouch_state.data);

  for (int i = 0; i < ui->element_list_length; ++i) {
    struct grid_ui_element* ele = &ui->element_list[i];
    if (ele->type == GRID_PARAMETER_ELEMENT_TOUCH) {
      grid_ui_touch_state_init(grid_ui_touch_get_state(ele), 10);
    }
  }
}
