/*
 * grid_msg.c
 *
 * Created: 9/23/2020 2:35:51 PM
 *  Author: suku
 */ 


#include "grid_msg.h"


// ======================= GRID MSG LEN ======================//
void	grid_msg_header_set_len(struct grid_msg* msg, uint16_t len){
	
	uint8_t error = 0;
	grid_msg_set_parameter(msg->header, GRID_BRC_LEN_offset, GRID_BRC_LEN_length, len, &error);
	
}

uint16_t grid_msg_header_get_len(struct grid_msg* msg){
	
	uint8_t error = 0;
	return grid_msg_get_parameter(msg->header, GRID_BRC_LEN_offset, GRID_BRC_LEN_length, &error);
}

// ======================= GRID MSG ID ======================//
void	grid_msg_header_set_id(struct grid_msg* msg, uint8_t id){
	
	uint8_t error = 0;
	grid_msg_set_parameter(msg->header, GRID_BRC_ID_offset, GRID_BRC_ID_length, id, &error);
	
}


uint8_t grid_msg_header_get_id(struct grid_msg* msg){
	
	uint8_t error = 0;
	return grid_msg_get_parameter(msg->header, GRID_BRC_ID_offset, GRID_BRC_ID_length, &error);
}

// ======================= GRID MSG DX ======================//
void	grid_msg_header_set_dx(struct grid_msg* msg, uint8_t dx){
	
	uint8_t error = 0;
	grid_msg_set_parameter(msg->header, GRID_BRC_DX_offset, GRID_BRC_DX_length, dx, &error);
	
}


uint8_t grid_msg_header_get_dx(struct grid_msg* msg){
	
	uint8_t error = 0;
	return grid_msg_get_parameter(msg->header, GRID_BRC_DX_offset, GRID_BRC_DX_length, &error);
}


// ======================= GRID MSG DY ======================//
void	grid_msg_header_set_dy(struct grid_msg* msg, uint8_t dy){
	
	uint8_t error = 0;
	grid_msg_set_parameter(msg->header, GRID_BRC_DY_offset, GRID_BRC_DY_length, dy, &error);
	
}


uint8_t grid_msg_header_get_dy(struct grid_msg* msg){
	
	uint8_t error = 0;
	return grid_msg_get_parameter(msg->header, GRID_BRC_DY_offset, GRID_BRC_DY_length, &error);
}

// ======================= GRID MSG SX ======================//
void	grid_msg_header_set_sx(struct grid_msg* msg, uint8_t sx){
	
	uint8_t error = 0;
	grid_msg_set_parameter(msg->header, GRID_BRC_SX_offset, GRID_BRC_SX_length, sx, &error);
	
}


uint8_t grid_msg_header_get_sx(struct grid_msg* msg){
	
	uint8_t error = 0;
	return grid_msg_get_parameter(msg->header, GRID_BRC_SX_offset, GRID_BRC_SX_length, &error);
}

// ======================= GRID MSG SY ======================//
void	grid_msg_header_set_sy(struct grid_msg* msg, uint8_t sy){
	
	uint8_t error = 0;
	grid_msg_set_parameter(msg->header, GRID_BRC_SY_offset, GRID_BRC_SY_length, sy, &error);
	
}


uint8_t grid_msg_header_get_sy(struct grid_msg* msg){
	
	uint8_t error = 0;
	return grid_msg_get_parameter(msg->header, GRID_BRC_SY_offset, GRID_BRC_SY_length, &error);
}



// ======================= GRID MSG ROT ======================//
void	grid_msg_header_set_rot(struct grid_msg* msg, uint8_t rot){
	
	uint8_t error = 0;
	grid_msg_set_parameter(msg->header, GRID_BRC_ROT_offset, GRID_BRC_ROT_length, rot, &error);
	
}


uint8_t grid_msg_header_get_rot(struct grid_msg* msg){
	
	uint8_t error = 0;
	return grid_msg_get_parameter(msg->header, GRID_BRC_ROT_offset, GRID_BRC_ROT_length, &error);
}

