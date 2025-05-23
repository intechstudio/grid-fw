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

int grid_config_parse_cal_pot(struct grid_cal_pot* cal_pot, char* errbuf, toml_table_t* calibration) {

  toml_array_t* centers = toml_array_in(calibration, CAL_POT_CENTERS_KEY);
  if (!centers) {
    return 1;
  }

  for (int i = 0; i < cal_pot->length; ++i) {
    toml_datum_t center = toml_int_at(centers, i);
    if (center.ok) {
      grid_cal_pot_center_set(cal_pot, i, (uint16_t)center.u.i);
    }
  }

  return 0;
}

int grid_config_parse_cal_but(struct grid_cal_but* cal_but, char* errbuf, toml_table_t* calibration) {

  toml_array_t* minima = toml_array_in(calibration, CAL_BUT_MINIMA_KEY);
  if (!minima) {
    return 1;
  }

  for (int i = 0; i < cal_but->length; ++i) {
    toml_datum_t min = toml_int_at(minima, i);
    if (min.ok) {
      grid_cal_but_min_set(cal_but, i, (uint16_t)min.u.i);
    }
  }

  toml_array_t* maxima = toml_array_in(calibration, CAL_BUT_MAXIMA_KEY);
  if (!maxima) {
    return 1;
  }

  for (int i = 0; i < cal_but->length; ++i) {
    toml_datum_t max = toml_int_at(maxima, i);
    if (max.ok) {
      grid_cal_but_max_set(cal_but, i, (uint16_t)max.u.i);
    }
  }

  return 0;
}

int grid_config_parse_cal(struct grid_cal_model* cal, char* errbuf, toml_table_t* conf) {

  if (!conf) {
    return 1;
  }

  toml_table_t* calibration = toml_table_in(conf, CAL_HEADER);
  if (!calibration) {
    return 1;
  }

  grid_config_parse_cal_pot(&cal->potmeter, errbuf, calibration);

  grid_config_parse_cal_but(&cal->button, errbuf, calibration);

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

int grid_config_generate_cal_pot(struct grid_cal_pot* cal_pot, char* dest) {

  if (cal_pot->length == 0) {
    return 0;
  }

  toml_cat_bare_key(dest, CAL_POT_CENTERS_KEY);

  strcat(dest, "[ ");

  char potcal[GRID_CONFIG_MAX_UINT16_ARRAYELEM + 1] = {0};
  for (int i = 0; i < cal_pot->length; ++i) {

    uint16_t center;
    int status = grid_cal_pot_center_get(cal_pot, i, &center);
    if (status) {
      return status;
    }

    snprintf(potcal, GRID_CONFIG_MAX_UINT16_ARRAYELEM + 1, "%hu, ", center);
    strcat(dest, potcal);
  }

  strcat(dest, "]\n");

  return 0;
}

int grid_config_generate_cal_but(struct grid_cal_but* cal_but, char* dest) {

  if (cal_but->length == 0) {
    return 0;
  }

  char butcal[GRID_CONFIG_MAX_UINT16_ARRAYELEM + 1] = {0};
  uint16_t min = 0;
  uint16_t max = 0;

  toml_cat_bare_key(dest, CAL_BUT_MINIMA_KEY);
  strcat(dest, "[ ");

  for (int i = 0; i < cal_but->length; ++i) {

    int status = grid_cal_but_minmax_get(cal_but, i, &min, &max);
    if (status) {
      return status;
    }

    snprintf(butcal, GRID_CONFIG_MAX_UINT16_ARRAYELEM + 1, "%hu, ", min);
    strcat(dest, butcal);
  }

  strcat(dest, "]\n");

  toml_cat_bare_key(dest, CAL_BUT_MAXIMA_KEY);
  strcat(dest, "[ ");

  for (int i = 0; i < cal_but->length; ++i) {

    int status = grid_cal_but_minmax_get(cal_but, i, &min, &max);
    if (status) {
      return status;
    }

    snprintf(butcal, GRID_CONFIG_MAX_UINT16_ARRAYELEM + 1, "%hu, ", max);
    strcat(dest, butcal);
  }

  strcat(dest, "]\n");

  return 0;
}

int grid_config_generate_cal(struct grid_cal_model* cal, char* dest) {

  int status;

  toml_cat_table_header(dest, CAL_HEADER);

  status = grid_config_generate_cal_pot(&cal->potmeter, dest);
  if (status) {
    return status;
  }

  status = grid_config_generate_cal_but(&cal->button, dest);
  if (status) {
    return status;
  }

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

uint32_t grid_config_bytes_cal(struct grid_cal_model* cal) {

  uint32_t bytes = 0;

  bytes += strlen("[" CAL_HEADER "]\n");

  bytes += strlen(CAL_POT_CENTERS_KEY " = ");
  bytes += strlen("[ ");
  bytes += cal->potmeter.length * GRID_CONFIG_MAX_UINT16_ARRAYELEM;
  bytes += strlen("]\n");

  bytes += strlen(CAL_BUT_MINIMA_KEY " = ");
  bytes += strlen("[ ");
  bytes += cal->button.length * GRID_CONFIG_MAX_UINT16_ARRAYELEM;
  bytes += strlen("]\n");

  bytes += strlen(CAL_BUT_MAXIMA_KEY " = ");
  bytes += strlen("[ ");
  bytes += cal->button.length * GRID_CONFIG_MAX_UINT16_ARRAYELEM;
  bytes += strlen("]\n");

  return bytes;
}

uint32_t grid_config_bytes(struct grid_config_model* config) {

  uint32_t bytes = 0;

  bytes += grid_config_bytes_cal(config->cal);
  bytes += 1; // \0

  return bytes;
}
