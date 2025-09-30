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

grid_fingerprint_t grid_fingerprint_calculate(const char* brc) {

  const uint8_t* msg = (const uint8_t*)brc;

  uint8_t recv_id = grid_msg_get_parameter_raw(msg, BRC_ID);
  uint8_t recv_sess = grid_msg_get_parameter_raw(msg, BRC_SESSION);

  int8_t updated_sx = grid_msg_get_parameter_raw(msg, BRC_SX) - GRID_PARAMETER_DEFAULT_POSITION;
  int8_t updated_sy = grid_msg_get_parameter_raw(msg, BRC_SY) - GRID_PARAMETER_DEFAULT_POSITION;

  return (recv_id << 24) + updated_sx * 256 * 256 + updated_sy * 256 + recv_sess;
}

void grid_fingerprint_buf_init(struct grid_fingerprint_buf* fpb, uint8_t capacity) {

  assert(capacity % 8 == 0);

  fpb->capa = capacity;
  fpb->write = 0;

  fpb->data = grid_platform_allocate_volatile(fpb->capa * sizeof(grid_fingerprint_t));
  assert(fpb->data);

  memset(fpb->data, 0, fpb->capa * sizeof(grid_fingerprint_t));
}

void grid_fingerprint_buf_store(struct grid_fingerprint_buf* fpb, grid_fingerprint_t f) {

  fpb->data[fpb->write] = f;
  fpb->write = (fpb->write + 1) % fpb->capa;
}

uint8_t grid_fingerprint_buf_find(struct grid_fingerprint_buf* fpb, grid_fingerprint_t f) {

  for (uint8_t i = 0; i < fpb->capa; i += 8) {

    int ret = fpb->data[i + 0] == f || fpb->data[i + 1] == f || fpb->data[i + 2] == f || fpb->data[i + 3] == f || fpb->data[i + 4] == f || fpb->data[i + 5] == f || fpb->data[i + 6] == f ||
              fpb->data[i + 7] == f;

    if (ret) {
      return 1;
    }
  }

  return 0;
}

struct grid_msg_model grid_msg_state;

void grid_msg_model_init(struct grid_msg_model* msg) {

  msg->sessionid = grid_platform_get_random_8();
  msg->editor_heartbeat_lastrealtime = 0;
  msg->heartbeat_type = 0;
}

void grid_msg_set_heartbeat_type(struct grid_msg_model* msg, uint8_t type) { msg->heartbeat_type = type; }

uint8_t grid_msg_get_heartbeat_type(struct grid_msg_model* msg) { return msg->heartbeat_type; }

void grid_msg_set_editor_heartbeat_lastrealtime(struct grid_msg_model* msg, uint64_t timestamp) { msg->editor_heartbeat_lastrealtime = timestamp; }

uint32_t grid_msg_get_editor_heartbeat_lastrealtime(struct grid_msg_model* msg) { return msg->editor_heartbeat_lastrealtime; }

void print_byte(uint8_t x) {

  grid_platform_printf("%02hhx ", x);
  for (uint8_t k = 0; k < 8; ++k) {
    uint8_t bit = x >> (7 - k) & 0x1;
    grid_platform_printf("%c", bit ? '1' : '0');
  }
  grid_platform_printf("\n");
}

void print_u32(uint8_t x) {

  grid_platform_printf("%02hhx ", x);
  for (uint8_t k = 0; k < 8; ++k) {
    uint8_t bit = x >> (7 - k) & 0x1;
    grid_platform_printf("%c", bit ? '1' : '0');
  }
  grid_platform_printf("\n");
}

uint8_t grid_frame_calculate_checksum_string(uint8_t* frame, size_t length) {

  /*
  uint8_t u8 = 0;

  for (size_t i = 0; i < length; ++i) {
          u8 ^= frame[i];
  }

  return u8;
  */

  uint8_t u8 = 0;
  size_t lead = (4 - ((size_t)frame) % 4) % 4;

  for (size_t i = 0; i < lead; ++i) {
    u8 ^= frame[i];
  }

  uint32_t u32 = 0;
  uint32_t* u32_src = (uint32_t*)((size_t)frame + lead);
  size_t u32_len = (length - lead) >> 2;

  for (size_t i = 0; i < u32_len; ++i) {
    u32 ^= u32_src[i];
  }

  u8 += u32 >> 24 ^ u32 >> 16 ^ u32 >> 8 ^ u32;

  for (size_t i = lead + u32_len * 4; i < length; ++i) {
    u8 ^= frame[i];
  }

  return u8;
}

