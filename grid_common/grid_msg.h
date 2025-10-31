#ifndef GRID_MSG_H
#define GRID_MSG_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "grid_platform.h"
#include "grid_protocol.h"

#include "grid_swsr.h"

typedef uint32_t grid_fingerprint_t;

grid_fingerprint_t grid_fingerprint_calculate(const char* msg);

struct grid_fingerprint_buf {

  uint8_t capa;
  uint8_t write;
  grid_fingerprint_t* data;
};

void grid_fingerprint_buf_init(struct grid_fingerprint_buf* fpb, uint8_t capacity);
void grid_fingerprint_buf_store(struct grid_fingerprint_buf* fpb, grid_fingerprint_t f);
uint8_t grid_fingerprint_buf_find(struct grid_fingerprint_buf* fpb, grid_fingerprint_t f);

struct grid_msg_model {

  uint64_t editor_heartbeat_lastrealtime;
  uint8_t heartbeat_type;
  uint8_t sessionid;
  uint8_t next_broadcast_message_id;
};

extern struct grid_msg_model grid_msg_state;

void grid_msg_model_init(struct grid_msg_model* msg);

void grid_msg_set_heartbeat_type(struct grid_msg_model* msg, uint8_t type);
uint8_t grid_msg_get_heartbeat_type(struct grid_msg_model* msg);

void grid_msg_set_editor_heartbeat_lastrealtime(struct grid_msg_model* msg, uint64_t timestamp);
uint64_t grid_msg_get_editor_heartbeat_lastrealtime(struct grid_msg_model* msg);

uint8_t grid_frame_calculate_checksum_string(uint8_t* frame, size_t length);
uint8_t grid_frame_calculate_checksum_packet(uint8_t* frame, size_t length);

uint32_t grid_frame_get_parameter(const uint8_t* frame, uint16_t offset, uint8_t length);
void grid_frame_set_parameter(uint8_t* frame, uint16_t offset, uint8_t length, uint32_t value);
int grid_frame_get_segment_char(uint8_t* frame, uint16_t offset, uint8_t head_hexes, uint32_t max_size, char* buffer);

void grid_str_checksum_set(char* str, uint32_t length, uint8_t checksum);

int grid_frame_verify(const uint8_t* frame, uint16_t length);

enum { GRID_MSG_SPI_OVERHEAD = GRID_PARAMETER_SPI_TRANSACTION_length - GRID_PARAMETER_SPI_SOURCE_FLAGS_index };
enum { GRID_MSG_BYTES = GRID_PARAMETER_SPI_TRANSACTION_length - GRID_MSG_SPI_OVERHEAD };

struct grid_msg {

  char data[GRID_MSG_BYTES];
  uint32_t length;
  uint32_t offset;
};

void grid_msg_reset_offset(struct grid_msg* msg);
void grid_msg_store_offset(struct grid_msg* msg);
void grid_msg_set_offset(struct grid_msg* msg, uint32_t offset);
int grid_msg_close(struct grid_msg* msg);
void grid_msg_init_brc(struct grid_msg_model* model, struct grid_msg* msg, uint8_t dx, uint8_t dy);
int grid_msg_close_brc(struct grid_msg_model* model, struct grid_msg* msg);
int grid_msg_nprintf(struct grid_msg* msg, const char* fmt, ...);
int grid_msg_add_segment_char(struct grid_msg* msg, uint8_t head_hexes, uint32_t size, char* buffer);
int grid_msg_add_debugtext(struct grid_msg* msg, const char* text);
int grid_msg_move(struct grid_msg* dest, struct grid_msg* src);
void grid_msg_to_swsr(struct grid_msg* msg, struct grid_swsr_t* swsr);
bool grid_msg_from_swsr(struct grid_msg* msg, struct grid_swsr_t* swsr);
bool grid_msg_from_uwsr(struct grid_msg* msg, struct grid_uwsr_t* uwsr);

void grid_str_transform_brc_params(uint8_t* msg, uint16_t length, int8_t dx, int8_t dy, uint8_t partner_rot);

// clang-format off

#define grid_msg_get_parameter_raw(data, param) \
	grid_frame_get_parameter( \
		(data), \
		GRID_ ## param ## _offset, \
		GRID_ ## param ## _length \
	)

#define grid_msg_set_parameter_raw(data, param, value) \
{ \
	grid_frame_set_parameter( \
		(data), \
		GRID_ ## param ## _offset, \
		GRID_ ## param ## _length, \
		(value) \
	); \
}

#define grid_msg_get_parameter(msg, param) \
	grid_frame_get_parameter( \
		(uint8_t*)(msg)->data, \
		(msg)->offset + GRID_ ## param ## _offset, \
		GRID_ ## param ## _length \
	)

#define grid_msg_set_parameter(msg, param, value) \
{ \
	grid_frame_set_parameter( \
		(uint8_t*)(msg)->data, \
		(msg)->offset + GRID_ ## param ## _offset, \
		GRID_ ## param ## _length, \
		(value) \
	); \
}

#define grid_msg_add_frame(msg, frame) \
	(grid_msg_store_offset((msg)), \
	grid_msg_nprintf((msg), frame))

// clang-format on

#endif /* GRID_MSG_H */
