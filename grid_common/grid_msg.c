/*
 * grid_msg.c
 *
 * Created: 9/23/2020 2:35:51 PM
 *  Author: suku
 */

#include "grid_msg.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

struct grid_msg_model grid_msg_state;

void grid_msg_init(struct grid_msg_model* msg) {

  msg->sessionid = grid_platform_get_random_8();
  msg->editor_heartbeat_lastrealtime = 0;
  msg->heartbeat_type = 0;
}

void grid_msg_set_heartbeat_type(struct grid_msg_model* msg, uint8_t type) { msg->heartbeat_type = type; }

uint8_t grid_msg_get_heartbeat_type(struct grid_msg_model* msg) { return msg->heartbeat_type; }

void grid_msg_set_editor_heartbeat_lastrealtime(struct grid_msg_model* msg, uint64_t timestamp) { msg->editor_heartbeat_lastrealtime = timestamp; }

uint32_t grid_msg_get_editor_heartbeat_lastrealtime(struct grid_msg_model* msg) { return msg->editor_heartbeat_lastrealtime; }

// ======================= GRID MSG LEN ======================//
void grid_msg_header_set_len(struct grid_msg_packet* packet, uint16_t len) {

  uint8_t error = 0;
  grid_str_set_parameter(packet->header, GRID_BRC_LEN_offset, GRID_BRC_LEN_length, len, &error);
}

uint16_t grid_msg_header_get_len(struct grid_msg_packet* packet) {

  uint8_t error = 0;
  return grid_str_get_parameter(packet->header, GRID_BRC_LEN_offset, GRID_BRC_LEN_length, &error);
}

// ======================= GRID MSG ID ======================//
void grid_msg_header_set_id(struct grid_msg_packet* packet, uint8_t id) {

  uint8_t error = 0;
  grid_str_set_parameter(packet->header, GRID_BRC_ID_offset, GRID_BRC_ID_length, id, &error);
}

uint8_t grid_msg_header_get_id(struct grid_msg_packet* packet) {

  uint8_t error = 0;
  return grid_str_get_parameter(packet->header, GRID_BRC_ID_offset, GRID_BRC_ID_length, &error);
}

// ======================= GRID MSG DX ======================//
void grid_msg_header_set_dx(struct grid_msg_packet* packet, uint8_t dx) {

  uint8_t error = 0;
  grid_str_set_parameter(packet->header, GRID_BRC_DX_offset, GRID_BRC_DX_length, dx, &error);
}

uint8_t grid_msg_header_get_dx(struct grid_msg_packet* packet) {

  uint8_t error = 0;
  return grid_str_get_parameter(packet->header, GRID_BRC_DX_offset, GRID_BRC_DX_length, &error);
}

// ======================= GRID MSG DY ======================//
void grid_msg_header_set_dy(struct grid_msg_packet* packet, uint8_t dy) {

  uint8_t error = 0;
  grid_str_set_parameter(packet->header, GRID_BRC_DY_offset, GRID_BRC_DY_length, dy, &error);
}

uint8_t grid_msg_header_get_dy(struct grid_msg_packet* packet) {

  uint8_t error = 0;
  return grid_str_get_parameter(packet->header, GRID_BRC_DY_offset, GRID_BRC_DY_length, &error);
}

// ======================= GRID MSG SX ======================//
void grid_msg_header_set_sx(struct grid_msg_packet* packet, uint8_t sx) {

  uint8_t error = 0;
  grid_str_set_parameter(packet->header, GRID_BRC_SX_offset, GRID_BRC_SX_length, sx, &error);
}

uint8_t grid_msg_header_get_sx(struct grid_msg_packet* packet) {

  uint8_t error = 0;
  return grid_str_get_parameter(packet->header, GRID_BRC_SX_offset, GRID_BRC_SX_length, &error);
}

// ======================= GRID MSG SY ======================//
void grid_msg_header_set_sy(struct grid_msg_packet* packet, uint8_t sy) {

  uint8_t error = 0;
  grid_str_set_parameter(packet->header, GRID_BRC_SY_offset, GRID_BRC_SY_length, sy, &error);
}

