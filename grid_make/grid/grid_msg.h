/*
 * grid_msg.h
 *
 * Created: 9/23/2020 2:36:14 PM
 *  Author: suku
 */ 


#ifndef GRID_MSG_H_
#define GRID_MSG_H_

#include "grid_module.h"


#define GRID_MSG_HEADER_maxlength 26
#define GRID_MSG_FOOTER_maxlength 5

#define GRID_MSG_BODY_maxlength  GRID_PARAMETER_PACKET_maxlength - GRID_MSG_HEADER_maxlength - GRID_MSG_FOOTER_maxlength




struct grid_msg{
	
	uint8_t header[GRID_MSG_HEADER_maxlength];
	uint8_t body[GRID_MSG_BODY_maxlength];
	uint8_t footer[GRID_MSG_FOOTER_maxlength];
	
	uint32_t header_length;
	uint32_t body_length;
	uint32_t footer_length;

	uint32_t last_appended_length;


};

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



void	grid_msg_init(struct grid_msg* msg);

void	grid_msg_init_header(struct grid_msg* msg, uint8_t dx, uint8_t dy);

void	grid_msg_body_append_text(struct grid_msg* msg, uint8_t* string);


void 	grid_msg_body_append_printf(struct grid_msg* msg, char const *fmt, ...);
void	grid_msg_body_append_parameter(struct grid_msg* msg,  uint8_t parameter_offset, uint8_t parameter_length, uint32_t value);




void	grid_msg_packet_receive_char(struct grid_msg* msg, uint8_t nextchar);

uint8_t	grid_msg_packet_send_char(struct grid_msg* msg, uint32_t charindex);


uint8_t	grid_msg_packet_close(struct grid_msg* msg);


uint8_t	grid_msg_packet_send_everywhere(struct grid_msg* msg);

#endif /* GRID_MSG_H_ */