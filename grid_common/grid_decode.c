#include "grid_decode.h"

#include <stdarg.h>
#include <string.h>

#include "grid_led.h"
#include "grid_lua_api.h"
#include "grid_msg.h"
#include "grid_platform.h"
#include "grid_sys.h"
#include "grid_ui_encoder.h"
#include "grid_ui_potmeter.h"
#include "grid_usb.h"

extern struct grid_transport grid_transport_state;

enum GRID_DESTINATION {

  GRID_DESTINATION_IS_ME = 1,
  GRID_DESTINATION_IS_GLOBAL = 2,
  GRID_DESTINATION_IS_LOCAL = 4,
};

static uint8_t grid_check_destination(char* header, uint8_t target_destination_bm) {

  uint8_t dx = grid_msg_get_parameter_raw((uint8_t*)header, BRC_DX);
  uint8_t dy = grid_msg_get_parameter_raw((uint8_t*)header, BRC_DY);

  uint8_t header_destination_bm = 0;

  if (dx == GRID_PARAMETER_DEFAULT_POSITION && dy == GRID_PARAMETER_DEFAULT_POSITION) {
    header_destination_bm |= GRID_DESTINATION_IS_ME;
  } else if (dx == GRID_PARAMETER_GLOBAL_POSITION && dy == GRID_PARAMETER_GLOBAL_POSITION) {
    header_destination_bm |= GRID_DESTINATION_IS_GLOBAL;
  } else if (dx == GRID_PARAMETER_LOCAL_POSITION && dy == GRID_PARAMETER_LOCAL_POSITION) {
    header_destination_bm |= GRID_DESTINATION_IS_LOCAL;
  }

  return header_destination_bm & target_destination_bm;
}

uint8_t grid_decode_midi_to_usb(char* header, char* chunk) {

  uint8_t instr = grid_msg_get_parameter_raw((uint8_t*)chunk, INSTR);

  if (instr != GRID_INSTR_EXECUTE_code) {
    return 1;
  }

  uint8_t channel = grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_MIDI_CHANNEL);
  uint8_t command = grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_MIDI_COMMAND);
  uint8_t param1 = grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_MIDI_PARAM1);
  uint8_t param2 = grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_MIDI_PARAM2);

  struct grid_midi_event_desc event = {
      .byte0 = 0 << 4 | command >> 4,
      .byte1 = command | channel,
      .byte2 = param1,
      .byte3 = param2,
  };

  grid_midi_tx_push(event);

  return 0;
}

uint8_t grid_decode_sysex_to_usb(char* header, char* chunk) {

  uint8_t instr = grid_msg_get_parameter_raw((uint8_t*)chunk, INSTR);

  if (instr != GRID_INSTR_EXECUTE_code) {
    return 1;
  }

  uint16_t length = grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_MIDISYSEX_LENGTH);

  uint16_t off = GRID_CLASS_MIDISYSEX_PAYLOAD_offset;
  uint8_t len = GRID_CLASS_MIDISYSEX_PAYLOAD_length;
  uint8_t first = grid_frame_get_parameter((uint8_t*)chunk, off, len);
  uint8_t last = grid_frame_get_parameter((uint8_t*)chunk, off + (length - 1) * 2, len);

  if (first != 0xf0 || last != 0xf7) {
    grid_port_debug_printf("sysex invalid: %d %d", first, last);
  }

  uint32_t packets_dropped = 0;

  struct grid_midi_event_desc event;
  for (uint16_t i = 0; i < length;) {

    memset(&event, 0, sizeof(struct grid_midi_event_desc));

    event.byte1 = grid_frame_get_parameter((uint8_t*)chunk, off + (i++) * 2, len);
    if (i < length) {
      event.byte2 = grid_frame_get_parameter((uint8_t*)chunk, off + (i++) * 2, len);
    }
    if (i < length) {
      event.byte3 = grid_frame_get_parameter((uint8_t*)chunk, off + (i++) * 2, len);
    }

    if (length < 4) { // shortsysex
      if (length == 2) {
        event.byte0 = 0 << 4 | 6;
      }
      if (length == 3) {
        event.byte0 = 0 << 4 | 7;
      }
    } else if (i < 4) { // first eventpacket of longsysex
      event.byte0 = 0 << 4 | 4;
    } else {            // how many useful bytes are in this eventpacket
      if (i % 3 == 0) { // 3
        event.byte0 = 0 << 4 | 7;
      } else if (i % 3 == 1) { // 1
        event.byte0 = 0 << 4 | 5;
      } else if (i % 3 == 2) { // 2
        event.byte0 = 0 << 4 | 6;
      }
    }

    // grid_port_debug_printf("packet: %d %d %d %d", event.byte0, event.byte1, event.byte2, event.byte3);
    packets_dropped += grid_midi_tx_push(event);
  }

  if (packets_dropped) {
    grid_port_debug_printf("sysex_to_usb: %d packets dropped", packets_dropped);
  }

  return 0;
}

uint8_t grid_decode_mousebutton_to_usb(char* header, char* chunk) {

  uint8_t instr = grid_msg_get_parameter_raw((uint8_t*)chunk, INSTR);

  if (instr != GRID_INSTR_EXECUTE_code) {
    return 1;
  }

  uint8_t state = grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_HIDMOUSEBUTTON_STATE);
  uint8_t button = grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_HIDMOUSEBUTTON_BUTTON);

  struct grid_usb_keyboard_event_desc key = {
      .ismodifier = 3, // 0: no, 1: yes, 2: mousemove, 3: mousebutton, f: delay
      .ispressed = state,
      .keycode = button,
      .delay = 1,
  };

  if (grid_usb_keyboard_tx_push(&grid_usb_keyboard_state, key)) {
    grid_port_debug_printf("mouse button: packet dropped");
  }

  return 0;
}

uint8_t grid_decode_mousemove_to_usb(char* header, char* chunk) {

  uint8_t instr = grid_msg_get_parameter_raw((uint8_t*)chunk, INSTR);

  if (instr != GRID_INSTR_EXECUTE_code) {
    return 1;
  }

  uint8_t position = grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_HIDMOUSEMOVE_POSITION);
  uint8_t axis = grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_HIDMOUSEMOVE_AXIS);

  struct grid_usb_keyboard_event_desc key = {
      .ismodifier = 2, // 0: no, 1: yes, 2: mousemove, 3: mousebutton, f: delay
      .ispressed = position,
      .keycode = axis,
      .delay = 1,
  };

  if (grid_usb_keyboard_tx_push(&grid_usb_keyboard_state, key)) {
    grid_port_debug_printf("mouse move: packet dropped");
  }

  return 0;
}

