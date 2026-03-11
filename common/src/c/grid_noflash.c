#include "grid_noflash.h"

struct grid_ui_event* grid_ui_event_find(struct grid_ui_element* ele, uint8_t event_type) {

  if (!ele) {
    return NULL;
  }

  uint8_t i = 0;
  while (i < ele->event_list_length && ele->event_list[i].type != event_type) {
    ++i;
  }

  return i < ele->event_list_length ? &ele->event_list[i] : NULL;
}

void grid_ui_event_state_set(struct grid_ui_event* eve, enum grid_eve_state_t state) {

  // TODO assert instead
  if (!eve) {
    return;
  }

  eve->state = state;
}
