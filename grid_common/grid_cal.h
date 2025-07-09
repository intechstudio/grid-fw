#ifndef GRID_CAL_H
#define GRID_CAL_H

#include <stdbool.h>
#include <stdint.h>

struct grid_cal_pot {

  uint8_t resolution;
  uint8_t length;
  uint16_t maximum;
  uint16_t* value;
  uint16_t* center;
  uint16_t* detentlo;
  uint16_t* detenthi;
  uint8_t* enable;
};

int grid_cal_pot_init(struct grid_cal_pot* cal, uint8_t resolution, uint8_t length);
int grid_cal_pot_enable_range(struct grid_cal_pot* cal, uint8_t start, uint8_t length);
int grid_cal_pot_enable_get(struct grid_cal_pot* cal, uint8_t channel, uint8_t* enable);
int grid_cal_pot_center_get(struct grid_cal_pot* cal, uint8_t channel, uint16_t* center);
int grid_cal_pot_center_set(struct grid_cal_pot* cal, uint8_t channel, uint16_t center);
int grid_cal_pot_detent_get(struct grid_cal_pot* cal, uint8_t channel, uint16_t* detent, bool high);
int grid_cal_pot_detent_set(struct grid_cal_pot* cal, uint8_t channel, uint16_t detent, bool high);
int grid_cal_pot_value_get(struct grid_cal_pot* cal, uint8_t channel, uint16_t* value);
int grid_cal_pot_next(struct grid_cal_pot* cal, uint8_t channel, uint16_t in, uint16_t* out);

struct grid_ui_button_state;
uint16_t grid_ui_button_state_get_min(struct grid_ui_button_state* state);
uint16_t grid_ui_button_state_get_max(struct grid_ui_button_state* state);
void grid_ui_button_state_value_update(struct grid_ui_button_state* state, uint16_t value, uint64_t now);

struct grid_cal_but {

  uint8_t length;
  uint8_t* enable;
  struct grid_ui_button_state** states;
};

int grid_cal_but_init(struct grid_cal_but* cal, uint8_t length);
int grid_cal_but_enable_get(struct grid_cal_but* cal, uint8_t channel, uint8_t* enable);
int grid_cal_but_enable_set(struct grid_cal_but* cal, uint8_t channel, struct grid_ui_button_state* state);
int grid_cal_but_minmax_get(struct grid_cal_but* cal, uint8_t channel, uint16_t* min, uint16_t* max);
int grid_cal_but_min_set(struct grid_cal_but* cal, uint8_t channel, uint16_t min);
int grid_cal_but_max_set(struct grid_cal_but* cal, uint8_t channel, uint16_t max);
struct grid_ui_button_state* grid_cal_but_state_get(struct grid_cal_but* cal, uint8_t channel);

struct grid_cal_model {

  struct grid_cal_pot potmeter;
  struct grid_cal_but button;
};

extern struct grid_cal_model grid_cal_state;

#endif /* GRID_CAL_H */