uint8_t grid_frame_calculate_checksum_packet(uint8_t* frame, size_t length) {

  assert(length > 3);

  return grid_frame_calculate_checksum_string(frame, length - 3);
}

uint32_t grid_frame_get_parameter(const uint8_t* frame, uint16_t offset, uint8_t length) {

  static const uint8_t hex_to_uint8[256] = {
      ['0'] = 0, ['1'] = 1, ['2'] = 2, ['3'] = 3, ['4'] = 4, ['5'] = 5, ['6'] = 6, ['7'] = 7, ['8'] = 8, ['9'] = 9, ['a'] = 10, ['b'] = 11, ['c'] = 12, ['d'] = 13, ['e'] = 14, ['f'] = 15,
  };

  assert(offset + length <= GRID_MSG_BYTES);

  const char* src = (const char*)(frame + offset);

  uint32_t result = 0;

  for (uint32_t i = 0; i < length; ++i) {

    uint32_t idx = length - 1 - i;
    uint32_t u4 = hex_to_uint8[(unsigned char)src[idx]];
    result += u4 << (4 * i);
  }

  return result;
}

void grid_frame_set_parameter(uint8_t* frame, uint16_t offset, uint8_t length, uint32_t value) {

  static char uint4_to_hex[16] = {
      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
  };

  assert(offset + length <= GRID_MSG_BYTES);

  char hexes[8] = {
      uint4_to_hex[value >> 28 & 0xf], uint4_to_hex[value >> 24 & 0xf], uint4_to_hex[value >> 20 & 0xf], uint4_to_hex[value >> 16 & 0xf],
      uint4_to_hex[value >> 12 & 0xf], uint4_to_hex[value >> 8 & 0xf],  uint4_to_hex[value >> 4 & 0xf],  uint4_to_hex[value >> 0 & 0xf],
  };

  // grid_platform_printf("hexes: %c%c%c%c%c%c%c%c\n", hexes[0], hexes[1], hexes[2], hexes[3], hexes[4], hexes[5], hexes[6], hexes[7]);

  memcpy(frame + offset, hexes + (8 - length), length);
}

int grid_frame_get_segment_char(uint8_t* frame, uint16_t offset, uint8_t head_hexes, uint32_t max_size, char* buffer) {

  uint32_t length = grid_frame_get_parameter(frame, offset, head_hexes);

  if (length >= max_size) {
    return -1;
  }

  assert(offset + length <= GRID_MSG_BYTES);

  memcpy((uint8_t*)buffer, frame + offset + head_hexes, length);
  buffer[length] = '\0';

  return head_hexes + length;
}

uint8_t grid_str_checksum_get(const char* str, uint32_t length) { return grid_frame_get_parameter((const uint8_t*)str, length - 3, 2); }

void grid_str_checksum_set(char* str, uint32_t length, uint8_t checksum) { grid_frame_set_parameter((uint8_t*)str, length - 3, 2, checksum); }

int grid_frame_verify(const uint8_t* frame, uint16_t length) {

  assert(length <= GRID_MSG_BYTES);

  // Must start with start-of-header
  if (length < 1 || frame[0] != GRID_CONST_SOH) {
    return 1;
  }

  // Must end with a line feed
  if (length < 2 || frame[length - 1] != GRID_CONST_LF) {
    return 1;
  }

  // Minimum length at which checksum can be calculated
  if (length < 4) {
    return 1;
  }

  uint8_t checksum_calc = grid_frame_calculate_checksum_packet(frame, length);
  uint8_t checksum_recv = grid_str_checksum_get((const char*)frame, length);

  // Calculated and received checksums must match
  if (checksum_calc != checksum_recv) {
    return 1;
  }

  // The length presented in broadcast packets must be correct
  if (frame[1] == GRID_CONST_BRC) {

    // if (grid_str_length_get((const char*)frame) != length - 3) {
    if (grid_frame_get_parameter(frame, GRID_BRC_LEN_offset, GRID_BRC_LEN_length) != length - 3) {
      return 1;
    }
  }

  return 0;
}

void grid_msg_reset_offset(struct grid_msg* msg) { msg->offset = 0; }

void grid_msg_store_offset(struct grid_msg* msg) { msg->offset = msg->length; }