// ======================= GRID MSG AGE ======================//
void	grid_msg_header_set_age(struct grid_msg* msg, uint8_t age){
	
	uint8_t error = 0;
	grid_msg_set_parameter(msg->header, GRID_BRC_MSGAGE_offset, GRID_BRC_MSGAGE_length, age, &error);
	
}

uint8_t grid_msg_header_get_age(struct grid_msg* msg){
	
	uint8_t error = 0;
	return grid_msg_get_parameter(msg->header, GRID_BRC_MSGAGE_offset, GRID_BRC_MSGAGE_length, &error);
}

// ======================= GRID MSG session ======================//
void	grid_msg_header_set_session(struct grid_msg* msg, uint8_t session){
	
	uint8_t error = 0;
	grid_msg_set_parameter(msg->header, GRID_BRC_SESSION_offset, GRID_BRC_SESSION_length, session, &error);
	
}

uint8_t grid_msg_header_get_session(struct grid_msg* msg){
	
	uint8_t error = 0;
	return grid_msg_get_parameter(msg->header, GRID_BRC_SESSION_offset, GRID_BRC_SESSION_length, &error);
}

// ======================= MSG GET PACKET LENGTH ======================//
uint32_t grid_msg_packet_get_length(struct grid_msg* msg){
	
	return (msg->header_length + msg->body_length + msg->footer_length);
}

// ======================= MSG GET HEADER LENGTH ======================//
uint32_t grid_msg_header_get_length(struct grid_msg* msg){
	
	return (msg->header_length);
}

// ======================= MSG GET BODY LENGTH ======================//
uint32_t grid_msg_body_get_length(struct grid_msg* msg){
	
	return (msg->body_length);
}

// ======================= MSG GET FOOTER LENGTH ======================//
uint32_t grid_msg_footer_get_length(struct grid_msg* msg){
	
	return (msg->footer_length);
}


void	grid_msg_body_append_text(struct grid_msg* msg, uint8_t* str){

	uint32_t len = strlen(str);
	
	for(uint32_t i=0; i<len; i++){
		
		msg->body[msg->body_length + i] = str[i];
	}
	
	msg->body_length += len;
	msg->last_appended_length  += len;

}



void grid_msg_body_append_printf(struct grid_msg* msg, char const *fmt, ...){

	va_list ap;


	va_start(ap, fmt);

	vsprintf(&msg->body[msg->body_length], fmt, ap);

	va_end(ap);
	

	msg->last_appended_length = strlen(&msg->body[msg->body_length]);

	msg->body_length += msg->last_appended_length;

	return;
}
void grid_msg_body_append_parameter(struct grid_msg* msg,  uint8_t parameter_offset, uint8_t parameter_length, uint32_t value){

	uint8_t text_start_offset = msg->body_length - msg->last_appended_length;

	grid_msg_text_set_parameter(msg, text_start_offset, parameter_offset, parameter_length, value);

}

uint32_t grid_msg_text_get_parameter(struct grid_msg* msg, uint32_t text_start_offset, uint8_t parameter_offset, uint8_t parameter_length){
	
	uint8_t error;
	
	return grid_sys_read_hex_string_value(&msg->body[text_start_offset + parameter_offset], parameter_length, error);
	
}

void grid_msg_text_set_parameter(struct grid_msg* msg, uint32_t text_start_offset, uint8_t parameter_offset, uint8_t parameter_length, uint32_t value){
	
	return grid_sys_write_hex_string_value(&msg->body[text_start_offset + parameter_offset], parameter_length, value);
	
}





// ======================= GRID MSG INIT ======================//
void	grid_msg_init(struct grid_msg* msg){
	
	msg->header_length = 0;
	msg->body_length = 0;
	msg->last_appended_length = 0;
	msg->footer_length = 0;
	

	// No need to initialize array

	// for (uint32_t i=0; i<GRID_MSG_HEADER_maxlength; i++)
	// {
	// 	msg->header[i] = 0;
	// }
	
	// for (uint32_t i=0; i<GRID_MSG_BODY_maxlength; i++)
	// {
	// 	msg->body[i] = 0;
	// }
	
	// for (uint32_t i=0; i<GRID_MSG_FOOTER_maxlength; i++)
	// {
	// 	msg->footer[i] = 0;
	// }
	
		
}