uint8_t grid_decode_gamepadmove_to_usb(char* header, char* chunk) {

  uint8_t instr = grid_msg_get_parameter_raw((uint8_t*)chunk, INSTR);

  if (instr != GRID_INSTR_EXECUTE_code) {
    return 1;
  }

  uint8_t axis = grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_HIDGAMEPADMOVE_AXIS);
  uint8_t position_raw = grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_HIDGAMEPADMOVE_POSITION);

  int8_t position = position_raw - 128;

  grid_usb_gamepad_axis_move(axis, position);

  return 0;
}

uint8_t grid_decode_gamepadbutton_to_usb(char* header, char* chunk) {

  uint8_t instr = grid_msg_get_parameter_raw((uint8_t*)chunk, INSTR);

  if (instr != GRID_INSTR_EXECUTE_code) {
    return 1;
  }

  uint8_t button = grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_HIDGAMEPADBUTTON_BUTTON);
  uint8_t state = grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_HIDGAMEPADBUTTON_STATE);

  grid_usb_gamepad_button_change(button, state);

  return 0;
}

uint8_t grid_decode_keyboard_to_usb(char* header, char* chunk) {

  uint8_t instr = grid_msg_get_parameter_raw((uint8_t*)chunk, INSTR);

  if (instr != GRID_INSTR_EXECUTE_code) {
    return 1;
  }

  uint8_t length = grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_HIDKEYBOARD_LENGTH);

  uint8_t default_delay = grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_HIDKEYBOARD_DEFAULTDELAY);

  uint32_t packets_dropped = 0;

  for (uint16_t i = 0; i < length * 4; i += 4) {

    uint8_t key_ismodifier = grid_frame_get_parameter((uint8_t*)chunk, GRID_CLASS_HIDKEYBOARD_KEYISMODIFIER_offset + i, GRID_CLASS_HIDKEYBOARD_KEYISMODIFIER_length);
    uint8_t key_state = grid_frame_get_parameter((uint8_t*)chunk, GRID_CLASS_HIDKEYBOARD_KEYSTATE_offset + i, GRID_CLASS_HIDKEYBOARD_KEYSTATE_length);
    uint8_t key_code = grid_frame_get_parameter((uint8_t*)chunk, GRID_CLASS_HIDKEYBOARD_KEYCODE_offset + i, GRID_CLASS_HIDKEYBOARD_KEYCODE_length);

    if (key_ismodifier == 0 || key_ismodifier == 1) {

      struct grid_usb_keyboard_event_desc key = {
          key.keycode = key_code,
          key.ismodifier = key_ismodifier,
          key.ispressed = key_state,
          key.delay = default_delay,
      };

      // Combined press and release
      if (key_state == 2) {

        key.ispressed = 1;
        packets_dropped += grid_usb_keyboard_tx_push(&grid_usb_keyboard_state, key);
        key.ispressed = 0;
        packets_dropped += grid_usb_keyboard_tx_push(&grid_usb_keyboard_state, key);
      }
      // Single press or release
      else {

        packets_dropped += grid_usb_keyboard_tx_push(&grid_usb_keyboard_state, key);
      }

    } else if (key_ismodifier == 0xf) {

      uint16_t delay = grid_frame_get_parameter((uint8_t*)chunk, GRID_CLASS_HIDKEYBOARD_DELAY_offset + i, GRID_CLASS_HIDKEYBOARD_DELAY_length);

      // Special delay event
      struct grid_usb_keyboard_event_desc key = {
          key.ismodifier = key_ismodifier,
          key.ispressed = 0,
          key.keycode = 0,
          key.delay = delay,
      };

      packets_dropped += grid_usb_keyboard_tx_push(&grid_usb_keyboard_state, key);

    } else {

      grid_platform_printf("invalid key_ismodifier parameter %d\n", key_ismodifier);
    }
  }

  if (packets_dropped) {
    grid_port_debug_printf("keyboard: %d packets dropped", packets_dropped);
  }

  return 0;
}

uint8_t grid_decode_pageactive_to_ui(char* header, char* chunk) {

  uint8_t instr = grid_msg_get_parameter_raw((uint8_t*)chunk, INSTR);

  uint8_t page = grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_PAGEACTIVE_PAGENUMBER);

  // The page is already active
  if (grid_ui_page_get_activepage(&grid_ui_state) == page) {
    return 0;
  }

  switch (instr) {

  case GRID_INSTR_EXECUTE_code: {

    // Page change disabled
    if (!grid_ui_state.page_change_enabled) {
      return 0;
    }

    grid_ui_page_load(&grid_ui_state, page);
    grid_sys_set_bank(&grid_sys_state, page);

  } break;
  case GRID_INSTR_REPORT_code: {

    // Page was already negotiated
    if (grid_ui_state.page_negotiated == true) {
      return 0;
    }

    uint8_t sx = grid_msg_get_parameter_raw((uint8_t*)header, BRC_SX);
    uint8_t sy = grid_msg_get_parameter_raw((uint8_t*)header, BRC_SY);

    // The report originates from this module
    if (sx == GRID_PARAMETER_DEFAULT_POSITION && sy == GRID_PARAMETER_DEFAULT_POSITION) {
      return 0;
    }

    // Page negotiated, disable feature from now on
    grid_ui_state.page_negotiated = true;

    grid_ui_page_load(&grid_ui_state, page);
    grid_sys_set_bank(&grid_sys_state, page);

  } break;
  }

  return 0;
}

uint8_t grid_decode_pagecount_to_ui(char* header, char* chunk) {

  if (grid_check_destination(header, GRID_DESTINATION_IS_ME | GRID_DESTINATION_IS_GLOBAL) == false) {
    return 1;
  }

  uint8_t instr = grid_msg_get_parameter_raw((uint8_t*)chunk, INSTR);

  if (instr != GRID_INSTR_FETCH_code) {
    return 1;
  }

  struct grid_msg msg;
  uint8_t xy = GRID_PARAMETER_GLOBAL_POSITION;
  grid_msg_init_brc(&grid_msg_state, &msg, xy, xy);

  grid_msg_add_frame(&msg, GRID_CLASS_PAGECOUNT_frame);
  grid_msg_set_parameter(&msg, INSTR, GRID_INSTR_REPORT_code);
  grid_msg_set_parameter(&msg, CLASS_PAGECOUNT_PAGENUMBER, grid_ui_state.page_count);

  if (grid_msg_close_brc(&grid_msg_state, &msg) >= 0) {
    grid_transport_send_msg_to_all(&grid_transport_state, &msg);
  }

  return 0;
}

