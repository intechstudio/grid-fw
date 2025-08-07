#include "grid_lua_api.h"

#include <stdlib.h>
#include <string.h>

#include "grid_cal.h"
#include "grid_led.h"
#include "grid_math.h"
#include "grid_msg.h"
#include "grid_platform.h"
#include "grid_protocol.h"
#include "grid_sys.h"
#include "grid_transport.h"
#include "grid_ui.h"

/*static*/ int32_t grid_utility_map(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max) { return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min; }

/* ==================== LUA C API REGISTERED FUNCTIONS  ====================*/

/*static*/ int l_my_print(lua_State* L) {
  char message[GRID_PARAMETER_SPI_TRANSACTION_length] = {0};
  int msg_len = 0; // Tracks the current length of the message

  int nargs = lua_gettop(L);

  for (int i = 1; i <= nargs; ++i) {
    if (lua_type(L, i) == LUA_TSTRING) {
      if (msg_len > 0) {
        msg_len += snprintf(message + msg_len, sizeof(message) - msg_len, ", %s", lua_tostring(L, i));
      } else {
        msg_len += snprintf(message + msg_len, sizeof(message) - msg_len, "%s", lua_tostring(L, i));
      }
    } else if (lua_type(L, i) == LUA_TBOOLEAN) {
      if (msg_len > 0) {
        msg_len += snprintf(message + msg_len, sizeof(message) - msg_len, ", %s", lua_toboolean(L, i) ? "true" : "false");
      } else {
        msg_len += snprintf(message + msg_len, sizeof(message) - msg_len, "%s", lua_toboolean(L, i) ? "true" : "false");
      }
    } else if (lua_type(L, i) == LUA_TNUMBER) {
      lua_Number lnum = lua_tonumber(L, i);
      if (msg_len > 0) {
        msg_len += snprintf(message + msg_len, sizeof(message) - msg_len, ", %lf", lnum);
      } else {
        msg_len += snprintf(message + msg_len, sizeof(message) - msg_len, "%lf", lnum);
      }
    } else if (lua_type(L, i) == LUA_TNIL) {
      if (msg_len > 0) {
        msg_len += snprintf(message + msg_len, sizeof(message) - msg_len, ", nil");
      } else {
        msg_len += snprintf(message + msg_len, sizeof(message) - msg_len, "nil");
      }
    } else if (lua_type(L, i) == LUA_TFUNCTION) {
      if (msg_len > 0) {
        msg_len += snprintf(message + msg_len, sizeof(message) - msg_len, ", function");
      } else {
        msg_len += snprintf(message + msg_len, sizeof(message) - msg_len, "function");
      }
    } else if (lua_type(L, i) == LUA_TTABLE) {
      if (msg_len > 0) {
        msg_len += snprintf(message + msg_len, sizeof(message) - msg_len, ", table");
      } else {
        msg_len += snprintf(message + msg_len, sizeof(message) - msg_len, "table");
      }
    }
  }

  // If there is any content to encode
  if (msg_len > 0) {

    grid_port_debug_print_text(message);
  }

  return 0;
}

#ifdef CONFIG_IDF_TARGET_ESP32S3

#include <dirent.h>

/*static*/ int l_grid_list_dir(lua_State* L) {
  const char* path = luaL_checkstring(L, 1); // Get the path from Lua
  DIR* dir = opendir(path);
  if (!dir) {
    lua_pushnil(L);
    lua_pushfstring(L, "Cannot open directory: %s", path);
    return 2;
  }

  struct dirent* entry;
  size_t buffer_size = 1024;
  size_t length = 0;
  char* result = (char*)malloc(buffer_size);
  if (!result) {
    closedir(dir);
    lua_pushnil(L);
    lua_pushstring(L, "Memory allocation failed");
    return 2;
  }
  result[0] = '\0'; // Empty string to start

  while ((entry = readdir(dir)) != NULL) {
    const char* name = entry->d_name;

    // Skip "." and ".."
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
      continue;
    }

    size_t name_len = strlen(name);
    if (length + name_len + 2 > buffer_size) {
      buffer_size *= 2;
      char* new_result = realloc(result, buffer_size);
      if (!new_result) {
        free(result);
        closedir(dir);
        lua_pushnil(L);
        lua_pushstring(L, "Memory reallocation failed");
        return 2;
      }
      result = new_result;
    }

    if (length > 0) {
      result[length++] = '\n';
      result[length] = '\0';
    }

    strcat(result, name);
    length += name_len;
  }

  closedir(dir);

  grid_platform_printf("LIST DIR: %s\n%s\n", path, result);
  lua_pushstring(L, result);
  free(result);
  return 1;
}

int l_grid_cat(lua_State* L) {
  // Get the file path from the Lua stack (the first argument)
  const char* filename = luaL_checkstring(L, 1);

  // Open the file in binary mode (use "r" for text mode or "rb" for binary mode)
  FILE* file = fopen(filename, "rb");
  if (!file) {
    // If the file cannot be opened, return nil and error message
    lua_pushnil(L);
    lua_pushfstring(L, "failed to open file: %s", filename);
    return 2; // Return two values: nil and the error message
  }

  // Get the size of the file
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  // Allocate buffer for the file contents
  char* buffer = (char*)malloc(file_size + 1); // +1 for the null terminator
  if (!buffer) {
    // If memory allocation fails, return nil and error message
    fclose(file);
    lua_pushnil(L);
    lua_pushstring(L, "memory allocation failed");
    return 2;
  }

  // Read the file into the buffer
  size_t bytes_read = fread(buffer, 1, file_size, file);
  fclose(file);

  // If reading fails, return nil and error message
  if (bytes_read != file_size) {
    free(buffer);
    lua_pushnil(L);
    lua_pushfstring(L, "failed to read file: %s", filename);
    return 2;
  }

  // Null-terminate the buffer and push it to the Lua stack
  buffer[bytes_read] = '\0';
  grid_platform_printf("CAT FILE: %s\n%s\n", filename, buffer);
  lua_pushstring(L, buffer);

  // Free the buffer after pushing it to the Lua stack
  free(buffer);

  // Return 1 value (the file content as a Lua string)
  return 1;
}

#else

/*static*/ int l_grid_list_dir(lua_State* L) { return 1; }
/*static*/ int l_grid_cat(lua_State* L) { return 1; }

#endif

/*static*/ int l_grid_websocket_send(lua_State* L) {

  char message[GRID_PARAMETER_SPI_TRANSACTION_length] = {0};

  int nargs = lua_gettop(L);
  // grid_platform_printf("LUA PRINT: ");
  for (int i = 1; i <= nargs; ++i) {

    if (lua_type(L, i) == LUA_TSTRING) {

      strcat(message, lua_tostring(L, i));
      // grid_platform_printf(" str: %s ", lua_tostring(L, i));
    } else if (lua_type(L, i) == LUA_TBOOLEAN) {
      lua_toboolean(L, i) ? grid_port_debug_printf("true") : grid_port_debug_printf("false");
    } else if (lua_type(L, i) == LUA_TNUMBER) {

      lua_Number lnum = lua_tonumber(L, i);
      lua_Integer lint;
      lua_numbertointeger(lnum, &lint);
      // int32_t num = lua_tonumber

      sprintf(&message[strlen(message)], "%lf", lnum);

      // remove unnesesery trailing zeros
      uint8_t index_helper = strlen(message);
      for (uint8_t i = 0; i < 8; i++) {

        if (message[index_helper - i - 1] == '0') {

          message[index_helper - i - 1] = '\0';
        } else if (message[index_helper - i - 1] == '.') {

          message[index_helper - i - 1] = '\0';
          break;
        } else {
          break;
        }
      }

      // grid_platform_printf(" num: %d ", (int)lnum);
    } else if (lua_type(L, i) == LUA_TNIL) {
      // grid_platform_printf(" nil ");
    } else if (lua_type(L, i) == LUA_TFUNCTION) {
      // grid_platform_printf(" fnc ");
    } else if (lua_type(L, i) == LUA_TTABLE) {
      // grid_platform_printf(" table ");
    } else {
      // grid_platform_printf(" unknown data type ");
    }
  }

  if (nargs == 0) {
    // grid_platform_printf(" no arguments ");
  }

  // grid_platform_printf("\r\n");

  grid_port_websocket_print_text(message);

  return 0;
}