uint8_t grid_msg_header_get_sy(struct grid_msg_packet* packet) {

  uint8_t error = 0;
  return grid_str_get_parameter(packet->header, GRID_BRC_SY_offset, GRID_BRC_SY_length, &error);
}

// ======================= GRID MSG ROT ======================//
void grid_msg_header_set_rot(struct grid_msg_packet* packet, uint8_t rot) {

  uint8_t error = 0;
  grid_str_set_parameter(packet->header, GRID_BRC_ROT_offset, GRID_BRC_ROT_length, rot, &error);
}

uint8_t grid_msg_header_get_rot(struct grid_msg_packet* packet) {

  uint8_t error = 0;
  return grid_str_get_parameter(packet->header, GRID_BRC_ROT_offset, GRID_BRC_ROT_length, &error);
}

// ======================= GRID MSG AGE ======================//
void grid_msg_header_set_age(struct grid_msg_packet* packet, uint8_t age) {

  uint8_t error = 0;
  grid_str_set_parameter(packet->header, GRID_BRC_MSGAGE_offset, GRID_BRC_MSGAGE_length, age, &error);
}

uint8_t grid_msg_header_get_age(struct grid_msg_packet* packet) {

  uint8_t error = 0;
  return grid_str_get_parameter(packet->header, GRID_BRC_MSGAGE_offset, GRID_BRC_MSGAGE_length, &error);
}

// ======================= GRID MSG session ======================//
void grid_msg_header_set_session(struct grid_msg_packet* packet, uint8_t session) {

  uint8_t error = 0;
  grid_str_set_parameter(packet->header, GRID_BRC_SESSION_offset, GRID_BRC_SESSION_length, session, &error);
}

uint8_t grid_msg_header_get_session(struct grid_msg_packet* packet) {

  uint8_t error = 0;
  return grid_str_get_parameter(packet->header, GRID_BRC_SESSION_offset, GRID_BRC_SESSION_length, &error);
}

// ======================= MSG GET PACKET LENGTH ======================//
uint32_t grid_msg_packet_get_length(struct grid_msg_packet* packet) { return (packet->header_length + packet->body_length + packet->footer_length); }

// ======================= MSG GET HEADER LENGTH ======================//
uint32_t grid_msg_packet_header_get_length(struct grid_msg_packet* packet) { return (packet->header_length); }

// ======================= MSG GET BODY LENGTH ======================//
uint32_t grid_msg_packet_body_get_length(struct grid_msg_packet* packet) { return (packet->body_length); }

// ======================= MSG GET FOOTER LENGTH ======================//
uint32_t grid_msg_packet_footer_get_length(struct grid_msg_packet* packet) { return (packet->footer_length); }

void grid_msg_packet_body_append_text(struct grid_msg_packet* packet, char* str) {

  uint32_t len = strlen((char*)str);

  for (uint32_t i = 0; i < len; i++) {

    packet->body[packet->body_length + i] = str[i];
  }

  packet->body_length += len;
  packet->last_appended_length += len;
}

void grid_msg_packet_body_append_printf(struct grid_msg_packet* packet, char const* fmt, ...) {

  va_list ap;

  va_start(ap, fmt);

  vsprintf((char*)&packet->body[packet->body_length], fmt, ap);

  va_end(ap);

  packet->last_appended_length = strlen((char*)&packet->body[packet->body_length]);

  packet->body_length += packet->last_appended_length;

  assert(packet->body_length <= grid_msg_packet_body_maxlength);

  return;
}

int grid_msg_packet_body_append_nprintf(struct grid_msg_packet* packet, char const* fmt, ...) {

  va_list ap;

  va_start(ap, fmt);

  int remain = grid_msg_packet_body_maxlength - packet->body_length;

  int n = vsnprintf((char*)&packet->body[packet->body_length], remain, fmt, ap);

  va_end(ap);

  packet->last_appended_length = strlen((char*)&packet->body[packet->body_length]);

  packet->body_length += packet->last_appended_length;

  return n >= remain ? -1 : n;
}

void grid_msg_packet_body_append_parameter(struct grid_msg_packet* packet, uint8_t parameter_offset, uint8_t parameter_length, uint32_t value) {

  uint8_t text_start_offset = packet->body_length - packet->last_appended_length;

  grid_msg_packet_body_set_parameter(packet, text_start_offset, parameter_offset, parameter_length, value);
}