uint8_t grid_decode_midi_to_ui(char* header, char* chunk) {

  // return 1;

  int ret = 1;

  grid_lua_semaphore_lock(&grid_lua_state);

  lua_State* L = grid_lua_state.L;

  lua_getglobal(L, GRID_LUA_DECODE_ORDER);
  if (lua_type(L, -1) != LUA_TTABLE) {
    goto grid_decode_midi_to_ui_cleanup;
  }

  lua_getglobal(L, GRID_LUA_DECODE_RESULT_MIDI);
  if (lua_type(L, -1) != LUA_TTABLE) {
    goto grid_decode_midi_to_ui_cleanup;
  }

  size_t result_len = lua_rawlen(L, -1);

  lua_newtable(L);

  size_t idx = 0;

  lua_pushinteger(L, grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_MIDI_CHANNEL));
  lua_rawseti(L, -2, ++idx);
  lua_pushinteger(L, grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_MIDI_COMMAND));
  lua_rawseti(L, -2, ++idx);
  lua_pushinteger(L, grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_MIDI_PARAM1));
  lua_rawseti(L, -2, ++idx);
  lua_pushinteger(L, grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_MIDI_PARAM2));
  lua_rawseti(L, -2, ++idx);

  lua_pushinteger(L, grid_msg_get_parameter_raw((uint8_t*)chunk, INSTR));
  lua_rawseti(L, -2, ++idx);
  lua_pushinteger(L, grid_msg_get_parameter_raw((uint8_t*)header, BRC_SX));
  lua_rawseti(L, -2, ++idx);
  lua_pushinteger(L, grid_msg_get_parameter_raw((uint8_t*)header, BRC_SY));
  lua_rawseti(L, -2, ++idx);

  lua_rawseti(L, -2, result_len + 1);

  size_t order_len = lua_rawlen(L, -2);
  lua_pushinteger(L, GRID_LUA_DECODE_ORDER_MIDI);
  lua_rawseti(L, -3, order_len + 1);

  ret = 0;

grid_decode_midi_to_ui_cleanup:

  lua_pop(L, lua_gettop(L));
  grid_lua_semaphore_release(&grid_lua_state);
  return ret;
}

uint8_t grid_decode_sysex_to_ui(char* header, char* chunk) {

  uint8_t sx = grid_msg_get_parameter_raw((uint8_t*)header, BRC_SX);
  uint8_t sy = grid_msg_get_parameter_raw((uint8_t*)header, BRC_SY);

  if (sx == GRID_PARAMETER_DEFAULT_POSITION && sy == GRID_PARAMETER_DEFAULT_POSITION) {
    return 1;
  }

  uint8_t instr = grid_msg_get_parameter_raw((uint8_t*)chunk, INSTR);

  uint16_t length = grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_MIDISYSEX_LENGTH);

  uint16_t off = GRID_CLASS_MIDISYSEX_PAYLOAD_offset;
  uint8_t len = GRID_CLASS_MIDISYSEX_PAYLOAD_length;
  uint8_t first = grid_frame_get_parameter((uint8_t*)chunk, off, len);
  uint8_t last = grid_frame_get_parameter((uint8_t*)chunk, off + (length - 1) * 2, len);

  if (first != 0xf0 || last != 0xf7) {
    grid_port_debug_printf("sysex invalid: %02hhx %02hhx", first, last);
  }

  char sysex_string[100] = {0};
  size_t size = length * GRID_CLASS_MIDISYSEX_PAYLOAD_length;
  memcpy(sysex_string, &chunk[GRID_CLASS_MIDISYSEX_PAYLOAD_offset], size);

  int ret = 1;

  grid_lua_semaphore_lock(&grid_lua_state);

  lua_State* L = grid_lua_state.L;

  lua_getglobal(L, GRID_LUA_DECODE_ORDER);
  if (lua_type(L, -1) != LUA_TTABLE) {
    goto grid_decode_sysex_to_ui_cleanup;
  }

  lua_getglobal(L, GRID_LUA_DECODE_RESULT_SYSEX);
  if (lua_type(L, -1) != LUA_TTABLE) {
    goto grid_decode_sysex_to_ui_cleanup;
  }

  size_t result_len = lua_rawlen(L, -1);

  lua_newtable(L);

  size_t idx = 0;

  lua_pushstring(L, sysex_string);
  lua_rawseti(L, -2, ++idx);

  lua_pushinteger(L, instr);
  lua_rawseti(L, -2, ++idx);
  lua_pushinteger(L, sx);
  lua_rawseti(L, -2, ++idx);
  lua_pushinteger(L, sy);
  lua_rawseti(L, -2, ++idx);

  lua_rawseti(L, -2, result_len + 1);

  size_t order_len = lua_rawlen(L, -2);
  lua_pushinteger(L, GRID_LUA_DECODE_ORDER_SYSEX);
  lua_rawseti(L, -3, order_len + 1);

  ret = 0;

grid_decode_sysex_to_ui_cleanup:

  lua_pop(L, lua_gettop(L));
  grid_lua_semaphore_release(&grid_lua_state);
  return ret;
}

uint8_t grid_decode_immediate_to_ui(char* header, char* chunk) {

  uint8_t target = GRID_DESTINATION_IS_ME | GRID_DESTINATION_IS_GLOBAL | GRID_DESTINATION_IS_LOCAL;
  if (grid_check_destination(header, target) == false) {
    return 1;
  }

  uint8_t instr = grid_msg_get_parameter_raw((uint8_t*)chunk, INSTR);

  if (instr != GRID_INSTR_EXECUTE_code) {
    return 1;
  }

  uint16_t length = grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_IMMEDIATE_ACTIONLENGTH);

  if (length > GRID_PARAMETER_ACTIONSTRING_maxlength) {
    return 1;
  }

  char* script = &chunk[GRID_CLASS_IMMEDIATE_ACTIONSTRING_offset];

  if (strncmp(script, "<?lua ", 6) != 0) {
    return 1;
  }

  if (strncmp(&script[length - 3], " ?>", 3) != 0) {
    return 1;
  }

  grid_lua_clear_stdo(&grid_lua_state);

  script[length - 3] = '\0';
  grid_lua_dostring(&grid_lua_state, &script[6]);
  script[length - 3] = ' ';

  char* stdo = grid_lua_get_output_string(&grid_lua_state);
  if (stdo[0] != '\0') {

    struct grid_msg msg;
    uint8_t xy = GRID_PARAMETER_GLOBAL_POSITION;
    grid_msg_init_brc(&grid_msg_state, &msg, xy, xy);

    grid_msg_nprintf(&msg, "%s", stdo);
    grid_lua_clear_stdo(&grid_lua_state);

    if (grid_msg_close_brc(&grid_msg_state, &msg) >= 0) {
      grid_transport_send_msg_to_all(&grid_transport_state, &msg);
    }
  }

  return 0;
}

