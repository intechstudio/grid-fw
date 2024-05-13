#include "grid_module.h"

#include "grid_ui.h"

#include "grid_ui_button.h"
#include "grid_ui_encoder.h"
#include "grid_ui_endless.h"
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
void grid_lua_ui_init_pb44(struct grid_lua_model* lua);

void grid_module_po16_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui) {

  // 16 pot, depth of 5, 14bit internal, 7bit result;
  grid_ain_init(ain, 16, 5);
  grid_led_init(led, 16);

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
  grid_ain_init(ain, 16, 5);
  grid_led_init(led, 16);

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
      grid_ui_element_potmeter_init(ele);

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

      grid_ui_element_endless_init(ele);

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

      grid_ui_element_system_init(ele);
    }
  }

  ui->lua_ui_init_callback = grid_lua_ui_init_en16;
}

void grid_lua_ui_init_po16(struct grid_lua_model* lua) {

  // define encoder_init_function

  grid_lua_dostring(lua, GRID_LUA_P_META_init);

  // create element array
  grid_lua_dostring(lua, GRID_LUA_KW_ELEMENT_short "= {} ");

  // initialize 16 potmeter
  grid_lua_dostring(lua, "for i=0, 15 do " GRID_LUA_KW_ELEMENT_short "[i] = {index = i} end");
  grid_lua_dostring(lua, "for i=0, 15 do setmetatable(" GRID_LUA_KW_ELEMENT_short "[i], potmeter_meta) end");

  grid_lua_gc_try_collect(lua);

  // initialize the system element
  grid_lua_dostring(lua, GRID_LUA_KW_ELEMENT_short "[16] = {index = 16}");
  grid_lua_dostring(lua, GRID_LUA_SYS_META_init);
  grid_lua_dostring(lua, "setmetatable(" GRID_LUA_KW_ELEMENT_short "[16], system_meta)");
}

void grid_lua_ui_init_bu16(struct grid_lua_model* lua) {

  // define encoder_init_function

  grid_lua_dostring(lua, GRID_LUA_B_META_init);

  // create element array
  grid_lua_dostring(lua, GRID_LUA_KW_ELEMENT_short "= {} ");

  // initialize 16 buttons
  grid_lua_dostring(lua, "for i=0, 15 do " GRID_LUA_KW_ELEMENT_short "[i] = {index = i} end");
  grid_lua_dostring(lua, "for i=0, 15 do setmetatable(" GRID_LUA_KW_ELEMENT_short "[i], button_meta) end");

  grid_lua_gc_try_collect(lua);

  // initialize the system element
  grid_lua_dostring(lua, GRID_LUA_KW_ELEMENT_short "[16] = {index = 16}");
  grid_lua_dostring(lua, GRID_LUA_SYS_META_init);
  grid_lua_dostring(lua, "setmetatable(" GRID_LUA_KW_ELEMENT_short "[16], system_meta)");
}

void grid_lua_ui_init_pbf4(struct grid_lua_model* lua) {

  // define encoder_init_function

  grid_lua_dostring(lua, GRID_LUA_P_META_init);
  grid_lua_dostring(lua, GRID_LUA_B_META_init);

  // create element array
  grid_lua_dostring(lua, GRID_LUA_KW_ELEMENT_short "= {} ");

  // initialize 8 potmeters and 8 buttons
  grid_lua_dostring(lua, "for i=0, 7  do " GRID_LUA_KW_ELEMENT_short "[i] = {index = i} end");
  grid_lua_dostring(lua, "for i=0, 7  do  setmetatable(" GRID_LUA_KW_ELEMENT_short "[i], potmeter_meta)  end");

  grid_lua_dostring(lua, "for i=8, 11 do " GRID_LUA_KW_ELEMENT_short "[i] = {index = i} end");
  grid_lua_dostring(lua, "for i=8, 11 do  setmetatable(" GRID_LUA_KW_ELEMENT_short "[i], button_meta)  end");

  grid_lua_gc_try_collect(lua);

  // initialize the system element
  grid_lua_dostring(lua, GRID_LUA_KW_ELEMENT_short "[12] = {index = 12}");
  grid_lua_dostring(lua, GRID_LUA_SYS_META_init);
  grid_lua_dostring(lua, "setmetatable(" GRID_LUA_KW_ELEMENT_short "[12], system_meta)");
}

void grid_lua_ui_init_pb44(struct grid_lua_model* lua) {

  // define encoder_init_function

  grid_lua_dostring(lua, GRID_LUA_P_META_init);
  grid_lua_dostring(lua, GRID_LUA_B_META_init);

  // create element array
  grid_lua_dostring(lua, GRID_LUA_KW_ELEMENT_short "= {} ");

  // initialize 8 potmeters and 8 buttons
  grid_lua_dostring(lua, "for i=0, 7  do " GRID_LUA_KW_ELEMENT_short "[i] = {index = i} end");
  grid_lua_dostring(lua, "for i=0, 7  do  setmetatable(" GRID_LUA_KW_ELEMENT_short "[i], potmeter_meta)  end");

  grid_lua_dostring(lua, "for i=8, 15 do " GRID_LUA_KW_ELEMENT_short "[i] = {index = i} end");
  grid_lua_dostring(lua, "for i=8, 15 do  setmetatable(" GRID_LUA_KW_ELEMENT_short "[i], button_meta)  end");

  grid_lua_gc_try_collect(lua);

  // initialize the system element
  grid_lua_dostring(lua, GRID_LUA_KW_ELEMENT_short "[16] = {index = 16}");
  grid_lua_dostring(lua, GRID_LUA_SYS_META_init);
  grid_lua_dostring(lua, "setmetatable(" GRID_LUA_KW_ELEMENT_short "[16], system_meta)");
}