/*static*/ int l_grid_package_send(lua_State* L) {

  char message[GRID_PARAMETER_SPI_TRANSACTION_length] = {0};

  int nargs = lua_gettop(L);
  // grid_platform_printf("LUA PRINT: ");
  for (int i = 1; i <= nargs; ++i) {

    if (lua_type(L, i) == LUA_TSTRING) {
      if (strlen(message) > 0) {

        strcat(message, ",");
      }
      strcat(message, "\"");
      strcat(message, lua_tostring(L, i));
      strcat(message, "\"");
      // grid_platform_printf(" str: %s ", lua_tostring(L, i));
    } else if (lua_type(L, i) == LUA_TBOOLEAN) {
      bool b = lua_toboolean(L, i);
      if (strlen(message) > 0) {

        strcat(message, ",");
      }
      if (b) {

        strcat(message, "true");
      } else {

        strcat(message, "false");
      }
    } else if (lua_type(L, i) == LUA_TNUMBER) {

      if (strlen(message) > 0) {

        strcat(message, ",");
      }
      lua_Number lnum = lua_tonumber(L, i);
      lua_Integer lint;
      lua_numbertointeger(lnum, &lint);
      // int32_t num = lua_tonumber

      sprintf(&message[strlen(message)], "%lf", lnum);

      // remove unnesesery trailing zeros
      uint8_t index_helper = strlen(message);
      for (uint8_t i = 0; i < 8; i++) {

        if (message[index_helper - i - 1] == '0') {

          message[index_helper - i - 1] = '\0';
        } else if (message[index_helper - i - 1] == '.') {

          message[index_helper - i - 1] = '\0';
          break;
        } else {
          break;
        }
      }

      // grid_platform_printf(" num: %d ", (int)lnum);
    } else if (lua_type(L, i) == LUA_TNIL) {
      // grid_platform_printf(" nil ");
    } else if (lua_type(L, i) == LUA_TFUNCTION) {
      // grid_platform_printf(" fnc ");
    } else if (lua_type(L, i) == LUA_TTABLE) {
      // grid_platform_printf(" table ");
    } else {
      // grid_platform_printf(" unknown data type ");
    }
  }

  if (nargs == 0) {
    // grid_platform_printf(" no arguments ");
  }

  // grid_platform_printf("\r\n");

  grid_port_package_print_text(message);

  return 0;
}

/*static*/ int l_grid_immediate_send(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 3) {
    grid_port_debug_printf("Invalid arguments! %s", GRID_LUA_FNC_G_IMMEDIATE_SEND_usage);
    return 0;
  }

  uint8_t x = GRID_PARAMETER_GLOBAL_POSITION;
  uint8_t y = GRID_PARAMETER_GLOBAL_POSITION;

  if (lua_type(L, 1) == LUA_TNUMBER && lua_type(L, 2) == LUA_TNUMBER) {
    x = lua_tonumber(L, 1) + GRID_PARAMETER_DEFAULT_POSITION;
    y = lua_tonumber(L, 2) + GRID_PARAMETER_DEFAULT_POSITION;
  }

  if (lua_type(L, 3) != LUA_TSTRING) {
    grid_port_debug_printf("Invalid arguments! %s", GRID_LUA_FNC_G_IMMEDIATE_SEND_usage);
    return 0;
  }

  const char* str = lua_tostring(L, 3);

  struct grid_msg_packet pkt;
  grid_msg_packet_init(&grid_msg_state, &pkt, x, y);

  grid_msg_packet_body_append_printf(&pkt, GRID_CLASS_IMMEDIATE_frame_start);
  grid_msg_packet_body_set_parameter(&pkt, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);

  uint32_t actionlength = strlen(str) + strlen("<?lua ") + strlen(" ?>");
  grid_msg_packet_body_set_parameter(&pkt, 0, GRID_CLASS_IMMEDIATE_ACTIONLENGTH_offset, GRID_CLASS_IMMEDIATE_ACTIONLENGTH_length, actionlength);

  if (grid_msg_packet_body_append_nprintf(&pkt, "<?lua %s ?>", str) <= 0) {
    grid_port_debug_printf("Length of actionstring exceeds message! %u", actionlength);
    return 0;
  }

  if (grid_msg_packet_body_append_nprintf(&pkt, GRID_CLASS_IMMEDIATE_frame_end) <= 0) {
    grid_port_debug_printf("GRID_CLASS_IMMEDIATE_frame_end exceeds message!");
    return 0;
  }

  grid_msg_packet_close(&grid_msg_state, &pkt);
  grid_transport_send_msg_packet_to_all(&grid_transport_state, &pkt);

  return 0;
}

/*static*/ int l_grid_elementname_send(lua_State* L) {

  int nargs = lua_gettop(L);
  // grid_platform_printf("LUA PRINT: ");
  if (nargs == 2) {

    if (lua_type(L, 1) == LUA_TNUMBER && lua_type(L, 2) == LUA_TSTRING) {

      uint8_t number = (uint8_t)lua_tointeger(L, 1);
      char string[20] = {0};

      strncpy(string, lua_tostring(L, 2), 19);

      char frame[30] = {0};
      sprintf(frame, GRID_CLASS_ELEMENTNAME_frame_start);

      grid_str_set_parameter(frame, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code, NULL);

      grid_str_set_parameter(frame, GRID_CLASS_ELEMENTNAME_NUM_offset, GRID_CLASS_ELEMENTNAME_NUM_length, number, NULL);
      grid_str_set_parameter(frame, GRID_CLASS_ELEMENTNAME_LENGTH_offset, GRID_CLASS_ELEMENTNAME_LENGTH_length, strlen(string), NULL);
      strcpy(&frame[GRID_CLASS_ELEMENTNAME_NAME_offset], string);
      sprintf(&frame[strlen(frame)], GRID_CLASS_ELEMENTNAME_frame_end);

      strcat(grid_lua_state.stdo, frame);

      // MUST BE SENT OUT IMMEDIATELY (NOT THROUGH STDO) BECAUSE IT MUST BE SENT
      // OUT EVEN AFTER LOCAL TRIGGER (CONFIG) struct grid_msg_packet response;

      // grid_msg_packet_init(&grid_msg_state, &response,
      // GRID_SYS_GLOBAL_POSITION, GRID_SYS_GLOBAL_POSITION);

      // uint8_t response_payload[50] = {0};

      // grid_msg_packet_body_append_grid_platform_printf(&response,
      // GRID_CLASS_ELEMENTNAME_frame_start);

      // grid_msg_packet_body_set_parameter(&response, 0, GRID_INSTR_offset,
      // GRID_INSTR_length, GRID_INSTR_EXECUTE_code);
      // grid_msg_packet_body_set_parameter(&response, 0,
      // GRID_CLASS_ELEMENTNAME_NUM_offset, GRID_CLASS_ELEMENTNAME_NUM_length,
      // number); grid_msg_packet_body_set_parameter(&response, 0,
      // GRID_CLASS_ELEMENTNAME_LENGTH_offset,
      // GRID_CLASS_ELEMENTNAME_LENGTH_length, strlen(string));

      // grid_msg_packet_body_append_grid_platform_printf(&response, "%s",
      // string); grid_msg_packet_body_append_grid_platform_printf(&response,
      // GRID_CLASS_ELEMENTNAME_frame_end);

      // grid_msg_packet_close(&grid_msg_state, &response);
      // grid_port_packet_send_everywhere(&response);

      // grid_port_debug_printf("SN: %d, %s", number, string);
      // grid_platform_printf(" str: %s ", lua_tostring(L, i));
    } else {
      grid_port_debug_printf("Invalid args");
    }
  } else {
    grid_port_debug_printf("Invalid args");
  }

  return 0;
}