uint8_t grid_decode_heartbeat_to_ui(char* header, char* chunk) {

  uint8_t type = grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_HEARTBEAT_TYPE);

  switch (type) {

  // Other grid module
  case 0: {
  } break;

  // USB connected grid module
  case 1: {

    // Calculate absolute position based on header parameters
    uint8_t sx = grid_msg_get_parameter_raw((uint8_t*)header, BRC_SX);
    uint8_t sy = grid_msg_get_parameter_raw((uint8_t*)header, BRC_SY);
    uint8_t rot = grid_msg_get_parameter_raw((uint8_t*)header, BRC_ROT);
    uint8_t portrot = grid_msg_get_parameter_raw((uint8_t*)header, BRC_PORTROT);
    grid_sys_set_module_absolute_position(&grid_sys_state, sx, sy, rot, portrot);

  } break;

  default: {

    if (type <= 127) {
      break;
    }

    grid_ui_state.page_change_enabled = type == 255 ? 1 : 0;

    // Note time of last heartbeat
    grid_msg_set_editor_heartbeat_lastrealtime(&grid_msg_state, grid_platform_rtc_get_micros());

    // If previously unconnected, set as connected
    if (grid_sys_get_editor_connected_state(&grid_sys_state) == 0) {

      grid_sys_set_editor_connected_state(&grid_sys_state, 1);
      grid_platform_printf("EDITOR CONNECT\n");
    }

    // LED preview
    if (grid_protocol_led_change_report_length(&grid_led_state)) {
      grid_protocol_led_preview_generate(&grid_led_state);
    }
  }
  }

  return 0;
}

uint8_t grid_decode_ledpreview_to_ui(char* header, char* chunk) {

  if (grid_check_destination(header, GRID_DESTINATION_IS_ME | GRID_DESTINATION_IS_GLOBAL) == false) {
    return 1;
  }

  uint8_t instr = grid_msg_get_parameter_raw((uint8_t*)chunk, INSTR);

  if (instr != GRID_INSTR_FETCH_code) {
    return 1;
  }

  grid_led_change_flag_reset(&grid_led_state);

  grid_protocol_led_preview_generate(&grid_led_state);

  return 0;
}

uint8_t grid_decode_eventpreview_to_ui(char* header, char* chunk) {

  if (grid_check_destination(header, GRID_DESTINATION_IS_ME | GRID_DESTINATION_IS_GLOBAL) == false) {
    return 1;
  }

  uint8_t instr = grid_msg_get_parameter_raw((uint8_t*)chunk, INSTR);

  if (instr != GRID_INSTR_FETCH_code) {
    return 1;
  }

  struct grid_msg msg;
  uint8_t xy = GRID_PARAMETER_GLOBAL_POSITION;
  grid_msg_init_brc(&grid_msg_state, &msg, xy, xy);

  grid_msg_add_frame(&msg, GRID_CLASS_EVENTPREVIEW_frame_start);
  grid_msg_set_parameter(&msg, INSTR, GRID_INSTR_REPORT_code);

  // -1 to exclude system element
  for (uint8_t j = 0; j < grid_ui_state.element_list_length - 1; ++j) {

    struct grid_ui_element* ele = &grid_ui_state.element_list[j];

    int32_t* params = ele->template_parameter_list;
    uint8_t value1 = params[ele->template_parameter_element_position_index_1];
    uint8_t value2 = params[ele->template_parameter_element_position_index_2];
    grid_msg_nprintf(&msg, "%02x%02x%02x", ele->index, value1, value2);
  }

  size_t length = (grid_ui_state.element_list_length - 1) * 6;
  grid_msg_set_parameter(&msg, CLASS_EVENTPREVIEW_LENGTH, length);

  grid_msg_add_frame(&msg, GRID_CLASS_EVENTPREVIEW_frame_end);

  if (grid_msg_close_brc(&grid_msg_state, &msg) >= 0) {
    grid_transport_send_msg_to_all(&grid_transport_state, &msg);
  }

  return 0;
}

uint8_t grid_decode_namepreview_to_ui(char* header, char* chunk) {

  if (grid_check_destination(header, GRID_DESTINATION_IS_ME | GRID_DESTINATION_IS_GLOBAL) == false) {
    return 1;
  }

  uint8_t instr = grid_msg_get_parameter_raw((uint8_t*)chunk, INSTR);

  if (instr != GRID_INSTR_FETCH_code) {
    return 1;
  }

  struct grid_msg msg;
  uint8_t xy = GRID_PARAMETER_GLOBAL_POSITION;
  grid_msg_init_brc(&grid_msg_state, &msg, xy, xy);

  grid_lua_semaphore_lock(&grid_lua_state);

  // -1 to exclude system element
  for (uint8_t j = 0; j < grid_ui_state.element_list_length - 1; ++j) {

    lua_State* L = grid_lua_state.L;
    lua_pushinteger(L, j);
    l_grid_elementname_get(L);
    l_grid_elementname_send(L);
    grid_msg_nprintf(&msg, "%s", grid_lua_state.stdo);
    grid_lua_clear_stdo(&grid_lua_state);
    lua_pop(L, lua_gettop(L));
  }

  grid_lua_semaphore_release(&grid_lua_state);

  if (grid_msg_close_brc(&grid_msg_state, &msg) >= 0) {
    grid_transport_send_msg_to_all(&grid_transport_state, &msg);
  }

  return 0;
}