uint32_t grid_msg_packet_body_get_parameter(struct grid_msg_packet* packet, uint32_t text_start_offset, uint8_t parameter_offset, uint8_t parameter_length) {

  uint8_t error = 0;

  return grid_str_read_hex_string_value(&packet->body[text_start_offset + parameter_offset], parameter_length, &error);
}

void grid_msg_packet_body_set_parameter(struct grid_msg_packet* packet, uint32_t text_start_offset, uint8_t parameter_offset, uint8_t parameter_length, uint32_t value) {
  return grid_str_write_hex_string_value(&packet->body[text_start_offset + parameter_offset], parameter_length, value);
}

// ======================= MSG INIT HEADER======================//

void grid_msg_packet_init(struct grid_msg_model* msg, struct grid_msg_packet* packet, uint8_t dx, uint8_t dy) {

  packet->header_length = 0;
  packet->body_length = 0;
  packet->last_appended_length = 0;
  packet->footer_length = 0;

  // No need to initialize array

  // for (uint32_t i=0; i<GRID_MSG_HEADER_maxlength; i++)
  // {
  // 	msg->header[i] = 0;
  // }

  // for (uint32_t i=0; i<grid_msg_packet_body_maxlength; i++)
  // {
  // 	msg->body[i] = 0;
  // }

  // for (uint32_t i=0; i<GRID_MSG_FOOTER_maxlength; i++)
  // {
  // 	msg->footer[i] = 0;
  // }

  uint8_t session = msg->sessionid;

  sprintf((char*)packet->header, GRID_BRC_frame_quick, GRID_CONST_SOH, GRID_CONST_BRC, session, dx, dy, GRID_CONST_EOB);
  packet->header_length = strlen((char*)packet->header);

  // grid_msg_header_set_dx(msg, dx);
  // grid_msg_header_set_dy(msg, dy);

  // grid_msg_header_set_sx(msg, GRID_SYS_DEFAULT_POSITION);
  // grid_msg_header_set_sy(msg, GRID_SYS_DEFAULT_POSITION);
  // grid_msg_header_set_rot(msg, rot);
  // grid_msg_header_set_age(msg, GRID_SYS_DEFAULT_AGE);

  // grid_msg_header_set_session(msg, session);
}

void grid_msg_packet_to_chunk(struct grid_msg_packet* packet, char* chunk) {

  uint16_t length = grid_msg_packet_get_length(packet);

  for (uint16_t i = 0; i < length; i++) {

    chunk[i] = grid_msg_packet_send_char_by_char(packet, i);
  }

  chunk[length] = '\0';
}

// ======================= MSG RECEIVE CHAR ======================//
void grid_msg_packet_receive_char_by_char(struct grid_msg_packet* packet, uint8_t nextchar) {

  if (packet->body_length == 0) {

    if (nextchar != GRID_CONST_EOB) {
      packet->header[packet->header_length] = nextchar;
      packet->header_length++;
    } else {
      packet->body[packet->body_length] = nextchar;
      packet->body_length++;
    }
  } else if (packet->footer_length == 0) {

    if (nextchar != GRID_CONST_EOT) {
      packet->body[packet->body_length] = nextchar;
      packet->body_length++;
    } else {
      packet->footer[packet->footer_length] = nextchar;
      packet->footer_length++;
    }
  } else {

    packet->footer[packet->footer_length] = nextchar;
    packet->footer_length++;
  }
}

// ======================= GRID MSG SEND CHAR ======================//

uint8_t grid_msg_packet_send_char_by_char(struct grid_msg_packet* packet, uint32_t charindex) {

  if (charindex < packet->header_length) {

    return packet->header[charindex];
  } else if (charindex < packet->body_length + packet->header_length) {

    return packet->body[charindex - packet->header_length];
  } else if (charindex < packet->footer_length + packet->body_length + packet->header_length) {

    return packet->footer[charindex - packet->header_length - packet->body_length];
  } else {
    // OVERRUN
    return -1;
  }
}

