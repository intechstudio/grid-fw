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

#define GRID_MSG_HEADER_maxlength 26
#define GRID_MSG_FOOTER_maxlength 5

#define GRID_MSG_BODY_maxlength  GRID_PARAMETER_PACKET_maxlength - GRID_MSG_HEADER_maxlength - GRID_MSG_FOOTER_maxlength



#define GRID_MSG_RECENT_MESSAGES_LENGTH			32
#define GRID_MSG_RECENT_MESSAGES_INDEX_T		uint8_t

struct grid_lastheader{

	uint8_t status;
	uint8_t id;

};


struct grid_msg_model
{
	

	uint32_t editor_heartbeat_lastrealtime;
	uint8_t heartbeat_type;

	uint32_t recent_messages[GRID_MSG_RECENT_MESSAGES_LENGTH];
	GRID_MSG_RECENT_MESSAGES_INDEX_T recent_messages_index;
	
    uint8_t sessionid;
	uint8_t next_broadcast_message_id;
	

	struct grid_lastheader lastheader_config;
	struct grid_lastheader lastheader_pagestore;
	struct grid_lastheader lastheader_pagediscard;
	struct grid_lastheader lastheader_pageclear;
	struct grid_lastheader lastheader_nvmerase;


};


volatile struct grid_msg_model grid_msg_state;


struct grid_msg{
	
	uint8_t header[GRID_MSG_HEADER_maxlength];
	uint8_t body[GRID_MSG_BODY_maxlength];
	uint8_t footer[GRID_MSG_FOOTER_maxlength];
	
	uint32_t header_length;
	uint32_t body_length;
	uint32_t footer_length;

	uint32_t last_appended_length;


};

void grid_msg_init(struct grid_msg_model* mod);

void	grid_msg_header_set_len(struct grid_msg* msg, uint16_t len);
uint16_t grid_msg_header_get_len(struct grid_msg* msg);

void	grid_msg_header_set_id(struct grid_msg* msg, uint8_t id);
uint8_t grid_msg_header_get_id(struct grid_msg* msg);

void	grid_msg_header_set_dx(struct grid_msg* msg, uint8_t dx);
uint8_t grid_msg_header_get_dx(struct grid_msg* msg);

void	grid_msg_header_set_dy(struct grid_msg* msg, uint8_t dy);
uint8_t grid_msg_header_get_dy(struct grid_msg* msg);

void	grid_msg_header_set_sx(struct grid_msg* msg, uint8_t sx);
uint8_t grid_msg_header_get_sx(struct grid_msg* msg);

void	grid_msg_header_set_sy(struct grid_msg* msg, uint8_t sy);
uint8_t grid_msg_header_get_sy(struct grid_msg* msg);



void	grid_msg_header_set_rot(struct grid_msg* msg, uint8_t rot);
uint8_t grid_msg_header_get_rot(struct grid_msg* msg);

void	grid_msg_header_set_age(struct grid_msg* msg, uint8_t age);
uint8_t grid_msg_header_get_age(struct grid_msg* msg);

void	grid_msg_header_set_session(struct grid_msg* msg, uint8_t session);
uint8_t grid_msg_header_get_session(struct grid_msg* msg);


uint32_t grid_msg_packet_get_length(struct grid_msg* msg);

uint32_t grid_msg_header_get_length(struct grid_msg* msg);
uint32_t grid_msg_body_get_length(struct grid_msg* msg);
uint32_t grid_msg_footer_get_length(struct grid_msg* msg);



uint32_t	grid_msg_text_get_parameter(struct grid_msg* msg, uint32_t text_start_offset, uint8_t parameter_offset, uint8_t parameter_length);
void		grid_msg_text_set_parameter(struct grid_msg* msg, uint32_t text_start_offset, uint8_t parameter_offset, uint8_t parameter_length, uint32_t value);


void	grid_msg_init_header(struct grid_msg* msg, uint8_t dx, uint8_t dy);

void	grid_msg_body_append_text(struct grid_msg* msg, uint8_t* string);


void 	grid_msg_body_append_printf(struct grid_msg* msg, char const *fmt, ...);
void	grid_msg_body_append_parameter(struct grid_msg* msg,  uint8_t parameter_offset, uint8_t parameter_length, uint32_t value);




void	grid_msg_packet_receive_char(struct grid_msg* msg, uint8_t nextchar);

uint8_t	grid_msg_packet_send_char(struct grid_msg* msg, uint32_t charindex);


uint8_t	grid_msg_packet_close(struct grid_msg* msg);




uint8_t grid_msg_string_validate(uint8_t* packet);


uint8_t grid_msg_find_recent(struct grid_msg_model* mod, uint32_t fingerprint);

void grid_msg_push_recent(struct grid_msg_model* mod, uint32_t fingerprint);




uint32_t grid_msg_get_parameter(uint8_t* message, uint8_t offset, uint8_t length, uint8_t* error);
uint32_t grid_msg_set_parameter(uint8_t* message, uint8_t offset, uint8_t length, uint32_t value, uint8_t* error);


uint8_t grid_msg_calculate_checksum_of_packet_string(uint8_t* str, uint32_t length);
uint8_t grid_msg_calculate_checksum_of_string(uint8_t* str, uint32_t length);

uint8_t grid_msg_checksum_read(uint8_t* str, uint32_t length);
void grid_msg_checksum_write(uint8_t* message, uint32_t length, uint8_t checksum);


uint8_t grid_msg_read_hex_char_value(uint8_t ascii, uint8_t* error_flag);
uint32_t grid_msg_read_hex_string_value(uint8_t* start_location, uint8_t length, uint8_t* error_flag);
void grid_msg_write_hex_string_value(uint8_t* start_location, uint8_t size, uint32_t value);


#endif /* GRID_MSG_H_ */