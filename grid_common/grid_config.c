/*
 * grid_config.c
 *
 * Created: 29/11/2024 13:35:37 PM
 * Author : BENB
 */

#include "grid_config.h"

#include <stdio.h>
#include <string.h>

#include "tomlc99/toml.h"

extern void grid_platform_printf(char const* fmt, ...);

struct grid_config_model grid_config_state;

int grid_config_init(struct grid_config_model* config, struct grid_cal_model* cal) {

  if (!cal) {
    return 1;
  }

  config->cal = cal;

  return 0;
}

int grid_config_parse_cal(struct grid_cal_model* cal, char* errbuf, toml_table_t* conf) {

  if (!conf) {
    return 1;
  }

  toml_table_t* calibration = toml_table_in(conf, "calibration");
  if (!calibration) {
    return 1;
  }

  toml_array_t* centers = toml_array_in(calibration, "centers");
  if (!centers) {
    return 1;
  }

  for (int i = 0; i < cal->length; ++i) {
    toml_datum_t center = toml_int_at(centers, i);
    if (center.ok) {
      grid_cal_center_set(cal, i, (uint16_t)center.u.i);
    }
  }

  return 0;
}

int grid_config_parse(struct grid_config_model* config, char* src) {

  char errbuf[GRID_CONFIG_ERRBUF];

  int status;

  toml_table_t* conf = toml_parse(src, errbuf, GRID_CONFIG_ERRBUF);

  if (!conf) {
    return 1;
  }

  status = grid_config_parse_cal(config->cal, errbuf, conf); // HECK
  if (status) {
    return status;
  }

  toml_free(conf);
  return 0;
}

void toml_cat_table_header(char* dest, char* name) {

  strcat(dest, "[");
  strcat(dest, name);
  strcat(dest, "]\n");
}

void toml_cat_bare_key(char* dest, char* name) {

  strcat(dest, name);
  strcat(dest, " = ");
}

int grid_config_generate_cal(struct grid_cal_model* cal, char* dest) {

  toml_cat_table_header(dest, "calibration");

  toml_cat_bare_key(dest, "centers");

  strcat(dest, "[ ");

  char potcal[GRID_CONFIG_MAX_UINT16_ARRAYELEM + 1] = {0};
  for (int i = 0; i < cal->length; ++i) {

    uint16_t center;
    int status = grid_cal_center_get(cal, i, &center);
    if (status) {
      return status;
    }

    snprintf(potcal, GRID_CONFIG_MAX_UINT16_ARRAYELEM + 1, "%hu, ", center);
    strcat(dest, potcal);
  }

  strcat(dest, "]\n");

  return 0;
}

int grid_config_generate(struct grid_config_model* config, char* dest) {

  int status;

  dest[0] = '\0';

  status = grid_config_generate_cal(config->cal, dest);
  if (status) {
    return status;
  }

  return 0;
}

uint32_t grid_config_bytes_cal(struct grid_cal_model* cal) { return strlen("[calibration]\n") + strlen("centers = ") + strlen("[ ") + cal->length * GRID_CONFIG_MAX_UINT16_ARRAYELEM + strlen("]\n"); }

uint32_t grid_config_bytes(struct grid_config_model* config) { return grid_config_bytes_cal(config->cal) + 1 /*\0*/; }