void grid_msg_packet_close(struct grid_msg_model* msg, struct grid_msg_packet* packet) {

  sprintf((char*)&packet->footer[packet->footer_length], "%c", GRID_CONST_EOT);
  packet->footer_length += strlen((char*)&packet->footer[packet->footer_length]);

  grid_msg_header_set_len(packet, packet->header_length + packet->body_length + packet->footer_length);
  grid_msg_header_set_session(packet, msg->sessionid);
  grid_msg_header_set_id(packet, msg->next_broadcast_message_id);

  msg->next_broadcast_message_id++;

  uint8_t checksum = 0;

  for (uint32_t i = 0; i < packet->header_length; i++) {
    checksum ^= packet->header[i];
  }

  for (uint32_t i = 0; i < packet->body_length; i++) {
    checksum ^= packet->body[i];
  }

  for (uint32_t i = 0; i < packet->footer_length; i++) {
    checksum ^= packet->footer[i];
  }

  sprintf((char*)&packet->footer[packet->footer_length], "%02x\n", checksum);
  packet->footer_length += strlen((char*)&packet->footer[packet->footer_length]);
}

// RECENT MESSAGES

void grid_msg_recent_fingerprint_buffer_init(struct grid_msg_recent_buffer* rec, uint8_t length) {

  assert(length % 8 == 0);

  rec->fingerprint_array_length = length;
  rec->fingerprint_array_index = 0;

  rec->fingerprint_array = (grid_fingerprint_t*)grid_platform_allocate_volatile(rec->fingerprint_array_length * sizeof(grid_fingerprint_t));
  memset(rec->fingerprint_array, 0, rec->fingerprint_array_length);
}

grid_fingerprint_t grid_msg_recent_fingerprint_calculate(char* message) {

  uint8_t error = 0;

  uint8_t received_id = grid_str_get_parameter(message, GRID_BRC_ID_offset, GRID_BRC_ID_length, &error);
  uint8_t received_session = grid_str_get_parameter(message, GRID_BRC_SESSION_offset, GRID_BRC_SESSION_length, &error);
  int8_t updated_sx = grid_str_get_parameter(message, GRID_BRC_SX_offset, GRID_BRC_SX_length, &error) - GRID_PARAMETER_DEFAULT_POSITION;
  int8_t updated_sy = grid_str_get_parameter(message, GRID_BRC_SY_offset, GRID_BRC_SY_length, &error) - GRID_PARAMETER_DEFAULT_POSITION;

  grid_fingerprint_t fingerprint = received_id * 256 * 256 * 256 + updated_sx * 256 * 256 + updated_sy * 256 + received_session;

  return fingerprint;
}

uint8_t grid_msg_recent_fingerprint_find(struct grid_msg_recent_buffer* rec, grid_fingerprint_t fingerprint) {

  for (uint8_t i = 0; i < rec->fingerprint_array_length; i += 8) {

    int ret = rec->fingerprint_array[i + 0] == fingerprint || rec->fingerprint_array[i + 1] == fingerprint || rec->fingerprint_array[i + 2] == fingerprint ||
              rec->fingerprint_array[i + 3] == fingerprint || rec->fingerprint_array[i + 4] == fingerprint || rec->fingerprint_array[i + 5] == fingerprint ||
              rec->fingerprint_array[i + 6] == fingerprint || rec->fingerprint_array[i + 7] == fingerprint;

    if (ret) {
      return 1;
    }
  }

  return 0;
}

void grid_msg_recent_fingerprint_store(struct grid_msg_recent_buffer* rec, grid_fingerprint_t fingerprint) {

  rec->fingerprint_array_index += 1;
  rec->fingerprint_array_index %= rec->fingerprint_array_length;

  rec->fingerprint_array[rec->fingerprint_array_index] = fingerprint;
}

uint8_t grid_str_calculate_checksum_of_packet_string(char* str, uint32_t length) {

  uint8_t checksum = 0;
  for (uint32_t i = 0; i < length - 3; i++) {
    checksum ^= str[i];
  }

  return checksum;
}

uint8_t grid_str_calculate_checksum_of_string(char* str, uint32_t length) {

  uint8_t checksum = 0;
  for (uint32_t i = 0; i < length; i++) {
    checksum ^= str[i];
  }

  return checksum;
}

