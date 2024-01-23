/*
 * grid_msg.h
 *
 * Created: 9/23/2020 2:36:14 PM
 *  Author: suku
 */

#ifndef GRID_MSG_H_
#define GRID_MSG_H_

#include "grid_protocol.h"

// for variable argument lists
#include <stdarg.h>

// only for uint definitions
#include <stdint.h>

// for sprintf and strlen
#include <stdio.h>
#include <string.h>

extern uint8_t grid_platform_get_random_8();

#define grid_msg_packet_header_maxlength 26
#define GRID_MSG_FOOTER_maxlength 5

#define grid_msg_packet_body_maxlength GRID_PARAMETER_PACKET_maxlength - grid_msg_packet_header_maxlength - GRID_MSG_FOOTER_maxlength

#define GRID_MSG_RECENT_FINGERPRINT_BUFFER_LENGTH 32
#define GRID_MSG_RECENT_FINGERPRINT_BUFFER_INDEX_T uint8_t


struct grid_msg_model {

  uint64_t editor_heartbeat_lastrealtime;
  uint8_t heartbeat_type;

  uint32_t recent_messages[GRID_MSG_RECENT_FINGERPRINT_BUFFER_LENGTH];
  GRID_MSG_RECENT_FINGERPRINT_BUFFER_INDEX_T recent_messages_index;

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

void grid_msg_init(struct grid_msg_model* mod);

void grid_msg_set_heartbeat_type(struct grid_msg_model* mod, uint8_t type);
uint8_t grid_msg_get_heartbeat_type(struct grid_msg_model* mod);


void grid_msg_set_editor_heartbeat_lastrealtime(struct grid_msg_model* mod, uint64_t timestamp);
uint32_t grid_msg_get_editor_heartbeat_lastrealtime(struct grid_msg_model* mod);

// PACKET FUNCTIONS

uint32_t grid_msg_packet_get_length(struct grid_msg_packet* msg);

void grid_msg_packet_header_set_len(struct grid_msg_packet* msg, uint16_t len);
uint16_t grid_msg_packet_header_get_len(struct grid_msg_packet* msg);

void grid_msg_packet_header_set_id(struct grid_msg_packet* msg, uint8_t id);
uint8_t grid_msg_packet_header_get_id(struct grid_msg_packet* msg);

void grid_msg_packet_header_set_dx(struct grid_msg_packet* msg, uint8_t dx);
uint8_t grid_msg_packet_header_get_dx(struct grid_msg_packet* msg);

void grid_msg_packet_header_set_dy(struct grid_msg_packet* msg, uint8_t dy);
uint8_t grid_msg_packet_header_get_dy(struct grid_msg_packet* msg);

void grid_msg_packet_header_set_sx(struct grid_msg_packet* msg, uint8_t sx);
uint8_t grid_msg_packet_header_get_sx(struct grid_msg_packet* msg);

void grid_msg_packet_header_set_sy(struct grid_msg_packet* msg, uint8_t sy);
uint8_t grid_msg_packet_header_get_sy(struct grid_msg_packet* msg);

void grid_msg_packet_header_set_rot(struct grid_msg_packet* msg, uint8_t rot);
uint8_t grid_msg_packet_header_get_rot(struct grid_msg_packet* msg);

void grid_msg_packet_header_set_age(struct grid_msg_packet* msg, uint8_t age);
uint8_t grid_msg_packet_header_get_age(struct grid_msg_packet* msg);

void grid_msg_packet_header_set_session(struct grid_msg_packet* msg, uint8_t session);
uint8_t grid_msg_packet_header_get_session(struct grid_msg_packet* msg);

uint32_t grid_msg_packet_header_get_length(struct grid_msg_packet* msg);

uint32_t grid_msg_packet_body_get_length(struct grid_msg_packet* msg);
uint32_t grid_msg_packet_footer_get_length(struct grid_msg_packet* msg);

uint32_t grid_msg_packet_body_get_parameter(struct grid_msg_packet* msg, uint32_t text_start_offset, uint8_t parameter_offset, uint8_t parameter_length);
void grid_msg_packet_body_set_parameter(struct grid_msg_packet* msg, uint32_t text_start_offset, uint8_t parameter_offset, uint8_t parameter_length, uint32_t value);

void grid_msg_packet_body_append_text(struct grid_msg_packet* msg, char* string);

void grid_msg_packet_body_append_printf(struct grid_msg_packet* msg, char const* fmt, ...);
void grid_msg_packet_body_append_parameter(struct grid_msg_packet* msg, uint8_t parameter_offset, uint8_t parameter_length, uint32_t value);

void grid_msg_packet_to_chunk(struct grid_msg_packet* msg, char* chunk);
void grid_msg_packet_receive_char_by_char(struct grid_msg_packet* msg, uint8_t nextchar);

uint8_t grid_msg_packet_send_char_by_char(struct grid_msg_packet* msg, uint32_t charindex);

void grid_msg_packet_init(struct grid_msg_model* mod, struct grid_msg_packet* msg, uint8_t dx, uint8_t dy);
void grid_msg_packet_close(struct grid_msg_model* mod, struct grid_msg_packet* msg);

// RECENT FINGERPRINT FUNCTIONS

uint8_t grid_msg_recent_fingerprint_find(struct grid_msg_model* mod, uint32_t fingerprint);

void grid_msg_recent_fingerprint_store(struct grid_msg_model* mod, uint32_t fingerprint);

// STRING FUNCTIONS

uint32_t grid_msg_string_get_parameter(char* message, uint16_t offset, uint8_t length, uint8_t* error);
void grid_msg_string_set_parameter(char* message, uint16_t offset, uint8_t length, uint32_t value, uint8_t* error);

uint8_t grid_msg_string_calculate_checksum_of_packet_string(char* str, uint32_t length);
uint8_t grid_msg_string_calculate_checksum_of_string(char* str, uint32_t length);

uint8_t grid_msg_string_checksum_read(char* str, uint32_t length);
void grid_msg_string_checksum_write(char* message, uint32_t length, uint8_t checksum);

uint8_t grid_msg_string_read_hex_char_value(uint8_t ascii, uint8_t* error_flag);
uint32_t grid_msg_string_read_hex_string_value(char* start_location, uint8_t length, uint8_t* error_flag);
void grid_msg_string_write_hex_string_value(char* start_location, uint8_t size, uint32_t value);

#endif /* GRID_MSG_H_ */
