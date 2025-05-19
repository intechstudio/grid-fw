#include "grid_noflash.h"

struct grid_ui_event* grid_ui_event_find(struct grid_ui_element* ele, uint8_t event_type) {

  if (ele == NULL) {
    return NULL;
  }

  for (uint8_t i = 0; i < ele->event_list_length; i++) {
    if (ele->event_list[i].type == event_type) {
      return &ele->event_list[i];
    }
  }

  return NULL;
}

void grid_ui_event_trigger(struct grid_ui_event* eve) {

  if (eve != NULL) {

    eve->trigger = GRID_UI_STATUS_TRIGGERED;
  }
}