// ======================= MSG INIT HEADER======================//

void	grid_msg_init_header(struct grid_msg* msg, uint8_t dx, uint8_t dy, uint8_t rot){
	
    
    uint8_t session = grid_sys_state.sessionid;
    
	sprintf(msg->header, GRID_BRC_frame_quick, GRID_CONST_SOH, GRID_CONST_BRC, session, dx, dy , GRID_CONST_EOB);
	msg->header_length = strlen(msg->header);
	
	// grid_msg_header_set_dx(msg, dx);
	// grid_msg_header_set_dy(msg, dy);

	// grid_msg_header_set_sx(msg, GRID_SYS_DEFAULT_POSITION);
	// grid_msg_header_set_sy(msg, GRID_SYS_DEFAULT_POSITION);
	// grid_msg_header_set_rot(msg, rot);
	// grid_msg_header_set_age(msg, GRID_SYS_DEFAULT_AGE);

	// grid_msg_header_set_session(msg, session);
	
	
}

// ======================= MSG RECEIVE CHAR ======================//
void	grid_msg_packet_receive_char(struct grid_msg* msg, uint8_t nextchar){
	
	if (msg->body_length == 0){
		
		if (nextchar != GRID_CONST_EOB){
			msg->header[msg->header_length] = nextchar;
			msg->header_length++;
		}
		else{
			msg->body[msg->body_length] = nextchar;
			msg->body_length++;
			
		}
		
	}
	else if(msg->footer_length == 0){
		
		if (nextchar != GRID_CONST_EOT){
			msg->body[msg->body_length] = nextchar;
			msg->body_length++;
		}
		else{
			msg->footer[msg->footer_length] = nextchar;
			msg->footer_length++;
			
		}		
		
	}
	else{
		
		msg->footer[msg->footer_length] = nextchar;
		msg->footer_length++;
		
	}
	
}

// ======================= GRID MSG SEND CHAR ======================//

uint8_t	grid_msg_packet_send_char(struct grid_msg* msg, uint32_t charindex){
	
	if (charindex < msg->header_length){
		
		return msg->header[charindex];
	}
	else if (charindex < msg->body_length + msg->header_length){
	
		return msg->body[charindex - msg->header_length];
	}
	else if (charindex < msg->footer_length + msg->body_length + msg->header_length){
	
		return msg->footer[charindex - msg->header_length - msg->body_length];
	}
	else{
		// OVERRUN
		return -1;
	}
	
	
}



uint8_t	grid_msg_packet_close(struct grid_msg* msg){
	
	
	sprintf(&msg->footer[msg->footer_length], "%c", GRID_CONST_EOT);
	msg->footer_length += strlen(&msg->footer[msg->footer_length]);
	
	grid_msg_header_set_len(msg, msg->header_length + msg->body_length + msg->footer_length);
	grid_msg_header_set_id(msg, grid_sys_state.next_broadcast_message_id);	
	
	grid_sys_state.next_broadcast_message_id++;
	
	
	uint8_t checksum = 0;
	
	for (uint32_t i=0; i<msg->header_length; i++){
		checksum ^= msg->header[i];
	}
		
	for (uint32_t i=0; i<msg->body_length; i++){
		checksum ^= msg->body[i];
	}
		
	for (uint32_t i=0; i<msg->footer_length; i++){
		checksum ^= msg->footer[i];
	}
	
	sprintf(&msg->footer[msg->footer_length], "%02x\n", checksum);
	msg->footer_length += strlen(&msg->footer[msg->footer_length]);
	
	
}

uint8_t	grid_msg_packet_send_everywhere(struct grid_msg* msg){
	
	uint32_t message_length = grid_msg_packet_get_length(msg);
	
	if (grid_buffer_write_init(&GRID_PORT_U.rx_buffer, message_length)){

		for(uint32_t i = 0; i<message_length; i++){

			grid_buffer_write_character(&GRID_PORT_U.rx_buffer, grid_msg_packet_send_char(msg, i));
		}

		grid_buffer_write_acknowledge(&GRID_PORT_U.rx_buffer);

		return 1;
	}
	else{
		
		return 0;
	}
	
	
}