uint8_t grid_decode_serialnumber_to_ui(char* header, char* chunk) {

  if (grid_check_destination(header, GRID_DESTINATION_IS_ME | GRID_DESTINATION_IS_GLOBAL) == false) {
    return 1;
  }

  uint8_t instr = grid_msg_get_parameter_raw((uint8_t*)chunk, INSTR);

  if (instr != GRID_INSTR_FETCH_code) {
    return 1;
  }

  uint32_t uniqueid[4] = {0};
  grid_sys_get_id(&grid_sys_state, uniqueid);

  struct grid_msg msg;
  uint8_t xy = GRID_PARAMETER_GLOBAL_POSITION;
  grid_msg_init_brc(&grid_msg_state, &msg, xy, xy);

  grid_msg_add_frame(&msg, GRID_CLASS_SERIALNUMBER_frame);
  grid_msg_set_parameter(&msg, INSTR, GRID_INSTR_REPORT_code);
  grid_msg_set_parameter(&msg, CLASS_SERIALNUMBER_WORD0, uniqueid[0]);
  grid_msg_set_parameter(&msg, CLASS_SERIALNUMBER_WORD1, uniqueid[1]);
  grid_msg_set_parameter(&msg, CLASS_SERIALNUMBER_WORD2, uniqueid[2]);
  grid_msg_set_parameter(&msg, CLASS_SERIALNUMBER_WORD3, uniqueid[3]);

  if (grid_msg_close_brc(&grid_msg_state, &msg) >= 0) {
    grid_transport_send_msg_to_all(&grid_transport_state, &msg);
  }

  return 0;
}

void grid_protocol_nvm_read_success_callback(uint8_t lastheader_id) {

  struct grid_msg msg;
  uint8_t xy = GRID_PARAMETER_GLOBAL_POSITION;
  grid_msg_init_brc(&grid_msg_state, &msg, xy, xy);

  grid_msg_add_frame(&msg, GRID_CLASS_PAGEDISCARD_frame);
  grid_msg_set_parameter(&msg, INSTR, GRID_INSTR_ACKNOWLEDGE_code);
  grid_msg_set_parameter(&msg, CLASS_PAGEDISCARD_LASTHEADER, lastheader_id);
  grid_msg_add_debugtext(&msg, "nvm read complete");

  if (grid_msg_close_brc(&grid_msg_state, &msg) >= 0) {
    grid_transport_send_msg_to_all(&grid_transport_state, &msg);
  }

  grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_WHITE_DIM, 100);
  grid_alert_all_set_timeout_automatic(&grid_led_state);
}

uint8_t grid_decode_pagediscard_to_ui(char* header, char* chunk) {

  if (grid_check_destination(header, GRID_DESTINATION_IS_ME | GRID_DESTINATION_IS_GLOBAL) == false) {
    return 1;
  }

  uint8_t instr = grid_msg_get_parameter_raw((uint8_t*)chunk, INSTR);
  uint8_t id = grid_msg_get_parameter_raw((uint8_t*)header, BRC_ID);

  switch (instr) {

  case GRID_INSTR_EXECUTE_code: {

    enum grid_ui_bulk_status_t status = GRID_UI_BULK_READ_PROGRESS;
    uint8_t page = grid_ui_page_get_activepage(&grid_ui_state);
    void (*cb)(uint8_t) = &grid_protocol_nvm_read_success_callback;

    if (grid_ui_bulk_operation_init(&grid_ui_state, status, page, id, cb)) {
      return 1;
    }

    // Start animation (will be stopped in the callback function)
    grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_YELLOW_DIM, -1);
    grid_alert_all_set_frequency(&grid_led_state, -4);

  } break;
  case GRID_INSTR_CHECK_code: {

    struct grid_msg msg;
    uint8_t xy = GRID_PARAMETER_GLOBAL_POSITION;
    grid_msg_init_brc(&grid_msg_state, &msg, xy, xy);

    grid_msg_add_frame(&msg, GRID_CLASS_PAGEDISCARD_frame);
    grid_msg_set_parameter(&msg, CLASS_PAGEDISCARD_LASTHEADER, id);
    uint8_t respcode = grid_ui_bulk_get_response_code(&grid_ui_state, id);
    grid_msg_set_parameter(&msg, INSTR, respcode);

    if (grid_msg_close_brc(&grid_msg_state, &msg) >= 0) {
      grid_transport_send_msg_to_all(&grid_transport_state, &msg);
    }

  } break;
  }

  return 0;
}

void grid_protocol_nvm_store_success_callback(uint8_t lastheader_id) {

  struct grid_msg msg;
  uint8_t xy = GRID_PARAMETER_GLOBAL_POSITION;
  grid_msg_init_brc(&grid_msg_state, &msg, xy, xy);

  grid_msg_add_frame(&msg, GRID_CLASS_PAGESTORE_frame);
  grid_msg_set_parameter(&msg, INSTR, GRID_INSTR_ACKNOWLEDGE_code);
  grid_msg_set_parameter(&msg, CLASS_PAGESTORE_LASTHEADER, lastheader_id);
  grid_msg_add_debugtext(&msg, "nvm store success");

  if (grid_msg_close_brc(&grid_msg_state, &msg) >= 0) {
    grid_transport_send_msg_to_all(&grid_transport_state, &msg);
  }

  grid_alert_all_set_timeout_automatic(&grid_led_state);

  uint8_t activepage = grid_ui_page_get_activepage(&grid_ui_state);

  grid_ui_page_clear_template_parameters(&grid_ui_state, activepage);

  grid_ui_page_load(&grid_ui_state, activepage);
}

uint8_t grid_decode_pagestore_to_ui(char* header, char* chunk) {

  if (grid_check_destination(header, GRID_DESTINATION_IS_ME | GRID_DESTINATION_IS_GLOBAL) == false) {
    return 1;
  }

  uint8_t instr = grid_msg_get_parameter_raw((uint8_t*)chunk, INSTR);
  uint8_t id = grid_msg_get_parameter_raw((uint8_t*)header, BRC_ID);

  switch (instr) {

  case GRID_INSTR_EXECUTE_code: {

    enum grid_ui_bulk_status_t status = GRID_UI_BULK_STORE_PROGRESS;
    uint8_t page = grid_ui_page_get_activepage(&grid_ui_state);
    void (*cb)(uint8_t) = &grid_protocol_nvm_store_success_callback;

    if (grid_ui_bulk_operation_init(&grid_ui_state, status, page, id, cb)) {
      return 1;
    }

    // Start animation (will be stopped in the callback function)
    grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_YELLOW_DIM, -1);
    grid_alert_all_set_frequency(&grid_led_state, -4);

  } break;
  case GRID_INSTR_CHECK_code: {

    struct grid_msg msg;
    uint8_t xy = GRID_PARAMETER_GLOBAL_POSITION;
    grid_msg_init_brc(&grid_msg_state, &msg, xy, xy);

    grid_msg_add_frame(&msg, GRID_CLASS_PAGESTORE_frame);
    grid_msg_set_parameter(&msg, CLASS_PAGESTORE_LASTHEADER, id);
    uint8_t respcode = grid_ui_bulk_get_response_code(&grid_ui_state, id);
    grid_msg_set_parameter(&msg, INSTR, respcode);

    if (grid_msg_close_brc(&grid_msg_state, &msg) >= 0) {
      grid_transport_send_msg_to_all(&grid_transport_state, &msg);
    }

  } break;
  }

  return 0;
}