uint8_t grid_str_checksum_read(char* str, uint32_t length) {
  uint8_t error_flag;
  return grid_str_read_hex_string_value(&str[length - 3], 2, &error_flag);
}

void grid_str_checksum_write(char* message, uint32_t length, uint8_t checksum) {

  // 	uint8_t checksum_string[4];
  //
  // 	sprintf((char*) checksum_string, "%02x", checksum);
  //
  // 	message[length-3] = checksum_string[0];
  // 	message[length-2] = checksum_string[1];

  grid_str_write_hex_string_value(&message[length - 3], 2, checksum);
}

// MESSAGE PARAMETER FUNCTIONS

uint32_t grid_str_get_parameter(char* message, uint16_t offset, uint8_t length, uint8_t* error) { return grid_str_read_hex_string_value(&message[offset], length, error); }

void grid_str_set_parameter(char* message, uint16_t offset, uint8_t length, uint32_t value, uint8_t* error) { grid_str_write_hex_string_value(&message[offset], length, value); }

uint8_t grid_str_read_hex_char_value(uint8_t ascii, uint8_t* error_flag) {

  uint8_t result = 0;

  if (ascii > 47 && ascii < 58) {
    result = ascii - 48;
  } else if (ascii > 96 && ascii < 103) {
    result = ascii - 97 + 10;
  } else {
    // wrong input
    if (error_flag != 0) {
      *error_flag = ascii;
    }
  }

  return result;
}

uint32_t grid_str_read_hex_string_value(char* start_location, uint8_t length, uint8_t* error_flag) {

  uint32_t result = 0;

  for (uint8_t i = 0; i < length; i++) {

    result += grid_str_read_hex_char_value(start_location[i], error_flag) << (length - i - 1) * 4;
  }

  return result;
}

void grid_str_write_hex_string_value(char* start_location, uint8_t size, uint32_t value) {

  uint8_t str[10];

  sprintf((char*)str, "%08lx", value);

  for (uint8_t i = 0; i < size; i++) {
    start_location[i] = str[8 - size + i];
  }
}

int grid_str_verify_frame(char* message, uint16_t length) {

  uint8_t error_flag = 0;

  // frame validator
  if (message[0] != GRID_CONST_SOH || message[length - 1] != GRID_CONST_LF) {
    return 1;
  }

  // minimum length at which a checksum can be calculated
  if (length < 4) {
    return 1;
  }

  // checksum validator
  uint8_t calculated_checksum = grid_str_calculate_checksum_of_packet_string(message, length);
  uint8_t received_checksum = grid_str_checksum_read(message, length);

  if (calculated_checksum != received_checksum) {
    // printf("C %d %d ", calculated_checksum, received_checksum);
    return 1;
  }

  // brc length parameter validator
  if (message[1] == GRID_CONST_BRC) {

    // BRC packets contain length parameter. Check this against actual string length in message

    uint16_t received_length = grid_str_read_hex_string_value(&message[GRID_BRC_LEN_offset], GRID_BRC_LEN_length, &error_flag);

    if (length - 3 != received_length) {

      // printf("L%d %d ", length-3, received_length);
      return 1;
    }
  }

  return 0;
}

uint32_t grid_str_set_segment_char(char* dest, uint8_t head_hexes, uint32_t size, char* buffer) {

  switch (head_hexes) {
  case 2:
    assert(size <= UINT8_MAX);
    break;
  case 4:
    assert(size <= UINT16_MAX);
    break;
  case 8:
    assert(size <= UINT32_MAX);
    break;
  default:
    assert(0);
  }

  grid_str_write_hex_string_value(dest, head_hexes, size);
  strcpy(dest + head_hexes, buffer);

  return head_hexes + size;
}

uint32_t grid_str_get_segment_char(char* src, uint8_t head_hexes, uint32_t max_size, char* buffer) {

  uint32_t size = grid_str_read_hex_string_value(src, head_hexes, NULL);

  if (size < max_size) {
    strncpy(buffer, src + head_hexes, size);
    buffer[size] = '\0';
  }

  return head_hexes + size;
}
