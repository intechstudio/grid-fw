#include "grid_module.h"

#include "grid_ui.h"

#include "grid_ui_button.h"
#include "grid_ui_encoder.h"
#include "grid_ui_endless.h"
#include "grid_ui_potmeter.h"
#include "grid_ui_system.h"

void grid_module_po16_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui) {

  // 16 pot, depth of 5, 14bit internal, 7bit result;
  grid_ain_init(ain, 16, 5);
  grid_led_init(led, 16);

  grid_ui_model_init(ui, 16 + 1); // +1 for the system element

  for (uint8_t j = 0; j < 16 + 1; j++) {

    struct grid_ui_element* ele = grid_ui_element_model_init(ui, j);

    if (j < 16) {
      grid_ui_element_potentiometer_init(ele);
    } else {
      grid_ui_element_system_init(ele);
    }
  }

  ui->lua_ui_init_callback = grid_lua_ui_init_po16;
}
void grid_module_bu16_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui) {

  // 16 pot, depth of 5, 14bit internal, 7bit result;
  grid_ain_init(ain, 16, 5);
  grid_led_init(led, 16);

  grid_ui_model_init(ui, 16 + 1); // +1 for the system element

  for (uint8_t j = 0; j < 16 + 1; j++) {

    struct grid_ui_element* ele = grid_ui_element_model_init(ui, j);

    if (j < 16) {

      grid_ui_element_button_init(ele);

    } else {

      grid_ui_element_system_init(ele);
    }
  }

  ui->lua_ui_init_callback = grid_lua_ui_init_bu16;
}

void grid_module_pbf4_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui) {

  // 16 pot, depth of 5, 14bit internal, 7bit result;
  grid_ain_init(ain, 16, 5);
  grid_led_init(led, 12);

  grid_ui_model_init(ui, 12 + 1); // +1 for the system element

  for (uint8_t j = 0; j < 12 + 1; j++) {

    struct grid_ui_element* ele = grid_ui_element_model_init(ui, j);

    if (j < 8) {
      // potmeter or fader
      grid_ui_element_potentiometer_init(ele);

    } else if (j < 12) {

      grid_ui_element_button_init(ele);

    } else {

      grid_ui_element_system_init(ele);
    }
  }

  ui->lua_ui_init_callback = grid_lua_ui_init_pbf4;
}

void grid_module_pb44_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui) {

  // 16 pot, depth of 5, 14bit internal, 7bit result;
  grid_ain_init(ain, 16, 5);
  grid_led_init(led, 16);

  grid_ui_model_init(ui, 16 + 1); // +1 for the system element

  for (uint8_t j = 0; j < 16 + 1; j++) {

    struct grid_ui_element* ele = grid_ui_element_model_init(ui, j);

    if (j < 8) {

      grid_ui_element_potentiometer_init(ele);

    } else if (j < 16) {

      grid_ui_element_button_init(ele);

    } else {
      grid_ui_element_system_init(ele);
    }
  }

  ui->lua_ui_init_callback = grid_lua_ui_init_pb44;
}

void grid_module_ef44_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui) {

  // should be 4 but indexing is bad at
  // grid_element_potmeter_template_parameter_init
  grid_ain_init(&grid_ain_state, 8, 5);

  grid_led_init(&grid_led_state, 8);

  grid_ui_model_init(ui, 8 + 1); // +1 for the system element

  for (uint8_t j = 0; j < 8 + 1; j++) {

    struct grid_ui_element* ele = grid_ui_element_model_init(ui, j);

    if (j < 4) {
      grid_ui_element_encoder_init(ele);

    } else if (j < 8) {
      // fader
      grid_ui_element_potentiometer_init(ele);

    } else {

      grid_ui_element_system_init(ele);
    }
  }

  ui->lua_ui_init_callback = grid_lua_ui_init_ef44;
}

void grid_module_tek2_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui) {

  // 16 pot, depth of 5, 14bit internal, 7bit result;
  grid_ain_init(ain, 16, 5); // TODO: 12 ain for TEK2
  grid_led_init(led, 18);    // TODO: 18 led for TEK2

  uint8_t led_lookup[18] = {10, 11, 12, 13, 14, 15, 16, 17, 0, 5, 1, 6, 2, 7, 3, 8, 4, 9};

  grid_led_lookup_init(led, led_lookup); // initialize the optional led index
                                         // lookup table for array remapping

  grid_ui_model_init(ui, 10 + 1); // 10+1 for the system element on TEK2

  for (uint8_t j = 0; j < 10 + 1; j++) {

    struct grid_ui_element* ele = grid_ui_element_model_init(ui, j);

    if (j < 8) {

      grid_ui_element_button_init(ele);

    } else if (j < 10) {

      grid_ui_element_endlesspot_init(ele);

    } else {
      grid_ui_element_system_init(ele);
    }
  }

  ui->lua_ui_init_callback = grid_lua_ui_init_tek2;
}

void grid_module_en16_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui) {

  grid_led_init(&grid_led_state, 16);

  grid_ui_model_init(ui, 16 + 1); // +1 for the system element

  for (uint8_t j = 0; j < 16 + 1; j++) {

    struct grid_ui_element* ele = grid_ui_element_model_init(ui, j);

    if (j < 16) {

      grid_ui_element_encoder_init(ele);
    } else {

      grid_ui_element_encoder_init(ele);
    }
  }

  ui->lua_ui_init_callback = grid_lua_ui_init_en16;
}
