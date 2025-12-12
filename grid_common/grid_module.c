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

extern struct luaL_Reg* grid_lua_api_gui_lib_reference;

void grid_lua_ui_init(struct grid_lua_model* lua) {

  struct grid_ui_model* ui = &grid_ui_state;

  // Create the element table in the lua state
  grid_lua_create_element_array(lua->L, ui->element_list_length);

  uint8_t countpertype[GRID_PARAMETER_ELEMENT_COUNT] = {0};

  for (int i = 0; i < ui->element_list_length; ++i) {

    struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, i);
    assert(ele);

    const char* type = NULL;
    const char* metainit = NULL;
    const luaL_Reg* indexmeta = NULL;

    // clang-format off
    #define GRID_LUA_UI_INIT_ASSIGN(t) \
      type = t##_TYPE; \
      metainit = t##_META_init; \
      indexmeta = t##_INDEX_META;
    // clang-format on

    switch (ele->type) {
    case GRID_PARAMETER_ELEMENT_SYSTEM: {
      GRID_LUA_UI_INIT_ASSIGN(GRID_LUA_S);
    } break;
    case GRID_PARAMETER_ELEMENT_POTMETER: {
      GRID_LUA_UI_INIT_ASSIGN(GRID_LUA_P);
    } break;
    case GRID_PARAMETER_ELEMENT_BUTTON: {
      GRID_LUA_UI_INIT_ASSIGN(GRID_LUA_B);
    } break;
    case GRID_PARAMETER_ELEMENT_ENCODER: {
      GRID_LUA_UI_INIT_ASSIGN(GRID_LUA_E);
    } break;
    case GRID_PARAMETER_ELEMENT_ENDLESS: {
      GRID_LUA_UI_INIT_ASSIGN(GRID_LUA_EP);
    } break;
    case GRID_PARAMETER_ELEMENT_LCD: {
      GRID_LUA_UI_INIT_ASSIGN(GRID_LUA_L);
    } break;
    default:
      assert(0);
      break;
    }
    assert(type);

    if (countpertype[ele->type] == 0) {

      // Initialize element type metatable and expand its __index
      grid_lua_dostring_unsafe(lua, metainit);
      grid_lua_register_index_meta_for_type(lua->L, type, indexmeta);

      if (ele->type == GRID_PARAMETER_ELEMENT_LCD) {
        grid_lua_register_functions_unsafe(lua, grid_lua_api_gui_lib_reference);
      }
    }

    ++countpertype[ele->type];

    // Initialize element entry and assign the metatable for its type
    grid_lua_register_element(lua->L, i);
    grid_lua_register_index_meta_for_element(lua->L, i, type);
  }
}

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

  ui->lua_ui_init_callback = grid_lua_ui_init;
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

  ui->lua_ui_init_callback = grid_lua_ui_init;
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

  ui->lua_ui_init_callback = grid_lua_ui_init;
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

  ui->lua_ui_init_callback = grid_lua_ui_init;
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

  ui->lua_ui_init_callback = grid_lua_ui_init;
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

  ui->lua_ui_init_callback = grid_lua_ui_init;
}