void grid_msg_set_offset(struct grid_msg* msg, uint32_t offset) { msg->offset = offset; }

int grid_msg_close(struct grid_msg* msg) {

  // As grid_msg_add_frame uses grid_msg_nprintf, which needs enough space for
  // null-termination, and this requirement matches the platform-specific
  // requirement that the SPI between ESP32 and RP2040 also needs nullterm,
  // successfully closing also means the message can be sent over that SPI
  int ret = grid_msg_add_frame(msg, GRID_FOOTER_frame);

  if (ret == 4) {

    uint8_t chksum = grid_frame_calculate_checksum_packet((uint8_t*)msg->data, msg->length);
    grid_str_checksum_set(msg->data, msg->length, chksum);
  }

  return ret;
}

void grid_msg_init_brc(struct grid_msg_model* model, struct grid_msg* msg, uint8_t dx, uint8_t dy) {

  msg->length = 0;

  uint8_t session = model->sessionid;

  grid_msg_nprintf(msg, GRID_BRC_frame_quick, GRID_CONST_SOH, GRID_CONST_BRC, session, dx, dy, GRID_CONST_EOB);

  grid_msg_store_offset(msg);
}

int grid_msg_close_brc(struct grid_msg_model* model, struct grid_msg* msg) {

  msg->offset = 0;

  grid_msg_set_parameter(msg, BRC_LEN, msg->length + 1); // +1 for later EOT
  grid_msg_set_parameter(msg, BRC_SESSION, model->sessionid);
  grid_msg_set_parameter(msg, BRC_ID, model->next_broadcast_message_id);

  ++model->next_broadcast_message_id;

  return grid_msg_close(msg);
}

int grid_msg_nprintf(struct grid_msg* msg, const char* fmt, ...) {

  va_list ap;

  va_start(ap, fmt);

  assert(msg->length <= GRID_MSG_BYTES);

  int remain = GRID_MSG_BYTES - (int)msg->length;

  int n = vsnprintf((char*)&msg->data[msg->length], remain, fmt, ap);

  va_end(ap);

  msg->length += n >= remain ? 0 : n;

  return n >= remain ? -1 : n;
}

int grid_msg_add_segment_char(struct grid_msg* msg, uint8_t head_hexes, uint32_t size, char* buffer) {

  assert(msg->length <= GRID_MSG_BYTES);

  int remain = GRID_MSG_BYTES - (int)msg->length;

  int buffer_len = strlen(buffer);

  if (remain < head_hexes + buffer_len) {
    return -1;
  }

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

  grid_frame_set_parameter((uint8_t*)msg->data, msg->length, head_hexes, size);
  msg->length += head_hexes;

  assert(grid_msg_nprintf(msg, "%.*s", size, buffer) >= 0);

  return head_hexes + size;
}

int grid_msg_add_debugtext(struct grid_msg* msg, const char* text) {

  uint32_t length_prev = msg->length;

  bool success = true;

  if (grid_msg_add_frame(msg, GRID_CLASS_DEBUGTEXT_frame_start) < 0) {
    goto grid_msg_add_debugtext_revert;
  }

  grid_msg_set_parameter(msg, INSTR, GRID_INSTR_EXECUTE_code);

  if (grid_msg_nprintf(msg, "%s", text) < 0) {
    goto grid_msg_add_debugtext_revert;
  }

  if (grid_msg_add_frame(msg, GRID_CLASS_DEBUGTEXT_frame_end) < 0) {
    goto grid_msg_add_debugtext_revert;
  }

  return 0;

grid_msg_add_debugtext_revert:
  msg->length = length_prev;
  return -1;
}

void grid_msg_to_swsr(struct grid_msg* msg, struct grid_swsr_t* swsr) {

  assert(grid_swsr_writable(swsr, msg->length));

  grid_swsr_write(swsr, msg->data, msg->length);
}

bool grid_msg_from_swsr(struct grid_msg* msg, struct grid_swsr_t* swsr) {

  int ret = grid_swsr_until_msg_end(swsr);

  if (ret < 0) {
    return false;
  }

  assert(grid_swsr_readable(swsr, ret + 1));
  assert(ret < GRID_MSG_BYTES);

  grid_swsr_read(swsr, msg->data, ret + 1);

  msg->length = ret + 1;
  grid_msg_reset_offset(msg);

  return true;
}