/*static*/ int l_grid_elementname_set(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 2) {
    // error
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  if (!lua_isinteger(L, 1)) {
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  int32_t element = lua_tointeger(L, 1);

  struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, element);
  if (!ele) {
    strcat(grid_lua_state.stde, "Invalid element index!");
    return 0;
  }

  if (!lua_isstring(L, 2)) {
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  const char* name = lua_tostring(L, 2);

  size_t name_len = strlen(name);
  if (name_len > GRID_ELEMENT_NAME_MAX) {
    strcat(grid_lua_state.stde, "Length of string exceeds maximum!");
    return 0;
  }

  strcpy(ele->name, name);

  return 0;
}

/*static*/ int l_grid_elementname_get(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 1) {
    // error
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  if (!lua_isinteger(L, 1)) {
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  int32_t element = lua_tointeger(L, 1);

  struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, element);

  if (!ele) {
    strcat(grid_lua_state.stde, "Invalid element index!");
    return 0;
  }

  size_t name_len = strnlen(ele->name, GRID_ELEMENT_NAME_SIZE);
  if (name_len > GRID_ELEMENT_NAME_MAX) {
    strcat(grid_lua_state.stde, "Stored string has no null terminator!");
    return 0;
  }

  lua_pushstring(L, ele->name);

  return 1;
}

/*static*/ int l_grid_string_get(lua_State* L) {

  int nargs = lua_gettop(L);
  // grid_platform_printf("LUA PRINT: ");
  if (nargs == 2) {

    if (lua_type(L, 1) == LUA_TNUMBER && lua_type(L, 2) == LUA_TSTRING) {

      uint32_t pointer = (uint32_t)lua_tointeger(L, 1);

      char* string = (char*)pointer;

      strcpy(string, lua_tostring(L, 2));

      // grid_platform_printf("GET: %x -> %s\r\n", pointer, string);
    } else {
      grid_port_debug_printf("Invalid type of args");
    }
  } else {
    grid_port_debug_printf("Invalid # of args");
  }

  return 0;
}

/*static*/ int l_grid_usb_keyboard_send(lua_State* L) {

  int nargs = lua_gettop(L);

  if ((nargs - 1) % 3 != 0 || nargs == 0) {

    grid_platform_printf("kb invalid params %d\r\n", nargs);
    return 0;
  }

  char temp[20 + nargs * 4];
  memset(temp, 0x00, 20 + nargs * 4);
  sprintf(temp, GRID_CLASS_HIDKEYBOARD_frame_start);

  grid_str_set_parameter(temp, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code, NULL);

  uint16_t cursor = 0;

  uint8_t default_delay = lua_tonumber(L, 1);

  grid_str_set_parameter(temp, GRID_CLASS_HIDKEYBOARD_DEFAULTDELAY_offset, GRID_CLASS_HIDKEYBOARD_DEFAULTDELAY_length, default_delay, NULL);

  uint8_t cnt = 0;

  for (int i = 2; i <= nargs; i += 3) {

    cnt++;

    int32_t modifier = lua_tonumber(L, i);
    int32_t keystate = lua_tonumber(L, i + 1);

    if (modifier > 15 || modifier < 0) {
      grid_platform_printf("invalid modifier param %d\r\n", modifier);
      continue;
    }
    if (!(keystate == 0 || keystate == 1 || keystate == 2)) {
      grid_platform_printf("invalid keystate param %d\r\n", keystate);
      continue;
    }

    if (modifier == 15) {
      // delay command
      int32_t delay = lua_tonumber(L, i + 2);

      if (delay > 4095)
        delay = 4095;
      if (delay < 0)
        delay = 0;

      grid_str_set_parameter(&temp[cursor], GRID_CLASS_HIDKEYBOARD_KEYISMODIFIER_offset, GRID_CLASS_HIDKEYBOARD_KEYISMODIFIER_length, modifier, NULL);
      grid_str_set_parameter(&temp[cursor], GRID_CLASS_HIDKEYBOARD_DELAY_offset, GRID_CLASS_HIDKEYBOARD_DELAY_length, delay, NULL);
      cursor += 4;
    } else if (modifier == 0 || modifier == 1) {
      // normal key or modifier
      int32_t keycode = lua_tonumber(L, i + 2);

      // 01234567890123456789012grid_platform_printf("%d-%d ", cnt, keycode);

      grid_str_set_parameter(&temp[cursor], GRID_CLASS_HIDKEYBOARD_KEYISMODIFIER_offset, GRID_CLASS_HIDKEYBOARD_KEYISMODIFIER_length, modifier, NULL);
      grid_str_set_parameter(&temp[cursor], GRID_CLASS_HIDKEYBOARD_KEYSTATE_offset, GRID_CLASS_HIDKEYBOARD_KEYSTATE_length, keystate, NULL);
      grid_str_set_parameter(&temp[cursor], GRID_CLASS_HIDKEYBOARD_KEYCODE_offset, GRID_CLASS_HIDKEYBOARD_KEYCODE_length, keycode, NULL);
      cursor += 4;

    } else {

      continue;
    }
  }

  grid_str_set_parameter(temp, GRID_CLASS_HIDKEYBOARD_LENGTH_offset, GRID_CLASS_HIDKEYBOARD_LENGTH_length, cursor / 4 + 1, NULL);

  temp[strlen(temp)] = GRID_CONST_ETX;

  if (cursor != 1) {
    strcat(grid_lua_state.stdo, temp);
    // grid_platform_printf("keyboard: %s\r\n", temp);
  } else {
    grid_platform_printf("invalid args!\r\n");
    return 0;
  }

  return 1;
}

/*static*/ int l_grid_mousemove_send(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 2) {
    // error
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  int32_t param[2] = {0};

  for (int i = 1; i <= nargs; ++i) {
    param[i - 1] = lua_tointeger(L, i);
  }

  uint8_t axis_raw = param[0];
  int32_t position_raw = param[1] + 128;

  uint8_t position;
  uint8_t axis;

  if (position_raw > 255) {
    position = 255;
    strcat(grid_lua_state.stde, "#positionOutOfRange");
  } else if (position_raw < 0) {
    position = 0;
    strcat(grid_lua_state.stde, "#positionOutOfRange");
  } else {
    position = position_raw;
  }

  if (axis_raw > 3) {
    strcat(grid_lua_state.stde, "#axisOutOfRange");
    axis = 3;
  } else if (axis_raw < 1) {
    strcat(grid_lua_state.stde, "#axisOutOfRange");
    axis = 1;
  } else {
    axis = axis_raw;
  }

  char frame[20] = {0};

  sprintf(frame, GRID_CLASS_HIDMOUSEMOVE_frame);

  grid_str_set_parameter(frame, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code, NULL);

  grid_str_set_parameter(frame, GRID_CLASS_HIDMOUSEMOVE_POSITION_offset, GRID_CLASS_HIDMOUSEMOVE_POSITION_length, position, NULL);
  grid_str_set_parameter(frame, GRID_CLASS_HIDMOUSEMOVE_AXIS_offset, GRID_CLASS_HIDMOUSEMOVE_AXIS_length, axis, NULL);

  strcat(grid_lua_state.stdo, frame);

  return 1;
}

/*static*/ int l_grid_mousebutton_send(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 2) {
    // error
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  uint8_t param[2] = {0};

  for (int i = 1; i <= nargs; ++i) {
    param[i - 1] = lua_tointeger(L, i);
  }

  uint8_t button_raw = param[0];
  int32_t state_raw = param[1];

  uint8_t state;
  uint8_t button;

  if (state_raw > 1) {
    state = 1;
    strcat(grid_lua_state.stde, "#stateOutOfRange");
  } else if (state_raw < 0) {
    state = 0;
    strcat(grid_lua_state.stde, "#stateOutOfRange");
  } else {
    state = state_raw;
  }

  if (button_raw > 4) {
    strcat(grid_lua_state.stde, "#buttonOutOfRange");
    button = 4;
  } else if (button_raw < 1) {
    strcat(grid_lua_state.stde, "#buttonOutOfRange");
    button = 1;
  } else {
    button = button_raw;
  }

  char frame[20] = {0};

  sprintf(frame, GRID_CLASS_HIDMOUSEBUTTON_frame);

  grid_str_set_parameter(frame, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code, NULL);

  grid_str_set_parameter(frame, GRID_CLASS_HIDMOUSEBUTTON_STATE_offset, GRID_CLASS_HIDMOUSEBUTTON_STATE_length, state, NULL);
  grid_str_set_parameter(frame, GRID_CLASS_HIDMOUSEBUTTON_BUTTON_offset, GRID_CLASS_HIDMOUSEBUTTON_BUTTON_length, button, NULL);

  strcat(grid_lua_state.stdo, frame);

  return 1;
}

/*static*/ int l_grid_gamepadmove_send(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 2) {
    // error
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  int32_t param[2] = {0};

  for (int i = 1; i <= nargs; ++i) {
    param[i - 1] = lua_tointeger(L, i);
  }

  uint8_t axis_raw = param[0];
  int32_t position_raw = param[1] + 128;

  uint8_t position;
  uint8_t axis;

  if (position_raw > 255) {
    position = 255;
    strcat(grid_lua_state.stde, "#positionOutOfRange");
  } else if (position_raw < 0) {
    position = 0;
    strcat(grid_lua_state.stde, "#positionOutOfRange");
  } else {
    position = position_raw;
  }

  if (axis_raw > 5) {
    strcat(grid_lua_state.stde, "#axisOutOfRange");
    axis = 5;
  } else {
    axis = axis_raw;
  }

  char frame[20] = {0};

  sprintf(frame, GRID_CLASS_HIDGAMEPADMOVE_frame);

  grid_str_set_parameter(frame, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code, NULL);

  grid_str_set_parameter(frame, GRID_CLASS_HIDGAMEPADMOVE_AXIS_offset, GRID_CLASS_HIDGAMEPADMOVE_AXIS_length, axis, NULL);
  grid_str_set_parameter(frame, GRID_CLASS_HIDGAMEPADMOVE_POSITION_offset, GRID_CLASS_HIDGAMEPADMOVE_POSITION_length, position, NULL);

  strcat(grid_lua_state.stdo, frame);

  return 1;
}

/*static*/ int l_grid_gamepadbutton_send(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 2) {
    // error
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  uint8_t param[2] = {0};

  for (int i = 1; i <= nargs; ++i) {
    param[i - 1] = lua_tointeger(L, i);
  }

  uint8_t button_raw = param[0];
  int32_t state_raw = param[1];

  uint8_t state;
  uint8_t button;

  if (state_raw > 1) {
    state = 1;
    strcat(grid_lua_state.stde, "#stateOutOfRange");
  } else if (state_raw < 0) {
    state = 0;
    strcat(grid_lua_state.stde, "#stateOutOfRange");
  } else {
    state = state_raw;
  }

  if (button_raw > 31) {
    strcat(grid_lua_state.stde, "#buttonOutOfRange");
    button = 31;
  } else {
    button = button_raw;
  }

  char frame[20] = {0};

  sprintf(frame, GRID_CLASS_HIDGAMEPADBUTTON_frame);

  grid_str_set_parameter(frame, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code, NULL);

  grid_str_set_parameter(frame, GRID_CLASS_HIDGAMEPADBUTTON_BUTTON_offset, GRID_CLASS_HIDGAMEPADBUTTON_BUTTON_length, button, NULL);
  grid_str_set_parameter(frame, GRID_CLASS_HIDGAMEPADBUTTON_STATE_offset, GRID_CLASS_HIDGAMEPADBUTTON_STATE_length, state, NULL);

  strcat(grid_lua_state.stdo, frame);

  return 1;
}

/*static*/ int l_grid_send(lua_State* L) {

  int nargs = lua_gettop(L);

  char start_of_text[2] = {GRID_CONST_STX, 0};

  strcat(grid_lua_state.stdo, start_of_text);

  for (int i = 1; i <= nargs; ++i) {
    strcat(grid_lua_state.stdo, lua_tostring(L, i));
  }

  char end_of_text[2] = {GRID_CONST_ETX, 0};

  strcat(grid_lua_state.stdo, end_of_text);

  return 0;
}

/*static*/ int l_grid_midirx_enabled(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 1) {
    // error
    strcat(grid_lua_state.stde, "#GTV.invalidParams");
    return 0;
  }

  int32_t param[1] = {0};
  // uint8_t isgetter = 0;

  for (int i = 1; i <= nargs; ++i) {

    if (lua_isnumber(L, i)) {
      param[i - 1] = lua_tointeger(L, i);
    }
  }

  grid_sys_set_midirx_any_state(&grid_sys_state, (uint8_t)param[0]);

  return 1;
}

