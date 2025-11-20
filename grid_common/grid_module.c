#include "grid_module.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "grid_lua_api.h"
#include "grid_protocol.h"
#include "grid_ui_button.h"
#include "grid_ui_encoder.h"
#include "grid_ui_endless.h"
#include "grid_ui_lcd.h"
#include "grid_ui_potmeter.h"
#include "grid_ui_system.h"

/* ====================  MODULE SPECIFIC INITIALIZERS  ====================*/

void grid_lua_ui_init_po16(struct grid_lua_model* lua);
void grid_lua_ui_init_bu16(struct grid_lua_model* lua);
void grid_lua_ui_init_pbf4(struct grid_lua_model* lua);
void grid_lua_ui_init_en16(struct grid_lua_model* lua);
void grid_lua_ui_init_ef44(struct grid_lua_model* lua);
void grid_lua_ui_init_tek2(struct grid_lua_model* lua);
void grid_lua_ui_init_tek1(struct grid_lua_model* lua);
void grid_lua_ui_init_vsn2(struct grid_lua_model* lua);
void grid_lua_ui_init_pb44(struct grid_lua_model* lua);

void grid_module_po16_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui) {

  // 16 pot, depth of 5, 14bit internal, 7bit result;
  grid_ain_init(ain, 16, 4);
  grid_led_init(led, 16);
  grid_led_lookup_alloc_identity(led, 0, 16);

  grid_ui_model_init(ui, 16 + 1); // +1 for the system element

  for (uint8_t j = 0; j < 16 + 1; j++) {

    struct grid_ui_element* ele = grid_ui_element_model_init(ui, j);

    if (j < 16) {
      grid_ui_element_potmeter_init(ele);
    } else {
      grid_ui_element_system_init(ele);
    }
  }

  ui->lua_ui_init_callback = grid_lua_ui_init_po16;
}
void grid_module_bu16_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui) {

  // 16 pot, depth of 5, 14bit internal, 7bit result;
  grid_ain_init(ain, 16, 4);
  grid_led_init(led, 16);
  grid_led_lookup_alloc_identity(led, 0, 16);

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
  grid_ain_init(ain, 16, 4);
  grid_led_init(led, 12);
  grid_led_lookup_alloc_identity(led, 0, 12);

  grid_ui_model_init(ui, 12 + 1); // +1 for the system element

  for (uint8_t j = 0; j < 12 + 1; j++) {

    struct grid_ui_element* ele = grid_ui_element_model_init(ui, j);

    if (j < 8) {
      // potmeter or fader
      grid_ui_element_potmeter_init(ele);

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
  grid_ain_init(ain, 16, 4);
  grid_led_init(led, 16);
  grid_led_lookup_alloc_identity(led, 0, 16);

  grid_ui_model_init(ui, 16 + 1); // +1 for the system element

  for (uint8_t j = 0; j < 16 + 1; j++) {

    struct grid_ui_element* ele = grid_ui_element_model_init(ui, j);

    if (j < 8) {

      grid_ui_element_potmeter_init(ele);

    } else if (j < 16) {

      grid_ui_element_button_init(ele);

    } else {
      grid_ui_element_system_init(ele);
    }
  }

  ui->lua_ui_init_callback = grid_lua_ui_init_pb44;
}

void grid_module_ef44_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui) {

  // TODO should be 4 ain channels but indexing is bad in grid_ui_potmeter.c
  grid_ain_init(&grid_ain_state, 8, 4);
  grid_led_init(&grid_led_state, 8);
  grid_led_lookup_alloc_identity(led, 0, 8);

  grid_ui_model_init(ui, 8 + 1); // +1 for the system element

  for (uint8_t j = 0; j < 8 + 1; j++) {

    struct grid_ui_element* ele = grid_ui_element_model_init(ui, j);

    if (j < 4) {
      grid_ui_element_encoder_init(ele);

    } else if (j < 8) {
      // fader
      grid_ui_element_potmeter_init(ele);

    } else {

      grid_ui_element_system_init(ele);
    }
  }

  ui->lua_ui_init_callback = grid_lua_ui_init_ef44;
}

void grid_module_tek2_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui) {

  // 16 pot, depth of 5, 14bit internal, 7bit result;
  grid_ain_init(ain, 16, 4); // TODO: 12 ain for TEK2
  grid_led_init(led, 18);    // TODO: 18 led for TEK2

  for (uint8_t i = 0; i < 8; ++i) {
    grid_led_lookup_alloc_single(led, i, i + 10);
  }
  grid_led_lookup_alloc_multi(led, 8, 5, (uint8_t[5]){0, 1, 2, 3, 4});
  grid_led_lookup_alloc_multi(led, 9, 5, (uint8_t[5]){5, 6, 7, 8, 9});

  grid_ui_model_init(ui, 10 + 1); // 10+1 for the system element on TEK2

  for (uint8_t j = 0; j < 10 + 1; j++) {

    struct grid_ui_element* ele = grid_ui_element_model_init(ui, j);

    if (j < 8) {

      grid_ui_element_button_init(ele);

    } else if (j < 10) {

      grid_ui_element_endless_init(ele);

    } else {
      grid_ui_element_system_init(ele);
    }
  }

  ui->lua_ui_init_callback = grid_lua_ui_init_tek2;
}

void grid_module_en16_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui) {

  grid_led_init(&grid_led_state, 16);
  grid_led_lookup_alloc_identity(led, 0, 16);

  grid_ui_model_init(ui, 16 + 1); // +1 for the system element

  for (uint8_t j = 0; j < 16 + 1; j++) {

    struct grid_ui_element* ele = grid_ui_element_model_init(ui, j);

    if (j < 16) {

      grid_ui_element_encoder_init(ele);
    } else {

      grid_ui_element_system_init(ele);
    }
  }

  ui->lua_ui_init_callback = grid_lua_ui_init_en16;
}

