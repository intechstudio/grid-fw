#ifndef GRID_CONFIG_H
#define GRID_CONFIG_H

#include "grid_cal.h"

#define GRID_CONFIG_MAX_UINT16_ARRAYELEM 7 // uint16_t + ',' + ' '

#define CAL_HEADER "calibration"
#define CAL_POT_CENTERS_KEY "potmeter-centers"
#define CAL_POT_DETENT_LOW_KEY "potmeter-detent-low"
#define CAL_POT_DETENT_HIGH_KEY "potmeter-detent-high"
#define CAL_RANGE_MINIMA_KEY "range-minima"
#define CAL_RANGE_MAXIMA_KEY "range-maxima"

enum {
  GRID_CONFIG_ERRBUF = 256,
};

struct grid_config_model {
  struct grid_cal_model* cal;
};

extern struct grid_config_model grid_config_state;

int grid_config_init(struct grid_config_model* config, struct grid_cal_model* cal);
int grid_config_parse(struct grid_config_model* config, char* src);
int grid_config_generate(struct grid_config_model* config, char* dest);
uint32_t grid_config_bytes(struct grid_config_model* config);

#endif /* GRID_CONFIG_H */