/*static*/ int l_grid_midirx_sync(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 1) {
    // error
    strcat(grid_lua_state.stde, "#GTV.invalidParams");
    return 0;
  }

  int32_t param[1] = {0};
  // uint8_t isgetter = 0;

  for (int i = 1; i <= nargs; ++i) {

    if (lua_isnumber(L, i)) {
      param[i - 1] = lua_tointeger(L, i);
    }
  }

  grid_sys_set_midirx_sync_state(&grid_sys_state, (uint8_t)param[0]);

  return 1;
}

/*static*/ int l_grid_midi_send(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 4) {
    // error
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  uint8_t param[4] = {0};

  for (int i = 1; i <= nargs; ++i) {
    param[i - 1] = lua_tointeger(L, i);
  }

  uint8_t channel = param[0];
  uint8_t command = param[1];
  uint8_t param1 = param[2];
  uint8_t param2 = param[3];

  char midiframe[15] = {0};

  sprintf(midiframe, GRID_CLASS_MIDI_frame);

  grid_str_set_parameter(midiframe, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code, NULL);

  grid_str_set_parameter(midiframe, GRID_CLASS_MIDI_CHANNEL_offset, GRID_CLASS_MIDI_CHANNEL_length, channel, NULL);
  grid_str_set_parameter(midiframe, GRID_CLASS_MIDI_COMMAND_offset, GRID_CLASS_MIDI_COMMAND_length, command, NULL);
  grid_str_set_parameter(midiframe, GRID_CLASS_MIDI_PARAM1_offset, GRID_CLASS_MIDI_PARAM1_length, param1, NULL);
  grid_str_set_parameter(midiframe, GRID_CLASS_MIDI_PARAM2_offset, GRID_CLASS_MIDI_PARAM2_length, param2, NULL);

  if (grid_lua_append_stdo(&grid_lua_state, midiframe)) {
    strcat(grid_lua_state.stde, "#stdoFull");
    return 0;
  }

  return 1;
}

/*static*/ int l_grid_midi_sysex_send(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs < 2) {
    // error
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  char midiframe[GRID_PARAMETER_SPI_TRANSACTION_length] = {0};

  sprintf(midiframe, GRID_CLASS_MIDISYSEX_frame_start);

  grid_str_set_parameter(midiframe, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code, NULL);

  int i;
  for (i = 1; i <= nargs; ++i) {

    grid_str_set_parameter(midiframe, GRID_CLASS_MIDISYSEX_PAYLOAD_offset + i * 2 - 2, 2, lua_tointeger(L, i), NULL);
  }

  grid_str_set_parameter(midiframe, GRID_CLASS_MIDISYSEX_LENGTH_offset, GRID_CLASS_MIDISYSEX_LENGTH_length, i - 1, NULL);

  sprintf(&midiframe[strlen(midiframe)], GRID_CLASS_MIDISYSEX_frame_end);

  // grid_platform_printf("MIDI: %s\r\n", midiframe);
  strcat(grid_lua_state.stdo, midiframe);

  return 1;
}

/*static*/ int l_grid_led_layer_min(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 5 && nargs != 6) {
    // error
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  if (lua_isnil(L, 1)) {
    return 0;
  }

  uint8_t num, layer, red, green, blue;

  uint8_t* param[5] = {&num, &layer, &red, &green, &blue};

  for (int i = 0; i < 5; ++i) {
    *(param[i]) = lua_tointeger(L, i + 1);
  }

  if (nargs == 6) {
    double alpha = lua_tonumber(L, nargs);
    if (alpha > 1.0) {
      alpha = 1.0;
    }
    if (alpha < 0.0) {
      alpha = 0.0;
    }

    red = (uint8_t)(red * alpha);
    green = (uint8_t)(green * alpha);
    blue = (uint8_t)(blue * alpha);
  }

  grid_led_set_layer_min(&grid_led_state, num, layer, red, green, blue);

  return 0;
}

/*static*/ int l_grid_led_layer_mid(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 5 && nargs != 6) {
    // error
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  if (lua_isnil(L, 1)) {
    return 0;
  }

  uint8_t num, layer, red, green, blue;

  uint8_t* param[5] = {&num, &layer, &red, &green, &blue};

  for (int i = 0; i < 5; ++i) {
    *(param[i]) = lua_tointeger(L, i + 1);
  }

  if (nargs == 6) {
    double alpha = lua_tonumber(L, nargs);
    if (alpha > 1.0) {
      alpha = 1.0;
    }
    if (alpha < 0.0) {
      alpha = 0.0;
    }

    red = (uint8_t)(red * alpha);
    green = (uint8_t)(green * alpha);
    blue = (uint8_t)(blue * alpha);
  }

  grid_led_set_layer_mid(&grid_led_state, num, layer, red, green, blue);

  return 0;
}

/*static*/ int l_grid_led_layer_max(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 5 && nargs != 6) {
    // error
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  if (lua_isnil(L, 1)) {
    return 0;
  }

  uint8_t num, layer, red, green, blue;

  uint8_t* param[5] = {&num, &layer, &red, &green, &blue};

  for (int i = 0; i < 5; ++i) {
    *(param[i]) = lua_tointeger(L, i + 1);
  }

  if (nargs == 6) {
    double alpha = lua_tonumber(L, nargs);
    if (alpha > 1.0) {
      alpha = 1.0;
    }
    if (alpha < 0.0) {
      alpha = 0.0;
    }

    red = (uint8_t)(red * alpha);
    green = (uint8_t)(green * alpha);
    blue = (uint8_t)(blue * alpha);
  }

  grid_led_set_layer_max(&grid_led_state, num, layer, red, green, blue);

  return 0;
}