void grid_protocol_nvm_clear_success_callback(uint8_t lastheader_id) {

  struct grid_msg msg;
  uint8_t xy = GRID_PARAMETER_GLOBAL_POSITION;
  grid_msg_init_brc(&grid_msg_state, &msg, xy, xy);

  grid_msg_add_frame(&msg, GRID_CLASS_PAGECLEAR_frame);
  grid_msg_set_parameter(&msg, INSTR, GRID_INSTR_ACKNOWLEDGE_code);
  grid_msg_set_parameter(&msg, CLASS_PAGECLEAR_LASTHEADER, lastheader_id);
  grid_msg_add_debugtext(&msg, "nvm clear complete");

  if (grid_msg_close_brc(&grid_msg_state, &msg) >= 0) {
    grid_transport_send_msg_to_all(&grid_transport_state, &msg);
  }

  grid_alert_all_set_timeout_automatic(&grid_led_state);

  uint8_t activepage = grid_ui_page_get_activepage(&grid_ui_state);

  grid_ui_page_clear_template_parameters(&grid_ui_state, activepage);

  grid_ui_page_load(&grid_ui_state, activepage);
}

uint8_t grid_decode_pageclear_to_ui(char* header, char* chunk) {

  if (grid_check_destination(header, GRID_DESTINATION_IS_ME | GRID_DESTINATION_IS_GLOBAL) == false) {
    return 1;
  }

  uint8_t instr = grid_msg_get_parameter_raw((uint8_t*)chunk, INSTR);
  uint8_t id = grid_msg_get_parameter_raw((uint8_t*)header, BRC_ID);

  switch (instr) {

  case GRID_INSTR_EXECUTE_code: {

    enum grid_ui_bulk_status_t status = GRID_UI_BULK_CLEAR_PROGRESS;
    uint8_t page = grid_ui_page_get_activepage(&grid_ui_state);
    void (*cb)(uint8_t) = &grid_protocol_nvm_clear_success_callback;

    if (grid_ui_bulk_operation_init(&grid_ui_state, status, page, id, cb)) {
      return 1;
    }

    // Start animation (will be stopped in the callback function)
    grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_YELLOW_DIM, -1);
    grid_alert_all_set_frequency(&grid_led_state, -4);

  } break;
  case GRID_INSTR_CHECK_code: {

    struct grid_msg msg;
    uint8_t xy = GRID_PARAMETER_GLOBAL_POSITION;
    grid_msg_init_brc(&grid_msg_state, &msg, xy, xy);

    grid_msg_add_frame(&msg, GRID_CLASS_PAGECLEAR_frame);
    grid_msg_set_parameter(&msg, CLASS_PAGECLEAR_LASTHEADER, id);
    uint8_t respcode = grid_ui_bulk_get_response_code(&grid_ui_state, id);
    grid_msg_set_parameter(&msg, INSTR, respcode);

    if (grid_msg_close_brc(&grid_msg_state, &msg) >= 0) {
      grid_transport_send_msg_to_all(&grid_transport_state, &msg);
    }

  } break;
  }

  return 0;
}

void grid_protocol_nvm_erase_success_callback(uint8_t lastheader_id) {

  struct grid_msg msg;
  uint8_t xy = GRID_PARAMETER_GLOBAL_POSITION;
  grid_msg_init_brc(&grid_msg_state, &msg, xy, xy);

  grid_msg_add_frame(&msg, GRID_CLASS_NVMERASE_frame);
  grid_msg_set_parameter(&msg, INSTR, GRID_INSTR_ACKNOWLEDGE_code);
  grid_msg_set_parameter(&msg, CLASS_NVMERASE_LASTHEADER, lastheader_id);
  grid_msg_add_debugtext(&msg, "nvm erase complete");

  if (grid_msg_close_brc(&grid_msg_state, &msg) >= 0) {
    grid_transport_send_msg_to_all(&grid_transport_state, &msg);
  }

  grid_ui_page_load(&grid_ui_state, grid_ui_page_get_activepage(&grid_ui_state));
}

uint8_t grid_decode_nvmerase_to_ui(char* header, char* chunk) {

  if (grid_check_destination(header, GRID_DESTINATION_IS_ME | GRID_DESTINATION_IS_GLOBAL) == false) {
    return 1;
  }

  uint8_t instr = grid_msg_get_parameter_raw((uint8_t*)chunk, INSTR);
  uint8_t id = grid_msg_get_parameter_raw((uint8_t*)header, BRC_ID);

  switch (instr) {

  case GRID_INSTR_EXECUTE_code: {

    void (*cb)(uint8_t) = &grid_protocol_nvm_erase_success_callback;

    if (grid_ui_bulk_nvmerase_init(&grid_ui_state, id, cb)) {
      return 1;
    }

    // Start animation (will be stopped in the callback function)
    grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_YELLOW_DIM, -1);
    grid_alert_all_set_frequency(&grid_led_state, -4);

    // for (uint8_t i = 0; i < grid_led_get_led_count(&grid_led_state); i++) {
    //   grid_led_set_layer_min(&grid_led_state, i, GRID_LED_LAYER_ALERT, GRID_LED_COLOR_YELLOW_DIM);
    // }

  } break;
  case GRID_INSTR_CHECK_code: {

    struct grid_msg msg;
    uint8_t xy = GRID_PARAMETER_GLOBAL_POSITION;
    grid_msg_init_brc(&grid_msg_state, &msg, xy, xy);

    grid_msg_add_frame(&msg, GRID_CLASS_NVMERASE_frame);
    grid_msg_set_parameter(&msg, CLASS_NVMERASE_LASTHEADER, id);
    uint8_t respcode = grid_ui_bulk_get_response_code(&grid_ui_state, id);
    grid_msg_set_parameter(&msg, INSTR, respcode);

    if (grid_msg_close_brc(&grid_msg_state, &msg) >= 0) {
      grid_transport_send_msg_to_all(&grid_transport_state, &msg);
    }

  } break;
  }

  return 0;
}