void grid_lua_ui_init_en16(struct grid_lua_model* lua) {

  // define encoder_init_function

  grid_lua_dostring(lua, GRID_LUA_E_META_init);

  // create element array
  grid_lua_dostring(lua, GRID_LUA_KW_ELEMENT_short "= {} ");

  // initialize 16 encoders
  grid_lua_dostring(lua, "for i=0, 15 do " GRID_LUA_KW_ELEMENT_short "[i] = {index = i} end");
  grid_lua_dostring(lua, "for i=0, 15 do setmetatable(" GRID_LUA_KW_ELEMENT_short "[i], encoder_meta) end");

  grid_lua_gc_try_collect(lua);

  // initialize the system element
  grid_lua_dostring(lua, GRID_LUA_KW_ELEMENT_short "[16] = {index = 16}");
  grid_lua_dostring(lua, GRID_LUA_SYS_META_init);
  grid_lua_dostring(lua, "setmetatable(" GRID_LUA_KW_ELEMENT_short "[16], system_meta)");
}

void grid_lua_ui_init_ef44(struct grid_lua_model* lua) {
  // define encoder_init_function

  grid_lua_dostring(lua, GRID_LUA_E_META_init);
  grid_lua_dostring(lua, GRID_LUA_P_META_init);

  // create element array
  grid_lua_dostring(lua, GRID_LUA_KW_ELEMENT_short "= {} ");

  // initialize 4 encoders and 4 faders
  grid_lua_dostring(lua, "for i=0, 3  do " GRID_LUA_KW_ELEMENT_short "[i] = {index = i} end");
  grid_lua_dostring(lua, "for i=0, 3  do  setmetatable(" GRID_LUA_KW_ELEMENT_short "[i], encoder_meta)  end");

  grid_lua_dostring(lua, "for i=4, 7 do " GRID_LUA_KW_ELEMENT_short "[i] = {index = i} end");
  grid_lua_dostring(lua, "for i=4, 7 do  setmetatable(" GRID_LUA_KW_ELEMENT_short "[i], potmeter_meta)  end");

  grid_lua_gc_try_collect(lua);

  // initialize the system element
  grid_lua_dostring(lua, GRID_LUA_KW_ELEMENT_short "[8] = {index = 8}");
  grid_lua_dostring(lua, GRID_LUA_SYS_META_init);
  grid_lua_dostring(lua, "setmetatable(" GRID_LUA_KW_ELEMENT_short "[8], system_meta)");
}

void grid_lua_ui_init_tek2(struct grid_lua_model* lua) {

  // define encoder_init_function

  grid_lua_dostring(lua, GRID_LUA_B_META_init);
  grid_lua_dostring(lua, GRID_LUA_EP_META_init);

  // create element array
  grid_lua_dostring(lua, GRID_LUA_KW_ELEMENT_short "= {} ");

  // initialize 8 buttons
  grid_lua_dostring(lua, "for i=0, 7 do " GRID_LUA_KW_ELEMENT_short "[i] = {index = i} end");
  grid_lua_dostring(lua, "for i=0, 7 do setmetatable(" GRID_LUA_KW_ELEMENT_short "[i], button_meta) end");

  // initialize 2 endless potentiometers as encoders
  grid_lua_dostring(lua, "for i=8, 9  do " GRID_LUA_KW_ELEMENT_short "[i] = {index = i} end");
  grid_lua_dostring(lua, "for i=8, 9  do  setmetatable(" GRID_LUA_KW_ELEMENT_short "[i], endless_meta)  end");

  grid_lua_gc_try_collect(lua);

  // initialize the system element
  grid_lua_dostring(lua, GRID_LUA_KW_ELEMENT_short "[10] = {index = 10}");
  grid_lua_dostring(lua, GRID_LUA_SYS_META_init);
  grid_lua_dostring(lua, "setmetatable(" GRID_LUA_KW_ELEMENT_short "[10], system_meta)");
}

void grid_lua_ui_init_tek1(struct grid_lua_model* lua) {

  // define encoder_init_function

  grid_lua_dostring(lua, GRID_LUA_B_META_init);
  grid_lua_dostring(lua, GRID_LUA_EP_META_init);

  // create element array
  grid_lua_dostring(lua, GRID_LUA_KW_ELEMENT_short "= {} ");

  // initialize 8 buttons
  grid_lua_dostring(lua, "for i=0, 7 do " GRID_LUA_KW_ELEMENT_short "[i] = {index = i} end");
  grid_lua_dostring(lua, "for i=0, 7 do setmetatable(" GRID_LUA_KW_ELEMENT_short "[i], button_meta) end");

  // initialize 2 endless potentiometers as encoders
  grid_lua_dostring(lua, "for i=8, 8 do " GRID_LUA_KW_ELEMENT_short "[i] = {index = i} end");
  grid_lua_dostring(lua, "for i=8, 8 do  setmetatable(" GRID_LUA_KW_ELEMENT_short "[i], endless_meta)  end");

  // initialize 8 buttons
  grid_lua_dostring(lua, "for i=9, 13 do " GRID_LUA_KW_ELEMENT_short "[i] = {index = i} end");
  grid_lua_dostring(lua, "for i=9, 13 do setmetatable(" GRID_LUA_KW_ELEMENT_short "[i], button_meta) end");

  grid_lua_gc_try_collect(lua);

  // initialize the system element
  grid_lua_dostring(lua, GRID_LUA_KW_ELEMENT_short "[10] = {index = 10}");
  grid_lua_dostring(lua, GRID_LUA_SYS_META_init);
  grid_lua_dostring(lua, "setmetatable(" GRID_LUA_KW_ELEMENT_short "[10], system_meta)");
}