/*static*/ int l_grid_led_layer_color(lua_State* L) {

  int nargs = lua_gettop(L);

  if (lua_isnil(L, 1)) {
    return 0;
  }

  if (nargs == 5) {

    uint8_t param[5] = {0};

    for (int i = 1; i <= nargs; ++i) {
      param[i - 1] = lua_tointeger(L, i);
    }

    grid_led_set_layer_color(&grid_led_state, param[0], param[1], param[2], param[3], param[4]);
  } else if (nargs == 6) {

    uint8_t param[6] = {0};

    for (int i = 1; i <= nargs; ++i) {
      if (lua_isnil(L, i)) {
        param[i - 1] = 0;
      } else {
        param[i - 1] = lua_tointeger(L, i);
      }
    }

    grid_led_set_layer_color(&grid_led_state, param[0], param[1], param[2], param[3], param[4]);
    if (param[5] != 0) {
      grid_led_set_layer_min(&grid_led_state, param[0], param[1], 0, 0, 0);
    }
  } else {
    // error
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  return 0;
}

/*static*/ int l_grid_led_layer_frequency(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 3) {
    // error
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  if (lua_isnil(L, 1)) {
    return 0;
  }

  uint8_t param[3] = {0};

  for (int i = 1; i <= nargs; ++i) {
    param[i - 1] = lua_tointeger(L, i);
  }

  grid_led_set_layer_frequency(&grid_led_state, param[0], param[1], param[2]);
  return 0;
}

/*static*/ int l_grid_led_layer_shape(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 3) {
    // error
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  if (lua_isnil(L, 1)) {
    return 0;
  }

  uint8_t param[3] = {0};

  for (int i = 1; i <= nargs; ++i) {
    param[i - 1] = lua_tointeger(L, i);
  }

  // grid_platform_printf("Led shape %d %d %d\r\n", param[0], param[1],
  // param[2]);

  grid_led_set_layer_shape(&grid_led_state, param[0], param[1], param[2]);

  return 0;
}

/*static*/ int l_grid_led_layer_timeout(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 3) {
    // error
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  if (lua_isnil(L, 1)) {
    return 0;
  }

  uint16_t param[3] = {0};

  for (int i = 1; i <= nargs; ++i) {
    param[i - 1] = lua_tointeger(L, i);
  }

  grid_led_set_layer_timeout(&grid_led_state, (uint8_t)param[0], (uint8_t)param[1], param[2]);
  return 0;
}

/*static*/ int l_led_default_red(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 0 && nargs != 1) {
    // error
    strcat(grid_lua_state.stde, "#LED.invalidParams");
    return 0;
  }

  uint8_t param[1] = {0};

  uint8_t isgetter = 1;

  for (int i = 1; i <= nargs; ++i) {

    if (lua_isinteger(L, i)) {
      isgetter = 0;
    } else if (lua_isnil(L, i)) {
      // grid_platform_printf(" %d : NIL ", i);
      if (i == 1) {
        isgetter = 1;
      }
    }

    param[i - 1] = lua_tointeger(L, i);
  }

  if (isgetter) {

    int32_t var = grid_sys_get_bank_red(&grid_sys_state);
    lua_pushinteger(L, var);
  } else {

    int32_t var = param[0];
    grid_sys_set_bank_red(&grid_sys_state, var);
  }

  return 1;
}
/*static*/ int l_led_default_green(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 0 && nargs != 1) {
    // error
    strcat(grid_lua_state.stde, "#LED.invalidParams");
    return 0;
  }

  uint8_t param[1] = {0};

  uint8_t isgetter = 1;

  for (int i = 1; i <= nargs; ++i) {

    if (lua_isinteger(L, i)) {
      isgetter = 0;
    } else if (lua_isnil(L, i)) {
      // grid_platform_printf(" %d : NIL ", i);
      if (i == 1) {
        isgetter = 1;
      }
    }

    param[i - 1] = lua_tointeger(L, i);
  }

  if (isgetter) {

    int32_t var = grid_sys_get_bank_gre(&grid_sys_state);
    lua_pushinteger(L, var);
  } else {

    int32_t var = param[0];
    grid_sys_set_bank_gre(&grid_sys_state, var);
  }

  return 1;
}
/*static*/ int l_led_default_blue(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 0 && nargs != 1) {
    // error
    strcat(grid_lua_state.stde, "#LED.invalidParams");
    return 0;
  }

  uint8_t param[1] = {0};

  uint8_t isgetter = 1;

  for (int i = 1; i <= nargs; ++i) {

    if (lua_isinteger(L, i)) {
      isgetter = 0;
    } else if (lua_isnil(L, i)) {
      // grid_platform_printf(" %d : NIL ", i);
      if (i == 1) {
        isgetter = 1;
      }
    }

    param[i - 1] = lua_tointeger(L, i);
  }

  if (isgetter) {

    int32_t var = grid_sys_get_bank_blu(&grid_sys_state);
    lua_pushinteger(L, var);
  } else {

    int32_t var = param[0];
    grid_sys_set_bank_blu(&grid_sys_state, var);
  }

  return 1;
}

/*static*/ int l_grid_led_address_get(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 2) {
    // error
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  if (!lua_isinteger(L, 1)) {
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  int32_t element = lua_tointeger(L, 1);

  if (!lua_isinteger(L, 2)) {
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  int32_t subidx = lua_tointeger(L, 2);

  uint8_t length = 0;
  uint8_t* lookup = grid_led_lookup_get(&grid_led_state, element, &length);

  if (lookup == NULL || subidx >= length) {
    lua_pushnil(L);
    return 1;
  }

  lua_pushinteger(L, lookup[subidx]);

  return 1;
}

/*static*/ int l_grid_version_major(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 0) {
    // error
    strcat(grid_lua_state.stde, "#GTV.invalidParams");
    return 0;
  }

  lua_pushinteger(L, GRID_PROTOCOL_VERSION_MAJOR);

  return 1;
}

/*static*/ int l_grid_version_minor(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 0) {
    // error
    strcat(grid_lua_state.stde, "#GTV.invalidParams");
    return 0;
  }

  lua_pushinteger(L, GRID_PROTOCOL_VERSION_MINOR);

  return 1;
}

/*static*/ int l_grid_version_patch(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 0) {
    // error
    strcat(grid_lua_state.stde, "#GTV.invalidParams");
    return 0;
  }

  lua_pushinteger(L, GRID_PROTOCOL_VERSION_PATCH);

  return 1;
}

/*static*/ int l_grid_hwcfg(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 0) {
    // error
    strcat(grid_lua_state.stde, "#GTV.invalidParams");
    return 0;
  }

  lua_pushinteger(L, grid_sys_get_hwcfg(&grid_sys_state));

  return 1;
}

/*static*/ int l_grid_random8(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 0) {
    // error
    strcat(grid_lua_state.stde, "#GTV.invalidParams");
    return 0;
  }

  uint8_t random = grid_platform_get_random_8();

  lua_pushinteger(L, random);

  return 1;
}

/*static*/ int l_grid_position_x(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 0) {
    // error
    strcat(grid_lua_state.stde, "#GTV.invalidParams");
    return 0;
  }

  lua_pushinteger(L, grid_sys_get_module_x(&grid_sys_state));

  return 1;
}

/*static*/ int l_grid_position_y(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 0) {
    // error
    strcat(grid_lua_state.stde, "#GTV.invalidParams");
    return 0;
  }

  lua_pushinteger(L, grid_sys_get_module_y(&grid_sys_state));

  return 1;
}

/*static*/ int l_grid_rotation(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 0) {
    // error
    strcat(grid_lua_state.stde, "#GTV.invalidParams");
    return 0;
  }

  lua_pushinteger(L, grid_sys_get_module_rot(&grid_sys_state));

  return 1;
}