uint8_t grid_decode_eventview_to_ui(char* header, char* chunk) {

  // return 1;

  uint8_t sx = grid_msg_get_parameter_raw((uint8_t*)header, BRC_SX);
  uint8_t sy = grid_msg_get_parameter_raw((uint8_t*)header, BRC_SY);

  uint8_t instr = grid_msg_get_parameter_raw((uint8_t*)chunk, INSTR);

  uint8_t page = grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_EVENTVIEW_PAGE);
  uint8_t element = grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_EVENTVIEW_ELEMENT);
  uint8_t event = grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_EVENTVIEW_EVENT);

  int16_t value1 = grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_EVENTVIEW_VALUE1);
  int16_t min1 = grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_EVENTVIEW_MIN1);
  int16_t max1 = grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_EVENTVIEW_MAX1);

  uint16_t offset = GRID_CLASS_EVENTVIEW_LENGTH_offset;
  uint8_t hexes = GRID_CLASS_EVENTVIEW_LENGTH_length;
  char name[GRID_ELEMENT_NAME_SIZE] = {0};
  if (grid_frame_get_segment_char((uint8_t*)chunk, offset, hexes, GRID_ELEMENT_NAME_SIZE, name) < 0) {
    return 1;
  }

  int ret = 1;

  grid_lua_semaphore_lock(&grid_lua_state);

  lua_State* L = grid_lua_state.L;

  lua_getglobal(L, GRID_LUA_DECODE_ORDER);
  if (lua_type(L, -1) != LUA_TTABLE) {
    goto grid_decode_eventview_to_ui_cleanup;
  }

  lua_getglobal(L, GRID_LUA_DECODE_RESULT_EVIEW);
  if (lua_type(L, -1) != LUA_TTABLE) {
    goto grid_decode_eventview_to_ui_cleanup;
  }

  size_t result_len = lua_rawlen(L, -1);

  lua_newtable(L);

  size_t idx = 0;

  lua_pushinteger(L, instr);
  lua_rawseti(L, -2, ++idx);
  lua_pushinteger(L, sx);
  lua_rawseti(L, -2, ++idx);
  lua_pushinteger(L, sy);
  lua_rawseti(L, -2, ++idx);

  lua_pushinteger(L, page);
  lua_rawseti(L, -2, ++idx);
  lua_pushinteger(L, element);
  lua_rawseti(L, -2, ++idx);
  lua_pushinteger(L, event);
  lua_rawseti(L, -2, ++idx);

  lua_pushinteger(L, value1);
  lua_rawseti(L, -2, ++idx);
  lua_pushinteger(L, min1);
  lua_rawseti(L, -2, ++idx);
  lua_pushinteger(L, max1);
  lua_rawseti(L, -2, ++idx);

  lua_pushstring(L, name);
  lua_rawseti(L, -2, ++idx);

  lua_rawseti(L, -2, result_len + 1);

  size_t order_len = lua_rawlen(L, -2);
  lua_pushinteger(L, GRID_LUA_DECODE_ORDER_EVIEW);
  lua_rawseti(L, -3, order_len + 1);

  ret = 0;

grid_decode_eventview_to_ui_cleanup:

  lua_pop(L, lua_gettop(L));
  grid_lua_semaphore_release(&grid_lua_state);
  return ret;
}