bool grid_msg_from_uwsr(struct grid_msg* msg, struct grid_uwsr_t* uwsr) {

  int ret = grid_uwsr_until_msg_end(uwsr);

  if (ret < 0) {
    return false;
  }

  assert(grid_uwsr_readable(uwsr, ret + 1));

  // If too many bytes are readable, skip them
  if (ret >= GRID_MSG_BYTES) {
    grid_uwsr_read(uwsr, NULL, ret + 1);
    return false;
  }

  grid_uwsr_read(uwsr, msg->data, ret + 1);

  msg->length = ret + 1;
  grid_msg_reset_offset(msg);

  return true;
}

bool grid_msg_pos_transformable(int8_t recv_x, int8_t recv_y) {

  int8_t x = recv_x + GRID_PARAMETER_DEFAULT_POSITION;
  int8_t y = recv_y + GRID_PARAMETER_DEFAULT_POSITION;

  // Editor-generated global message
  if (x == 0 && y == 0) {
    return false;
  }

  // Grid-generated global message
  if (x == -1 && y == -1) {
    return false;
  }

  return true;
}

void grid_str_transform_brc_params(uint8_t* msg, uint16_t length, int8_t dx, int8_t dy, uint8_t partner_rot) {

  assert(partner_rot < 4);

  if (msg[1] != GRID_CONST_BRC) {
    return;
  }

  // uint8_t recv_session = grid_msg_get_parameter_raw(msg, BRC_SESSION)
  uint8_t recv_age = grid_msg_get_parameter_raw(msg, BRC_MSGAGE);
  uint8_t new_age = recv_age + 1;

  uint8_t recv_dx = grid_msg_get_parameter_raw(msg, BRC_DX) - GRID_PARAMETER_DEFAULT_POSITION;
  uint8_t recv_dy = grid_msg_get_parameter_raw(msg, BRC_DY) - GRID_PARAMETER_DEFAULT_POSITION;

  uint8_t recv_sx = grid_msg_get_parameter_raw(msg, BRC_SX) - GRID_PARAMETER_DEFAULT_POSITION;
  uint8_t recv_sy = grid_msg_get_parameter_raw(msg, BRC_SY) - GRID_PARAMETER_DEFAULT_POSITION;

  uint8_t recv_rot = grid_msg_get_parameter_raw(msg, BRC_ROT);
  uint8_t new_rot = (recv_rot + partner_rot) % 4;

  int8_t sign_x[4] = {1, -1, -1, 1};
  int8_t sign_y[4] = {1, 1, -1, -1};

  uint8_t cross = partner_rot % 2;
  int8_t rot_dx = sign_x[partner_rot] * (recv_dx * !cross + recv_dy * cross);
  int8_t rot_dy = sign_y[partner_rot] * (recv_dy * !cross + recv_dx * cross);
  int8_t rot_sx = sign_x[partner_rot] * (recv_sx * !cross + recv_sy * cross);
  int8_t rot_sy = sign_y[partner_rot] * (recv_sy * !cross + recv_sx * cross);

  uint8_t new_dx = rot_dx + GRID_PARAMETER_DEFAULT_POSITION + dx;
  uint8_t new_dy = rot_dy + GRID_PARAMETER_DEFAULT_POSITION + dy;
  uint8_t new_sx = rot_sx + GRID_PARAMETER_DEFAULT_POSITION + dx;
  uint8_t new_sy = rot_sy + GRID_PARAMETER_DEFAULT_POSITION + dy;

  if (grid_msg_pos_transformable(recv_dx, recv_dy)) {

    grid_msg_set_parameter_raw(msg, BRC_DX, new_dx);
    grid_msg_set_parameter_raw(msg, BRC_DY, new_dy);
  }

  if (grid_msg_pos_transformable(recv_sx, recv_sy)) {

    grid_msg_set_parameter_raw(msg, BRC_SX, new_sx);
    grid_msg_set_parameter_raw(msg, BRC_SY, new_sy);
  }

  grid_msg_set_parameter_raw(msg, BRC_MSGAGE, new_age);
  grid_msg_set_parameter_raw(msg, BRC_ROT, new_rot);
  grid_msg_set_parameter_raw(msg, BRC_PORTROT, partner_rot);

  // Recalculate and update checksum
  uint8_t chksum = grid_frame_calculate_checksum_packet(msg, length);
  grid_str_checksum_set((char*)msg, length, chksum);
}