/*static*/ int l_grid_led_layer_phase(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 2 && nargs != 3) {
    // error
    strcat(grid_lua_state.stde, "#GTV.invalidParams");
    return 0;
  }

  if (lua_isnil(L, 1)) {
    return 0;
  }

  int32_t param[3] = {0};
  // uint8_t isgetter = 0;

  for (int i = 1; i <= nargs; ++i) {

    if (lua_isnumber(L, i)) {
      param[i - 1] = lua_tointeger(L, i);
    }
  }

  if (nargs == 3) {
    // setter

    if (param[2] > 255)
      param[2] = 255;
    if (param[2] > -1) { // phase is a phase value
      grid_led_set_layer_phase(&grid_led_state, param[0], param[1], param[2]);
    } else { // phase == -1 means it should be automatically calculated based on
             // min-max values

      uint8_t num = param[0];
      uint8_t layer = param[1];

      struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, num);
      if (ele == NULL) {
        return 0;
      }
      uint8_t ele_type = ele->type;

      int32_t min = 0;
      int32_t max = 0;
      int32_t val = 0;

      if (ele_type == GRID_PARAMETER_ELEMENT_POTMETER) {

        min = grid_ui_element_get_template_parameter(ele, GRID_LUA_FNC_P_POTMETER_MIN_index);
        max = grid_ui_element_get_template_parameter(ele, GRID_LUA_FNC_P_POTMETER_MAX_index);
        val = grid_ui_element_get_template_parameter(ele, GRID_LUA_FNC_P_POTMETER_VALUE_index);
      } else if (ele_type == GRID_PARAMETER_ELEMENT_ENCODER) {

        if (layer == 1) {
          // button layer
          min = grid_ui_element_get_template_parameter(ele, GRID_LUA_FNC_B_BUTTON_MIN_index);
          max = grid_ui_element_get_template_parameter(ele, GRID_LUA_FNC_B_BUTTON_MAX_index);
          val = grid_ui_element_get_template_parameter(ele, GRID_LUA_FNC_B_BUTTON_VALUE_index);
        } else {
          min = grid_ui_element_get_template_parameter(ele, GRID_LUA_FNC_E_ENCODER_MIN_index);
          max = grid_ui_element_get_template_parameter(ele, GRID_LUA_FNC_E_ENCODER_MAX_index);
          val = grid_ui_element_get_template_parameter(ele, GRID_LUA_FNC_E_ENCODER_VALUE_index);
        }
      } else if (ele_type == GRID_PARAMETER_ELEMENT_ENDLESS) {
        min = grid_ui_element_get_template_parameter(ele, GRID_LUA_FNC_EP_ENDLESS_MIN_index);
        max = grid_ui_element_get_template_parameter(ele, GRID_LUA_FNC_EP_ENDLESS_MAX_index);
        val = grid_ui_element_get_template_parameter(ele, GRID_LUA_FNC_EP_ENDLESS_VALUE_index);
      } else if (ele_type == GRID_PARAMETER_ELEMENT_BUTTON) {

        min = grid_ui_element_get_template_parameter(ele, GRID_LUA_FNC_B_BUTTON_MIN_index);
        max = grid_ui_element_get_template_parameter(ele, GRID_LUA_FNC_B_BUTTON_MAX_index);
        val = grid_ui_element_get_template_parameter(ele, GRID_LUA_FNC_B_BUTTON_VALUE_index);
      } else {

        strcat(grid_lua_state.stde, "#elementNotSupported");
        return 0;
      }

      uint16_t phase = grid_utility_map(val, min, max, 0, 255);
      // grid_platform_printf("LED: %d\r\n", phase);
      grid_led_set_layer_phase(&grid_led_state, param[0], param[1], phase);
    }
  } else {
    // getter
    int32_t var = grid_led_get_layer_phase(&grid_led_state, param[0], param[1]);
    lua_pushinteger(L, var);
  }

  return 1;
}

/*static*/ int l_grid_led_layer_pfs(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 5) {
    // error
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  if (lua_isnil(L, 1)) {
    return 0;
  }

  uint8_t param[5] = {0};

  for (int i = 1; i <= nargs; ++i) {
    param[i - 1] = lua_tointeger(L, i);
  }

  // grid_platform_printf("Led shape %d %d %d\r\n", param[0], param[1],
  // param[2]);

  grid_led_set_layer_phase(&grid_led_state, param[0], param[1], param[2]);
  grid_led_set_layer_frequency(&grid_led_state, param[0], param[1], param[3]);
  grid_led_set_layer_shape(&grid_led_state, param[0], param[1], param[4]);

  return 0;
}

/*static*/ int l_grid_template_variable(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 2 && nargs != 3) {
    // error
    strcat(grid_lua_state.stde, "#GTV.invalidParams");
    return 0;
  }

  int32_t param[3] = {0};

  uint8_t isgetter = 0;

  for (int i = 1; i <= nargs; ++i) {

    if (lua_isinteger(L, i)) {
    } else if (lua_isnil(L, i)) {
      // grid_platform_printf(" %d : NIL ", i);
      if (i == 3) {
        isgetter = 1;
      }
    }

    param[i - 1] = lua_tointeger(L, i);
  }
  // lua_pop(L, 2);

  struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, param[0]);

  if (ele != NULL) {

    uint8_t template_index = param[1];

    if (isgetter) {

      int32_t var = grid_ui_element_get_template_parameter(ele, template_index);
      lua_pushinteger(L, var);
    } else {

      int32_t var = param[2];
      grid_ui_element_set_template_parameter(ele, template_index, var);
    }
  }

  return 1;
}

/*static*/ int l_grid_page_next(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 0) {
    // error
    strcat(grid_lua_state.stde, "#GTV.invalidParams");
    return 0;
  }

  uint8_t page = grid_ui_page_get_next(&grid_ui_state);
  lua_pushinteger(L, page);

  return 1;
}

/*static*/ int l_grid_page_prev(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 0) {
    // error
    strcat(grid_lua_state.stde, "#GTV.invalidParams");
    return 0;
  }

  uint8_t page = grid_ui_page_get_prev(&grid_ui_state);
  lua_pushinteger(L, page);

  return 1;
}

/*static*/ int l_grid_page_curr(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 0) {
    // error
    strcat(grid_lua_state.stde, "#GTV.invalidParams");
    return 0;
  }

  uint8_t page = grid_ui_page_get_activepage(&grid_ui_state);
  lua_pushinteger(L, page);

  return 1;
}
/*static*/ int l_grid_page_load(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 1) {
    // error
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  uint8_t param[1] = {0};

  for (int i = 1; i <= nargs; ++i) {
    param[i - 1] = lua_tointeger(L, i);
  }

  uint8_t page = param[0];

  if (grid_ui_page_change_is_enabled(&grid_ui_state)) {

    if (grid_ui_bulk_is_in_progress(&grid_ui_state, GRID_UI_BULK_READ_PROGRESS) == 0) {

      grid_port_debug_printf("page request: %d", page);
      char response[20] = {0};
      sprintf(response, GRID_CLASS_PAGEACTIVE_frame);
      grid_str_set_parameter(response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code, NULL);
      grid_str_set_parameter(response, GRID_CLASS_PAGEACTIVE_PAGENUMBER_offset, GRID_CLASS_PAGEACTIVE_PAGENUMBER_length, page, NULL);
      strcat(grid_lua_state.stdo, response);
    } else {
      // grid_platform_printf("page change in progress \r\n");
    }
  } else {
    // grid_platform_printf("page change is disabled\r\n");
    grid_port_debug_printf("page change is disabled");
    grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_PURPLE, 64);
  }

  return 1;
}

/*static*/ int l_grid_timer_start(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 2) {
    // error
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  uint32_t param[2] = {0};

  for (int i = 1; i <= nargs; ++i) {
    param[i - 1] = lua_tointeger(L, i);
  }

  struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, param[0]);

  if (ele != NULL) {

    grid_ui_element_timer_set(ele, param[1]);
  } else {

    strcat(grid_lua_state.stde, "#invalidRange");
  }

  return 1;
}

/*static*/ int l_grid_timer_stop(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 1) {
    // error
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  uint32_t param[1] = {0};

  for (int i = 1; i <= nargs; ++i) {
    param[i - 1] = lua_tointeger(L, i);
  }

  struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, param[0]);

  if (ele != NULL) {

    grid_ui_element_timer_set(ele, 0);
  } else {

    strcat(grid_lua_state.stde, "#invalidRange");
  }

  return 1;
}

/*static*/ int l_grid_timer_source(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 2) {
    // error
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  uint32_t param[2] = {0};

  for (int i = 1; i <= nargs; ++i) {
    param[i - 1] = lua_tointeger(L, i);
  }

  struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, param[0]);

  if (ele != NULL) {

    grid_ui_element_timer_source(ele, param[1]);
  } else {

    strcat(grid_lua_state.stde, "#invalidRange");
  }

  return 1;
}

