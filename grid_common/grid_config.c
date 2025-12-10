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

  toml_table_t* calibration = toml_table_in(conf, CAL_HEADER);
  if (!calibration) {
    return 1;
  }

  toml_array_t* centers = toml_array_in(calibration, CAL_POT_CENTERS_KEY);
  if (!centers) {
    return 1;
  }

  for (int i = 0; i < cal->length; ++i) {
    toml_datum_t ctr = toml_int_at(centers, i);
    if (ctr.ok) {
      struct grid_cal_center* center;
      int status = grid_cal_get(cal, i, GRID_CAL_CENTER, (void**)&center);
      if (status) {
        return status;
      }
      if (center) {
        center->center = (uint16_t)ctr.u.i;
      }
    }
  }

  toml_array_t* minima = toml_array_in(calibration, CAL_RANGE_MINIMA_KEY);
  if (!minima) {
    return 1;
  }

  for (int i = 0; i < cal->length; ++i) {
    toml_datum_t min = toml_int_at(minima, i);
    if (min.ok) {
      struct grid_cal_limits* limits;
      int status = grid_cal_get(cal, i, GRID_CAL_LIMITS, (void**)&limits);
      if (status) {
        return status;
      }
      if (limits) {
        limits->min = (uint16_t)min.u.i;
      }
    }
  }

  toml_array_t* maxima = toml_array_in(calibration, CAL_RANGE_MAXIMA_KEY);
  if (!maxima) {
    return 1;
  }

  for (int i = 0; i < cal->length; ++i) {
    toml_datum_t max = toml_int_at(maxima, i);
    if (max.ok) {
      struct grid_cal_limits* limits;
      int status = grid_cal_get(cal, i, GRID_CAL_LIMITS, (void**)&limits);
      if (status) {
        return status;
      }
      if (limits) {
        limits->max = (uint16_t)max.u.i;
      }
    }
  }

  toml_array_t* detentlo = toml_array_in(calibration, CAL_POT_DETENT_LOW_KEY);
  if (!detentlo) {
    return 1;
  }

  for (int i = 0; i < cal->length; ++i) {
    toml_datum_t det = toml_int_at(detentlo, i);
    if (det.ok) {
      struct grid_cal_detent* detent;
      int status = grid_cal_get(cal, i, GRID_CAL_DETENT, (void**)&detent);
      if (status) {
        return status;
      }
      if (detent) {
        detent->lo = (uint16_t)det.u.i;
      }
    }
  }

  toml_array_t* detenthi = toml_array_in(calibration, CAL_POT_DETENT_HIGH_KEY);
  if (!detenthi) {
    return 1;
  }

  for (int i = 0; i < cal->length; ++i) {
    toml_datum_t det = toml_int_at(detenthi, i);
    if (det.ok) {
      struct grid_cal_detent* detent;
      int status = grid_cal_get(cal, i, GRID_CAL_DETENT, (void**)&detent);
      if (status) {
        return status;
      }
      if (detent) {
        detent->hi = (uint16_t)det.u.i;
      }
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

  int status;

  toml_cat_table_header(dest, CAL_HEADER);

  char temp[GRID_CONFIG_MAX_UINT16_ARRAYELEM + 1] = {0};

  for (int i = 0; i < cal->length; ++i) {

    if (i == 0) {
      toml_cat_bare_key(dest, CAL_POT_CENTERS_KEY);
      strcat(dest, "[ ");
    }

    struct grid_cal_center* center;
    int status = grid_cal_get(cal, i, GRID_CAL_CENTER, (void**)&center);
    if (status) {
      return status;
    }

    uint16_t ctr = grid_cal_center_center_get(center);
    snprintf(temp, GRID_CONFIG_MAX_UINT16_ARRAYELEM + 1, "%hu, ", ctr);
    strcat(dest, temp);

    if (i == cal->length - 1) {
      strcat(dest, "]\n");
    }
  }

  for (int i = 0; i < cal->length; ++i) {

    if (i == 0) {
      toml_cat_bare_key(dest, CAL_RANGE_MINIMA_KEY);
      strcat(dest, "[ ");
    }

    struct grid_cal_limits* limits;
    int status = grid_cal_get(cal, i, GRID_CAL_LIMITS, (void**)&limits);
    if (status) {
      return status;
    }

    uint16_t min = grid_cal_limits_min_get(limits);
    snprintf(temp, GRID_CONFIG_MAX_UINT16_ARRAYELEM + 1, "%hu, ", min);
    strcat(dest, temp);

    if (i == cal->length - 1) {
      strcat(dest, "]\n");
    }
  }

  for (int i = 0; i < cal->length; ++i) {

    if (i == 0) {
      toml_cat_bare_key(dest, CAL_RANGE_MAXIMA_KEY);
      strcat(dest, "[ ");
    }

    struct grid_cal_limits* limits;
    int status = grid_cal_get(cal, i, GRID_CAL_LIMITS, (void**)&limits);
    if (status) {
      return status;
    }

    uint16_t max = grid_cal_limits_max_get(limits);
    snprintf(temp, GRID_CONFIG_MAX_UINT16_ARRAYELEM + 1, "%hu, ", max);
    strcat(dest, temp);

    if (i == cal->length - 1) {
      strcat(dest, "]\n");
    }
  }

  for (int i = 0; i < cal->length; ++i) {

    if (i == 0) {
      toml_cat_bare_key(dest, CAL_POT_DETENT_LOW_KEY);
      strcat(dest, "[ ");
    }

    struct grid_cal_detent* detent;
    int status = grid_cal_get(cal, i, GRID_CAL_DETENT, (void**)&detent);
    if (status) {
      return status;
    }

    uint16_t det = grid_cal_detent_lo_get(detent);
    snprintf(temp, GRID_CONFIG_MAX_UINT16_ARRAYELEM + 1, "%hu, ", det);
    strcat(dest, temp);

    if (i == cal->length - 1) {
      strcat(dest, "]\n");
    }
  }

  for (int i = 0; i < cal->length; ++i) {

    if (i == 0) {
      toml_cat_bare_key(dest, CAL_POT_DETENT_HIGH_KEY);
      strcat(dest, "[ ");
    }

    struct grid_cal_detent* detent;
    int status = grid_cal_get(cal, i, GRID_CAL_DETENT, (void**)&detent);
    if (status) {
      return status;
    }

    uint16_t det = grid_cal_detent_hi_get(detent);
    snprintf(temp, GRID_CONFIG_MAX_UINT16_ARRAYELEM + 1, "%hu, ", det);
    strcat(dest, temp);

    if (i == cal->length - 1) {
      strcat(dest, "]\n");
    }
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
  bytes += cal->length * GRID_CONFIG_MAX_UINT16_ARRAYELEM;
  bytes += strlen("]\n");

  bytes += strlen(CAL_RANGE_MINIMA_KEY " = ");
  bytes += strlen("[ ");
  bytes += cal->length * GRID_CONFIG_MAX_UINT16_ARRAYELEM;
  bytes += strlen("]\n");

  bytes += strlen(CAL_RANGE_MAXIMA_KEY " = ");
  bytes += strlen("[ ");
  bytes += cal->length * GRID_CONFIG_MAX_UINT16_ARRAYELEM;
  bytes += strlen("]\n");

  bytes += strlen(CAL_POT_DETENT_LOW_KEY " = ");
  bytes += strlen("[ ");
  bytes += cal->length * GRID_CONFIG_MAX_UINT16_ARRAYELEM;
  bytes += strlen("]\n");

  bytes += strlen(CAL_POT_DETENT_HIGH_KEY " = ");
  bytes += strlen("[ ");
  bytes += cal->length * GRID_CONFIG_MAX_UINT16_ARRAYELEM;
  bytes += strlen("]\n");

  return bytes;
}

uint32_t grid_config_bytes(struct grid_config_model* config) {

  uint32_t bytes = 0;

  bytes += grid_config_bytes_cal(config->cal);
  bytes += 1; // \0

  return bytes;
}
