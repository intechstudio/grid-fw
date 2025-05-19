#ifndef GRID_NOFLASH_H
#define GRID_NOFLASH_H

#include "grid_ui.h"

struct grid_ui_event* grid_ui_event_find(struct grid_ui_element* ele, uint8_t event_type);

void grid_ui_event_trigger(struct grid_ui_event* eve);

#endif /* GRID_NOFLASH_H */