/*static*/ int l_grid_event_trigger(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 2) {
    // error
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  uint32_t param[2] = {0};

  for (int i = 1; i <= nargs; ++i) {
    param[i - 1] = lua_tointeger(L, i);
  }

  struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, param[0]);

  if (ele != NULL) {

    struct grid_ui_event* eve = grid_ui_event_find(ele, param[1]);

    if (eve != NULL) {

      grid_ui_event_state_set(eve, GRID_EVE_STATE_TRIG);
    } else {
      strcat(grid_lua_state.stde, "#invalidEvent");
      sprintf(&grid_lua_state.stde[strlen(grid_lua_state.stde) - 1], "%ld %ld", param[0], param[1]);
    }
  } else {

    strcat(grid_lua_state.stde, "#invalidRange");
  }

  return 1;
}

/*static*/ int l_grid_element_count(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 0) {
    // error
    strcat(grid_lua_state.stde, "#GTV.invalidParams");
    return 0;
  }

  uint8_t element_count = grid_ui_state.element_list_length;

  lua_pushinteger(L, element_count);

  return 1;
}

/*static*/ int l_grid_potmeter_calibration_get(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 0) {
    // error
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  struct grid_cal_pot* cal_pot = &grid_cal_state.potmeter;

  lua_newtable(L);

  for (uint8_t i = 0; i < grid_ui_state.element_list_length; ++i) {

    struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, i);

    uint8_t enabled = 0;
    if (grid_cal_pot_enable_get(cal_pot, i, &enabled) != 0) {

      strcat(grid_lua_state.stde, "#indexOutOfRange");
      lua_pop(L, 1);
      return 0;
    }

    uint16_t value = 0;

    if (ele->type == GRID_PARAMETER_ELEMENT_POTMETER && enabled) {

      if (grid_cal_pot_value_get(cal_pot, i, &value) != 0) {

        strcat(grid_lua_state.stde, "#indexOutOfRange");
        lua_pop(L, 1);
        return 0;
      }
    }

    lua_pushinteger(L, i + 1);
    lua_pushinteger(L, value);
    lua_settable(L, -3);
  }

  return 1;
}

/*static*/ int l_grid_potmeter_calibration_set(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 1) {
    // error
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  if (!lua_istable(L, -1)) {
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  struct grid_cal_pot* cal_pot = &grid_cal_state.potmeter;

  for (uint8_t i = 0; i < grid_ui_state.element_list_length; ++i) {

    struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, i);

    uint8_t enabled = 0;
    if (grid_cal_pot_enable_get(cal_pot, i, &enabled) != 0) {

      strcat(grid_lua_state.stde, "#indexOutOfRange");
      return 0;
    }

    if (!enabled) {
      continue;
    }

    lua_pushinteger(L, i + 1);
    lua_gettable(L, -2);

    if (!lua_isinteger(L, -1)) {
      strcat(grid_lua_state.stde, "#invalidParams");
      return 0;
    }

    int32_t value = 0;

    if (ele->type == GRID_PARAMETER_ELEMENT_POTMETER) {
      value = lua_tointeger(L, -1);
    }

    lua_pop(L, 1);

    if (grid_cal_pot_center_set(cal_pot, i, value) != 0) {

      strcat(grid_lua_state.stde, "#indexOutOfRange");
      return 0;
    }
  }

  grid_ui_bulk_conf_init(&grid_ui_state, GRID_UI_BULK_CONFSTORE_PROGRESS, 0, NULL);

  return 0;
}

/*static*/ int l_grid_potmeter_detent_set(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 2) {
    // error
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  if (!lua_istable(L, -2)) {
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  if (!lua_isboolean(L, -1)) {
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  bool high = lua_toboolean(L, -1);
  lua_pop(L, 1);

  struct grid_cal_pot* cal_pot = &grid_cal_state.potmeter;

  for (uint8_t i = 0; i < grid_ui_state.element_list_length; ++i) {

    struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, i);

    uint8_t enabled = 0;
    if (grid_cal_pot_enable_get(cal_pot, i, &enabled) != 0) {

      strcat(grid_lua_state.stde, "#indexOutOfRange");
      return 0;
    }

    if (!enabled) {
      continue;
    }

    lua_pushinteger(L, i + 1);
    lua_gettable(L, -2);

    if (!lua_isinteger(L, -1)) {
      strcat(grid_lua_state.stde, "#invalidParams");
      return 0;
    }

    int32_t value = 0;

    if (ele->type == GRID_PARAMETER_ELEMENT_POTMETER) {
      value = lua_tointeger(L, -1);
    }

    lua_pop(L, 1);

    if (grid_cal_pot_detent_set(cal_pot, i, value, high) != 0) {

      strcat(grid_lua_state.stde, "#indexOutOfRange");
      return 0;
    }
  }

  grid_ui_bulk_conf_init(&grid_ui_state, GRID_UI_BULK_CONFSTORE_PROGRESS, 0, NULL);

  return 0;
}

/*static*/ int l_grid_button_calibration_get(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 0) {
    // error
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  struct grid_cal_but* cal_but = &grid_cal_state.button;

  lua_newtable(L);

  for (uint8_t i = 0; i < grid_ui_state.element_list_length; ++i) {

    struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, i);

    uint8_t enabled = 0;
    if (grid_cal_but_enable_get(cal_but, i, &enabled) != 0) {

      strcat(grid_lua_state.stde, "#indexOutOfRange");
      lua_pop(L, 1);
      return 0;
    }

    uint16_t min = 0;
    uint16_t max = 0;

    if (ele->type == GRID_PARAMETER_ELEMENT_BUTTON && enabled) {

      if (grid_cal_but_minmax_get(cal_but, i, &min, &max) != 0) {

        strcat(grid_lua_state.stde, "#indexOutOfRange");
        lua_pop(L, 1);
        return 0;
      }
    }

    lua_pushinteger(L, i + 1);

    lua_createtable(L, 2, 0);

    lua_pushinteger(L, 1);
    lua_pushinteger(L, min);
    lua_settable(L, -3);

    lua_pushinteger(L, 2);
    lua_pushinteger(L, max);
    lua_settable(L, -3);

    lua_settable(L, -3);
  }

  return 1;
}

