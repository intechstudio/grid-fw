#ifndef GRID_MSG_H
#define GRID_MSG_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "grid_protocol.h"

extern void* grid_platform_allocate_volatile(size_t size);
extern uint8_t grid_platform_get_random_8();

#define grid_msg_packet_header_maxlength 26
#define GRID_MSG_FOOTER_maxlength 5

#define grid_msg_packet_body_maxlength GRID_PARAMETER_PACKET_maxlength - grid_msg_packet_header_maxlength - GRID_MSG_FOOTER_maxlength

typedef uint32_t grid_fingerprint_t;

struct grid_msg_recent_buffer {
  grid_fingerprint_t* fingerprint_array;
  uint8_t fingerprint_array_index;
  uint8_t fingerprint_array_length;
};

// RECENT FINGERPRINT FUNCTIONS

void grid_msg_recent_fingerprint_buffer_init(struct grid_msg_recent_buffer* rec, uint8_t length);
grid_fingerprint_t grid_msg_recent_fingerprint_calculate(char* message);
uint8_t grid_msg_recent_fingerprint_find(struct grid_msg_recent_buffer* rec, grid_fingerprint_t fingerprint);
void grid_msg_recent_fingerprint_store(struct grid_msg_recent_buffer* rec, grid_fingerprint_t fingerprint);

struct grid_msg_model {

  uint64_t editor_heartbeat_lastrealtime;
  uint8_t heartbeat_type;
  uint8_t sessionid;
  uint8_t next_broadcast_message_id;
};

extern struct grid_msg_model grid_msg_state;

struct grid_msg_packet {

  char header[grid_msg_packet_header_maxlength];
  char body[grid_msg_packet_body_maxlength];
  char footer[GRID_MSG_FOOTER_maxlength];

  uint32_t header_length;
  uint32_t body_length;
  uint32_t footer_length;

  uint32_t last_appended_length;
};

void grid_msg_init(struct grid_msg_model* msg);

void grid_msg_set_heartbeat_type(struct grid_msg_model* msg, uint8_t type);
uint8_t grid_msg_get_heartbeat_type(struct grid_msg_model* msg);

void grid_msg_set_editor_heartbeat_lastrealtime(struct grid_msg_model* msg, uint64_t timestamp);
uint32_t grid_msg_get_editor_heartbeat_lastrealtime(struct grid_msg_model* msg);

// PACKET FUNCTIONS

uint32_t grid_msg_packet_get_length(struct grid_msg_packet* packet);

void grid_msg_header_set_len(struct grid_msg_packet* packet, uint16_t len);
uint16_t grid_msg_packet_header_get_len(struct grid_msg_packet* packet);

void grid_msg_header_set_id(struct grid_msg_packet* packet, uint8_t id);
uint8_t grid_msg_packet_header_get_id(struct grid_msg_packet* packet);

void grid_msg_header_set_dx(struct grid_msg_packet* packet, uint8_t dx);
uint8_t grid_msg_packet_header_get_dx(struct grid_msg_packet* packet);

void grid_msg_header_set_dy(struct grid_msg_packet* packet, uint8_t dy);
uint8_t grid_msg_packet_header_get_dy(struct grid_msg_packet* packet);

void grid_msg_header_set_sx(struct grid_msg_packet* packet, uint8_t sx);
uint8_t grid_msg_packet_header_get_sx(struct grid_msg_packet* packet);

void grid_msg_header_set_sy(struct grid_msg_packet* packet, uint8_t sy);
uint8_t grid_msg_packet_header_get_sy(struct grid_msg_packet* packet);

void grid_msg_header_set_rot(struct grid_msg_packet* packet, uint8_t rot);
uint8_t grid_msg_packet_header_get_rot(struct grid_msg_packet* packet);

void grid_msg_header_set_age(struct grid_msg_packet* packet, uint8_t age);
uint8_t grid_msg_packet_header_get_age(struct grid_msg_packet* packet);

void grid_msg_header_set_session(struct grid_msg_packet* packet, uint8_t session);
uint8_t grid_msg_packet_header_get_session(struct grid_msg_packet* packet);

uint32_t grid_msg_packet_header_get_length(struct grid_msg_packet* packet);

uint32_t grid_msg_packet_body_get_length(struct grid_msg_packet* packet);
uint32_t grid_msg_packet_footer_get_length(struct grid_msg_packet* packet);

uint32_t grid_msg_packet_body_get_parameter(struct grid_msg_packet* packet, uint32_t text_start_offset, uint8_t parameter_offset, uint8_t parameter_length);
void grid_msg_packet_body_set_parameter(struct grid_msg_packet* packet, uint32_t text_start_offset, uint8_t parameter_offset, uint8_t parameter_length, uint32_t value);

void grid_msg_packet_body_append_text(struct grid_msg_packet* packet, char* string);

void grid_msg_packet_body_append_printf(struct grid_msg_packet* packet, char const* fmt, ...);
int grid_msg_packet_body_append_nprintf(struct grid_msg_packet* packet, char const* fmt, ...);
void grid_msg_packet_body_append_parameter(struct grid_msg_packet* packet, uint8_t parameter_offset, uint8_t parameter_length, uint32_t value);

void grid_msg_packet_to_chunk(struct grid_msg_packet* packet, char* chunk);
void grid_msg_packet_receive_char_by_char(struct grid_msg_packet* packet, uint8_t nextchar);

uint8_t grid_msg_packet_send_char_by_char(struct grid_msg_packet* packet, uint32_t charindex);

void grid_msg_packet_init(struct grid_msg_model* msg, struct grid_msg_packet* packet, uint8_t dx, uint8_t dy);
void grid_msg_packet_close(struct grid_msg_model* msg, struct grid_msg_packet* packet);

// STRING FUNCTIONS

uint32_t grid_str_get_parameter(char* message, uint16_t offset, uint8_t length, uint8_t* error);
void grid_str_set_parameter(char* message, uint16_t offset, uint8_t length, uint32_t value, uint8_t* error);

uint8_t grid_str_calculate_checksum_of_packet_string(char* str, uint32_t length);
uint8_t grid_str_calculate_checksum_of_string(char* str, uint32_t length);

uint8_t grid_str_checksum_read(char* str, uint32_t length);
void grid_str_checksum_write(char* message, uint32_t length, uint8_t checksum);

uint8_t grid_str_read_hex_char_value(uint8_t ascii, uint8_t* error_flag);
uint32_t grid_str_read_hex_string_value(char* start_location, uint8_t length, uint8_t* error_flag);
void grid_str_write_hex_string_value(char* start_location, uint8_t size, uint32_t value);

int grid_str_verify_frame(char* message, uint16_t length);

uint32_t grid_str_set_segment_char(char* dest, uint8_t head_bytes, uint32_t size, const char* buffer);
uint32_t grid_str_get_segment_char(char* src, uint8_t head_hexes, uint32_t max_size, char* buffer);

#endif /* GRID_MSG_H */