uint8_t grid_decode_config_to_ui(char* header, char* chunk) {

  if (grid_check_destination(header, GRID_DESTINATION_IS_ME | GRID_DESTINATION_IS_LOCAL) == false) {
    return 1;
  }

  uint8_t instr = grid_msg_get_parameter_raw((uint8_t*)chunk, INSTR);

  uint8_t page = grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_CONFIG_PAGENUMBER);
  uint8_t element = grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_CONFIG_ELEMENTNUMBER);
  uint8_t event = grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_CONFIG_EVENTTYPE);

  // Map system element to 255
  if (element == 255) {
    element = grid_ui_state.element_list_length - 1;
  }

  switch (instr) {

  case GRID_INSTR_EXECUTE_code: {

    // Disable HID
    grid_usb_keyboard_disable(&grid_usb_keyboard_state);

    uint16_t actionlength = grid_msg_get_parameter_raw((uint8_t*)chunk, CLASS_CONFIG_ACTIONLENGTH);

    char* action = &chunk[GRID_CLASS_CONFIG_ACTIONSTRING_offset];

    // By default, generate nacknowledge
    uint8_t respinstr = GRID_INSTR_NACKNOWLEDGE_code;

    bool validlength = actionlength <= GRID_PARAMETER_ACTIONSTRING_maxlength;
    bool endswithetx = validlength ? action[actionlength] == GRID_CONST_ETX : false;
    bool currentpage = page == grid_ui_state.page_activepage;
    bool validelement = element < grid_ui_state.element_list_length;
    struct grid_ui_element* ele = validelement ? &grid_ui_state.element_list[element] : NULL;
    struct grid_ui_event* eve = validelement ? grid_ui_event_find(ele, event) : NULL;

    if (validlength && endswithetx && currentpage && validelement && ele && eve) {

      // Disable page change
      grid_ui_state.page_change_enabled = 0;

      // Set alert for feedback
      grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_WHITE, 64);

      // Register actionstring for event
      action[actionlength] = '\0';
      grid_ui_event_register_actionstring(eve, action);
      action[actionlength] = GRID_CONST_ETX;

      // Local-trigger the event
      grid_ui_event_state_set(eve, GRID_EVE_STATE_TRIG_LOCAL);

      // Set acknowledge as response code
      respinstr = GRID_INSTR_ACKNOWLEDGE_code;

    } else {

      grid_port_debug_printf("failed to set config, conditions: %d%d%d%d%d%d", validlength, endswithetx, currentpage, validelement, ele != NULL, eve != NULL);
    }

    uint8_t id = grid_msg_get_parameter_raw((uint8_t*)header, BRC_ID);

    // Generate response
    struct grid_msg msg;
    uint8_t xy = GRID_PARAMETER_GLOBAL_POSITION;
    grid_msg_init_brc(&grid_msg_state, &msg, xy, xy);

    grid_msg_add_frame(&msg, GRID_CLASS_CONFIG_frame_check);
    grid_msg_set_parameter(&msg, INSTR, respinstr);
    grid_msg_set_parameter(&msg, CLASS_CONFIG_LASTHEADER, id);

    if (grid_msg_close_brc(&grid_msg_state, &msg) >= 0) {
      grid_transport_send_msg_to_all(&grid_transport_state, &msg);
    }

  } break;

  case GRID_INSTR_FETCH_code: {

    char temp[GRID_PARAMETER_ACTIONSTRING_maxlength] = {0};
    int status = grid_ui_event_recall_configuration(&grid_ui_state, page, element, event, temp);

    if (status || temp[0] == '\0') {
      break;
    }

    // Map system element back to 255
    uint8_t element2 = element == grid_ui_state.element_list_length - 1 ? 255 : element;

    struct grid_msg msg;
    uint8_t xy = GRID_PARAMETER_GLOBAL_POSITION;
    grid_msg_init_brc(&grid_msg_state, &msg, xy, xy);

    grid_msg_add_frame(&msg, GRID_CLASS_CONFIG_frame_start);
    grid_msg_set_parameter(&msg, INSTR, GRID_INSTR_REPORT_code);
    grid_msg_set_parameter(&msg, CLASS_CONFIG_VERSIONMAJOR, GRID_PROTOCOL_VERSION_MAJOR);
    grid_msg_set_parameter(&msg, CLASS_CONFIG_VERSIONMINOR, GRID_PROTOCOL_VERSION_MINOR);
    grid_msg_set_parameter(&msg, CLASS_CONFIG_VERSIONPATCH, GRID_PROTOCOL_VERSION_PATCH);
    grid_msg_set_parameter(&msg, CLASS_CONFIG_PAGENUMBER, page);
    grid_msg_set_parameter(&msg, CLASS_CONFIG_ELEMENTNUMBER, element2);
    grid_msg_set_parameter(&msg, CLASS_CONFIG_EVENTTYPE, event);
    grid_msg_set_parameter(&msg, CLASS_CONFIG_ACTIONLENGTH, strlen(temp));
    grid_msg_nprintf(&msg, "%s", temp);
    grid_msg_add_frame(&msg, GRID_CLASS_CONFIG_frame_end);

    if (grid_msg_close_brc(&grid_msg_state, &msg) >= 0) {
      grid_transport_send_msg_to_all(&grid_transport_state, &msg);
    }

  } break;
  }

  return 0;
}

int grid_port_decode_class(struct grid_decoder_collection* coll, uint16_t class, char* header, char* chunk) {

  for (uint8_t i = 0; coll[i].process != NULL; ++i) {

    if (class == coll[i].class) {

      coll[i].process(header, chunk);
      return 0;
    }
  }

  return 1;
}

void grid_port_decode_msg(struct grid_decoder_collection* coll, struct grid_msg* msg) {

  grid_lua_decode_clear_results(&grid_lua_state);

  for (uint32_t i = 0; i < msg->length; ++i) {

    if (msg->data[i] != GRID_CONST_STX) {
      continue;
    }

    grid_msg_set_offset(msg, i);
    uint32_t class = grid_msg_get_parameter(msg, PARAMETER_CLASSCODE);
    grid_port_decode_class(coll, class, msg->data, &msg->data[i]);
  }

  grid_lua_decode_process_results(&grid_lua_state);

  grid_lua_broadcast_stdo(&grid_lua_state);
  grid_lua_broadcast_stde(&grid_lua_state);
}

struct grid_decoder_collection grid_decoder_to_ui[] = {
    {GRID_CLASS_PAGEACTIVE_code, grid_decode_pageactive_to_ui},
    {GRID_CLASS_PAGECOUNT_code, grid_decode_pagecount_to_ui},
    {GRID_CLASS_MIDI_code, grid_decode_midi_to_ui},
    {GRID_CLASS_MIDISYSEX_code, grid_decode_sysex_to_ui},
    {GRID_CLASS_IMMEDIATE_code, grid_decode_immediate_to_ui},
    {GRID_CLASS_HEARTBEAT_code, grid_decode_heartbeat_to_ui},
    {GRID_CLASS_SERIALNUMBER_code, grid_decode_serialnumber_to_ui},
    {GRID_CLASS_PAGEDISCARD_code, grid_decode_pagediscard_to_ui},
    {GRID_CLASS_PAGESTORE_code, grid_decode_pagestore_to_ui},
    {GRID_CLASS_PAGECLEAR_code, grid_decode_pageclear_to_ui},
    {GRID_CLASS_NVMERASE_code, grid_decode_nvmerase_to_ui},
    {GRID_CLASS_LEDPREVIEW_code, grid_decode_ledpreview_to_ui},
    {GRID_CLASS_EVENTPREVIEW_code, grid_decode_eventpreview_to_ui},
    {GRID_CLASS_NAMEPREVIEW_code, grid_decode_namepreview_to_ui},
    {GRID_CLASS_EVENTVIEW_code, grid_decode_eventview_to_ui},
    {GRID_CLASS_CONFIG_code, grid_decode_config_to_ui},
    {0, NULL},
};

struct grid_decoder_collection* grid_decoder_to_ui_reference = grid_decoder_to_ui;

struct grid_decoder_collection grid_decoder_to_usb[] = {
    {GRID_CLASS_MIDI_code, grid_decode_midi_to_usb},
    {GRID_CLASS_MIDISYSEX_code, grid_decode_sysex_to_usb},
    {GRID_CLASS_HIDMOUSEBUTTON_code, grid_decode_mousebutton_to_usb},
    {GRID_CLASS_HIDMOUSEMOVE_code, grid_decode_mousemove_to_usb},
    {GRID_CLASS_HIDGAMEPADBUTTON_code, grid_decode_gamepadbutton_to_usb},
    {GRID_CLASS_HIDGAMEPADMOVE_code, grid_decode_gamepadmove_to_usb},
    {GRID_CLASS_HIDKEYBOARD_code, grid_decode_keyboard_to_usb},
    {0, NULL},
};

struct grid_decoder_collection* grid_decoder_to_usb_reference = grid_decoder_to_usb;