/*static*/ int l_grid_button_calibration_set(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 1) {
    // error
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  if (!lua_istable(L, -1)) {
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  struct grid_cal_but* cal_but = &grid_cal_state.button;

  for (uint8_t i = 0; i < grid_ui_state.element_list_length; ++i) {

    struct grid_ui_element* ele = grid_ui_element_find(&grid_ui_state, i);

    uint8_t enabled = 0;
    if (grid_cal_but_enable_get(cal_but, i, &enabled) != 0) {

      strcat(grid_lua_state.stde, "#indexOutOfRange");
      return 0;
    }

    if (!enabled) {
      continue;
    }

    lua_pushinteger(L, i + 1);
    lua_gettable(L, -2);

    if (!lua_istable(L, -1)) {
      strcat(grid_lua_state.stde, "#invalidParams");
      return 0;
    }

    lua_pushinteger(L, 1);
    lua_gettable(L, -2);

    if (!lua_isinteger(L, -1)) {
      strcat(grid_lua_state.stde, "#invalidParams");
      return 0;
    }

    int32_t min = lua_tointeger(L, -1);
    lua_pop(L, 1);

    grid_cal_but_min_set(cal_but, i, min);

    lua_pushinteger(L, 2);
    lua_gettable(L, -2);

    if (!lua_isinteger(L, -1)) {
      strcat(grid_lua_state.stde, "#invalidParams");
      return 0;
    }

    int32_t max = lua_tointeger(L, -1);
    lua_pop(L, 1);

    grid_cal_but_max_set(cal_but, i, max);

    lua_pop(L, 1);
  }

  grid_ui_bulk_conf_init(&grid_ui_state, GRID_UI_BULK_CONFSTORE_PROGRESS, 0, NULL);

  return 0;
}

/*static*/ int l_grid_lcd_set_backlight(lua_State* L) {

  int nargs = lua_gettop(L);

  if (nargs != 1) {
    // error
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  if (!lua_isinteger(L, -1)) {
    strcat(grid_lua_state.stde, "#invalidParams");
    return 0;
  }

  int32_t backlight = lua_tointeger(L, -1);
  lua_pop(L, 1);

  backlight = clampi32(backlight, 0, 255);

  grid_platform_lcd_set_backlight(backlight);

  return 0;
}

/*static*/ const struct luaL_Reg grid_lua_api_generic_lib[] = {
    {"print", l_my_print},
    {"grid_send", l_grid_send},

    {GRID_LUA_FNC_G_LED_RED_short, GRID_LUA_FNC_G_LED_RED_fnptr},
    {GRID_LUA_FNC_G_LED_GRE_short, GRID_LUA_FNC_G_LED_GRE_fnptr},
    {GRID_LUA_FNC_G_LED_BLU_short, GRID_LUA_FNC_G_LED_BLU_fnptr},

    {GRID_LUA_FNC_G_LED_PHASE_short, GRID_LUA_FNC_G_LED_PHASE_fnptr},
    {GRID_LUA_FNC_G_LED_MIN_short, GRID_LUA_FNC_G_LED_MIN_fnptr},
    {GRID_LUA_FNC_G_LED_MID_short, GRID_LUA_FNC_G_LED_MID_fnptr},
    {GRID_LUA_FNC_G_LED_MAX_short, GRID_LUA_FNC_G_LED_MAX_fnptr},
    {GRID_LUA_FNC_G_LED_COLOR_short, GRID_LUA_FNC_G_LED_COLOR_fnptr},
    {GRID_LUA_FNC_G_LED_FREQUENCY_short, GRID_LUA_FNC_G_LED_FREQUENCY_fnptr},
    {GRID_LUA_FNC_G_LED_SHAPE_short, GRID_LUA_FNC_G_LED_SHAPE_fnptr},
    {GRID_LUA_FNC_G_LED_TIMEOUT_short, GRID_LUA_FNC_G_LED_TIMEOUT_fnptr},
    {GRID_LUA_FNC_G_LED_PSF_short, GRID_LUA_FNC_G_LED_PSF_fnptr},
    {GRID_LUA_FNC_G_LED_ADDRESS_GET_short, GRID_LUA_FNC_G_LED_ADDRESS_GET_fnptr},

    {GRID_LUA_FNC_G_MIDI_SEND_short, GRID_LUA_FNC_G_MIDI_SEND_fnptr},
    {GRID_LUA_FNC_G_MIDISYSEX_SEND_short, GRID_LUA_FNC_G_MIDISYSEX_SEND_fnptr},

    {GRID_LUA_FNC_G_MIDIRX_ENABLED_short, GRID_LUA_FNC_G_MIDIRX_ENABLED_fnptr},
    {GRID_LUA_FNC_G_MIDIRX_SYNC_short, GRID_LUA_FNC_G_MIDIRX_SYNC_fnptr},

    {GRID_LUA_FNC_G_KEYBOARD_SEND_short, GRID_LUA_FNC_G_KEYBOARD_SEND_fnptr},

    {GRID_LUA_FNC_G_MOUSEMOVE_SEND_short, GRID_LUA_FNC_G_MOUSEMOVE_SEND_fnptr},
    {GRID_LUA_FNC_G_MOUSEBUTTON_SEND_short, GRID_LUA_FNC_G_MOUSEBUTTON_SEND_fnptr},

    {GRID_LUA_FNC_G_GAMEPADMOVE_SEND_short, GRID_LUA_FNC_G_GAMEPADMOVE_SEND_fnptr},
    {GRID_LUA_FNC_G_GAMEPADBUTTON_SEND_short, GRID_LUA_FNC_G_GAMEPADBUTTON_SEND_fnptr},

    {GRID_LUA_FNC_G_VERSION_MAJOR_short, GRID_LUA_FNC_G_VERSION_MAJOR_fnptr},
    {GRID_LUA_FNC_G_VERSION_MINOR_short, GRID_LUA_FNC_G_VERSION_MINOR_fnptr},
    {GRID_LUA_FNC_G_VERSION_PATCH_short, GRID_LUA_FNC_G_VERSION_PATCH_fnptr},

    {GRID_LUA_FNC_G_MODULE_POSX_short, GRID_LUA_FNC_G_MODULE_POSX_fnptr},
    {GRID_LUA_FNC_G_MODULE_POSY_short, GRID_LUA_FNC_G_MODULE_POSY_fnptr},
    {GRID_LUA_FNC_G_MODULE_ROT_short, GRID_LUA_FNC_G_MODULE_ROT_fnptr},

    {GRID_LUA_FNC_G_PAGE_NEXT_short, GRID_LUA_FNC_G_PAGE_NEXT_fnptr},
    {GRID_LUA_FNC_G_PAGE_PREV_short, GRID_LUA_FNC_G_PAGE_PREV_fnptr},
    {GRID_LUA_FNC_G_PAGE_CURR_short, GRID_LUA_FNC_G_PAGE_CURR_fnptr},
    {GRID_LUA_FNC_G_PAGE_LOAD_short, GRID_LUA_FNC_G_PAGE_LOAD_fnptr},

    {GRID_LUA_FNC_G_TIMER_START_short, GRID_LUA_FNC_G_TIMER_START_fnptr},
    {GRID_LUA_FNC_G_TIMER_STOP_short, GRID_LUA_FNC_G_TIMER_STOP_fnptr},
    {GRID_LUA_FNC_G_TIMER_SOURCE_short, GRID_LUA_FNC_G_TIMER_SOURCE_fnptr},

    {GRID_LUA_FNC_G_EVENT_TRIGGER_short, GRID_LUA_FNC_G_EVENT_TRIGGER_fnptr},

    {GRID_LUA_FNC_G_HWCFG_short, GRID_LUA_FNC_G_HWCFG_fnptr},

    {GRID_LUA_FNC_G_RANDOM_short, GRID_LUA_FNC_G_RANDOM_fnptr},
    {GRID_LUA_FNC_G_ELEMENTNAME_SEND_short, GRID_LUA_FNC_G_ELEMENTNAME_SEND_fnptr},
    {GRID_LUA_FNC_G_ELEMENTNAME_SET_short, GRID_LUA_FNC_G_ELEMENTNAME_SET_fnptr},
    {GRID_LUA_FNC_G_ELEMENTNAME_GET_short, GRID_LUA_FNC_G_ELEMENTNAME_GET_fnptr},
    {GRID_LUA_FNC_G_STRING_GET_short, GRID_LUA_FNC_G_STRING_GET_fnptr},

    {GRID_LUA_FNC_G_WEBSOCKET_SEND_short, GRID_LUA_FNC_G_WEBSOCKET_SEND_fnptr},
    {GRID_LUA_FNC_G_PACKAGE_SEND_short, GRID_LUA_FNC_G_PACKAGE_SEND_fnptr},
    {GRID_LUA_FNC_G_IMMEDIATE_SEND_short, GRID_LUA_FNC_G_IMMEDIATE_SEND_fnptr},

    {GRID_LUA_FNC_G_ELEMENT_COUNT_short, GRID_LUA_FNC_G_ELEMENT_COUNT_fnptr},

    {GRID_LUA_FNC_G_POTMETER_CALIBRATION_GET_short, GRID_LUA_FNC_G_POTMETER_CALIBRATION_GET_fnptr},
    {GRID_LUA_FNC_G_POTMETER_CALIBRATION_SET_short, GRID_LUA_FNC_G_POTMETER_CALIBRATION_SET_fnptr},
    {GRID_LUA_FNC_G_POTMETER_DETENT_SET_short, GRID_LUA_FNC_G_POTMETER_DETENT_SET_fnptr},
    {GRID_LUA_FNC_G_BUTTON_CALIBRATION_GET_short, GRID_LUA_FNC_G_BUTTON_CALIBRATION_GET_fnptr},
    {GRID_LUA_FNC_G_BUTTON_CALIBRATION_SET_short, GRID_LUA_FNC_G_BUTTON_CALIBRATION_SET_fnptr},

    {GRID_LUA_FNC_G_LCD_SET_BACKLIGHT_short, GRID_LUA_FNC_G_LCD_SET_BACKLIGHT_fnptr},

    {GRID_LUA_FNC_G_FILESYSTEM_LISTDIR_short, GRID_LUA_FNC_G_FILESYSTEM_LISTDIR_fnptr},
    {GRID_LUA_FNC_G_FILESYSTEM_CAT_short, GRID_LUA_FNC_G_FILESYSTEM_CAT_fnptr},
    {"print", l_my_print},

    {"gtv", l_grid_template_variable},

    {NULL, NULL} /* end of array */
};

const struct luaL_Reg* grid_lua_api_generic_lib_reference = grid_lua_api_generic_lib;