void grid_lua_ui_init_po16(struct grid_lua_model* lua) {

  grid_lua_create_element_array(lua->L, 17);

  GRID_LUA_UI_INIT_ELEMENTTYPE_META(lua, GRID_LUA_P);
  for (int i = 0; i < 16; ++i) {
    GRID_LUA_UI_INIT_ELEMENT(lua, i, GRID_LUA_P);
  }

  GRID_LUA_UI_INIT_ELEMENTTYPE_META(lua, GRID_LUA_S);
  GRID_LUA_UI_INIT_ELEMENT(lua, 16, GRID_LUA_S);
}

void grid_lua_ui_init_bu16(struct grid_lua_model* lua) {

  grid_lua_create_element_array(lua->L, 17);

  GRID_LUA_UI_INIT_ELEMENTTYPE_META(lua, GRID_LUA_B);
  for (int i = 0; i < 16; ++i) {
    GRID_LUA_UI_INIT_ELEMENT(lua, i, GRID_LUA_B);
  }

  GRID_LUA_UI_INIT_ELEMENTTYPE_META(lua, GRID_LUA_S);
  GRID_LUA_UI_INIT_ELEMENT(lua, 16, GRID_LUA_S);
}

void grid_lua_ui_init_pbf4(struct grid_lua_model* lua) {

  grid_lua_create_element_array(lua->L, 13);

  GRID_LUA_UI_INIT_ELEMENTTYPE_META(lua, GRID_LUA_P);
  for (int i = 0; i < 8; ++i) {
    GRID_LUA_UI_INIT_ELEMENT(lua, i, GRID_LUA_P);
  }

  GRID_LUA_UI_INIT_ELEMENTTYPE_META(lua, GRID_LUA_B);
  for (int i = 8; i < 12; ++i) {
    GRID_LUA_UI_INIT_ELEMENT(lua, i, GRID_LUA_B);
  }

  GRID_LUA_UI_INIT_ELEMENTTYPE_META(lua, GRID_LUA_S);
  GRID_LUA_UI_INIT_ELEMENT(lua, 12, GRID_LUA_S);
}

void grid_lua_ui_init_pb44(struct grid_lua_model* lua) {

  grid_lua_create_element_array(lua->L, 17);

  GRID_LUA_UI_INIT_ELEMENTTYPE_META(lua, GRID_LUA_P);
  for (int i = 0; i < 8; ++i) {
    GRID_LUA_UI_INIT_ELEMENT(lua, i, GRID_LUA_P);
  }

  GRID_LUA_UI_INIT_ELEMENTTYPE_META(lua, GRID_LUA_B);
  for (int i = 8; i < 16; ++i) {
    GRID_LUA_UI_INIT_ELEMENT(lua, i, GRID_LUA_B);
  }

  GRID_LUA_UI_INIT_ELEMENTTYPE_META(lua, GRID_LUA_S);
  GRID_LUA_UI_INIT_ELEMENT(lua, 16, GRID_LUA_S);
}

void grid_lua_ui_init_en16(struct grid_lua_model* lua) {

  grid_lua_create_element_array(lua->L, 17);

  GRID_LUA_UI_INIT_ELEMENTTYPE_META(lua, GRID_LUA_E);
  for (int i = 0; i < 16; ++i) {
    GRID_LUA_UI_INIT_ELEMENT(lua, i, GRID_LUA_E);
  }

  GRID_LUA_UI_INIT_ELEMENTTYPE_META(lua, GRID_LUA_S);
  GRID_LUA_UI_INIT_ELEMENT(lua, 16, GRID_LUA_S);
}

void grid_lua_ui_init_ef44(struct grid_lua_model* lua) {

  grid_lua_create_element_array(lua->L, 9);

  GRID_LUA_UI_INIT_ELEMENTTYPE_META(lua, GRID_LUA_E);
  for (int i = 0; i < 4; ++i) {
    GRID_LUA_UI_INIT_ELEMENT(lua, i, GRID_LUA_E);
  }

  GRID_LUA_UI_INIT_ELEMENTTYPE_META(lua, GRID_LUA_P);
  for (int i = 4; i < 8; ++i) {
    GRID_LUA_UI_INIT_ELEMENT(lua, i, GRID_LUA_P);
  }

  GRID_LUA_UI_INIT_ELEMENTTYPE_META(lua, GRID_LUA_S);
  GRID_LUA_UI_INIT_ELEMENT(lua, 8, GRID_LUA_S);
}

void grid_lua_ui_init_tek2(struct grid_lua_model* lua) {

  grid_lua_create_element_array(lua->L, 11);

  GRID_LUA_UI_INIT_ELEMENTTYPE_META(lua, GRID_LUA_B);
  for (int i = 0; i < 8; ++i) {
    GRID_LUA_UI_INIT_ELEMENT(lua, i, GRID_LUA_B);
  }

  GRID_LUA_UI_INIT_ELEMENTTYPE_META(lua, GRID_LUA_EP);
  for (int i = 8; i < 10; ++i) {
    GRID_LUA_UI_INIT_ELEMENT(lua, i, GRID_LUA_EP);
  }

  GRID_LUA_UI_INIT_ELEMENTTYPE_META(lua, GRID_LUA_S);
  GRID_LUA_UI_INIT_ELEMENT(lua, 10, GRID_LUA_S);
}
