/*
 * grid_msg.c
 *
 * Created: 9/23/2020 2:35:51 PM
 *  Author: suku
 */ 


#include "include/grid_msg.h"

struct grid_msg_model grid_msg_state;

void grid_msg_init(struct grid_msg_model* mod){


	mod->sessionid = grid_platform_get_random_8();

	for (uint8_t i=0; i<GRID_MSG_LASTHEADER_INDEX_COUNT; i++){
		grid_msg_store_lastheader(mod, i, 0);
	}

	mod->editor_heartbeat_lastrealtime = 0;	
	mod->heartbeat_type = 0;

}

void grid_msg_set_heartbeat_type(struct grid_msg_model* mod, uint8_t type){
	mod->heartbeat_type = type;
}

uint8_t grid_msg_get_heartbeat_type(struct grid_msg_model* mod){
	return mod->heartbeat_type;
}


void grid_msg_store_lastheader(struct grid_msg_model* mod, enum grid_msg_lastheader_index_t index, uint8_t value){

	mod->lastheader[index].id = value;
	mod->lastheader[index].state = -1; // -1 means not completed operation

}

uint8_t grid_msg_get_lastheader_state(struct grid_msg_model* mod, enum grid_msg_lastheader_index_t index){
	return mod->lastheader[index].state;
}

uint8_t grid_msg_get_lastheader_id(struct grid_msg_model* mod, enum grid_msg_lastheader_index_t index){
	return mod->lastheader[index].id;
}

void grid_msg_clear_lastheader(struct grid_msg_model* mod, enum grid_msg_lastheader_index_t index){

	mod->lastheader[index].state = 0; // 0 means operation completed
}



void grid_msg_set_editor_heartbeat_lastrealtime(struct grid_msg_model* mod, uint64_t timestamp){
	mod->editor_heartbeat_lastrealtime = timestamp;
}

uint32_t grid_msg_get_editor_heartbeat_lastrealtime(struct grid_msg_model* mod){
	return mod->editor_heartbeat_lastrealtime;
}


// ======================= GRID MSG LEN ======================//
void	grid_msg_header_set_len(struct grid_msg_packet* msg, uint16_t len){
	
	uint8_t error = 0;
	grid_msg_string_set_parameter(msg->header, GRID_BRC_LEN_offset, GRID_BRC_LEN_length, len, &error);
	
}

uint16_t grid_msg_header_get_len(struct grid_msg_packet* msg){
	
	uint8_t error = 0;
	return grid_msg_string_get_parameter(msg->header, GRID_BRC_LEN_offset, GRID_BRC_LEN_length, &error);
}

// ======================= GRID MSG ID ======================//
void	grid_msg_header_set_id(struct grid_msg_packet* msg, uint8_t id){
	
	uint8_t error = 0;
	grid_msg_string_set_parameter(msg->header, GRID_BRC_ID_offset, GRID_BRC_ID_length, id, &error);
	
}


uint8_t grid_msg_header_get_id(struct grid_msg_packet* msg){
	
	uint8_t error = 0;
	return grid_msg_string_get_parameter(msg->header, GRID_BRC_ID_offset, GRID_BRC_ID_length, &error);
}

// ======================= GRID MSG DX ======================//
void	grid_msg_header_set_dx(struct grid_msg_packet* msg, uint8_t dx){
	
	uint8_t error = 0;
	grid_msg_string_set_parameter(msg->header, GRID_BRC_DX_offset, GRID_BRC_DX_length, dx, &error);
	
}


uint8_t grid_msg_header_get_dx(struct grid_msg_packet* msg){
	
	uint8_t error = 0;
	return grid_msg_string_get_parameter(msg->header, GRID_BRC_DX_offset, GRID_BRC_DX_length, &error);
}


// ======================= GRID MSG DY ======================//
void	grid_msg_header_set_dy(struct grid_msg_packet* msg, uint8_t dy){
	
	uint8_t error = 0;
	grid_msg_string_set_parameter(msg->header, GRID_BRC_DY_offset, GRID_BRC_DY_length, dy, &error);
	
}


uint8_t grid_msg_header_get_dy(struct grid_msg_packet* msg){
	
	uint8_t error = 0;
	return grid_msg_string_get_parameter(msg->header, GRID_BRC_DY_offset, GRID_BRC_DY_length, &error);
}

// ======================= GRID MSG SX ======================//
void	grid_msg_header_set_sx(struct grid_msg_packet* msg, uint8_t sx){
	
	uint8_t error = 0;
	grid_msg_string_set_parameter(msg->header, GRID_BRC_SX_offset, GRID_BRC_SX_length, sx, &error);
	
}


uint8_t grid_msg_header_get_sx(struct grid_msg_packet* msg){
	
	uint8_t error = 0;
	return grid_msg_string_get_parameter(msg->header, GRID_BRC_SX_offset, GRID_BRC_SX_length, &error);
}

// ======================= GRID MSG SY ======================//
void	grid_msg_header_set_sy(struct grid_msg_packet* msg, uint8_t sy){
	
	uint8_t error = 0;
	grid_msg_string_set_parameter(msg->header, GRID_BRC_SY_offset, GRID_BRC_SY_length, sy, &error);
	
}


uint8_t grid_msg_header_get_sy(struct grid_msg_packet* msg){
	
	uint8_t error = 0;
	return grid_msg_string_get_parameter(msg->header, GRID_BRC_SY_offset, GRID_BRC_SY_length, &error);
}



// ======================= GRID MSG ROT ======================//
void	grid_msg_header_set_rot(struct grid_msg_packet* msg, uint8_t rot){
	
	uint8_t error = 0;
	grid_msg_string_set_parameter(msg->header, GRID_BRC_ROT_offset, GRID_BRC_ROT_length, rot, &error);
	
}


uint8_t grid_msg_header_get_rot(struct grid_msg_packet* msg){
	
	uint8_t error = 0;
	return grid_msg_string_get_parameter(msg->header, GRID_BRC_ROT_offset, GRID_BRC_ROT_length, &error);
}

// ======================= GRID MSG AGE ======================//
void	grid_msg_header_set_age(struct grid_msg_packet* msg, uint8_t age){
	
	uint8_t error = 0;
	grid_msg_string_set_parameter(msg->header, GRID_BRC_MSGAGE_offset, GRID_BRC_MSGAGE_length, age, &error);
	
}

uint8_t grid_msg_header_get_age(struct grid_msg_packet* msg){
	
	uint8_t error = 0;
	return grid_msg_string_get_parameter(msg->header, GRID_BRC_MSGAGE_offset, GRID_BRC_MSGAGE_length, &error);
}

// ======================= GRID MSG session ======================//
void	grid_msg_header_set_session(struct grid_msg_packet* msg, uint8_t session){
	
	uint8_t error = 0;
	grid_msg_string_set_parameter(msg->header, GRID_BRC_SESSION_offset, GRID_BRC_SESSION_length, session, &error);
	
}

uint8_t grid_msg_header_get_session(struct grid_msg_packet* msg){
	
	uint8_t error = 0;
	return grid_msg_string_get_parameter(msg->header, GRID_BRC_SESSION_offset, GRID_BRC_SESSION_length, &error);
}

// ======================= MSG GET PACKET LENGTH ======================//
uint32_t grid_msg_packet_get_length(struct grid_msg_packet* msg){
	
	return (msg->header_length + msg->body_length + msg->footer_length);
}

// ======================= MSG GET HEADER LENGTH ======================//
uint32_t grid_msg_packet_header_get_length(struct grid_msg_packet* msg){
	
	return (msg->header_length);
}

// ======================= MSG GET BODY LENGTH ======================//
uint32_t grid_msg_packet_body_get_length(struct grid_msg_packet* msg){
	
	return (msg->body_length);
}

// ======================= MSG GET FOOTER LENGTH ======================//
uint32_t grid_msg_packet_footer_get_length(struct grid_msg_packet* msg){
	
	return (msg->footer_length);
}


void	grid_msg_packet_body_append_text(struct grid_msg_packet* msg, char* str){

	uint32_t len = strlen((char*) str);
	
	for(uint32_t i=0; i<len; i++){
		
		msg->body[msg->body_length + i] = str[i];
	}
	
	msg->body_length += len;
	msg->last_appended_length  += len;

}



void grid_msg_packet_body_append_printf(struct grid_msg_packet* msg, char const *fmt, ...){

	va_list ap;


	va_start(ap, fmt);

	vsprintf((char*) &msg->body[msg->body_length], fmt, ap);

	va_end(ap);
	

	msg->last_appended_length = strlen((char*) &msg->body[msg->body_length]);

	msg->body_length += msg->last_appended_length;

	return;
}
void grid_msg_packet_body_append_parameter(struct grid_msg_packet* msg,  uint8_t parameter_offset, uint8_t parameter_length, uint32_t value){

	uint8_t text_start_offset = msg->body_length - msg->last_appended_length;

	grid_msg_packet_body_set_parameter(msg, text_start_offset, parameter_offset, parameter_length, value);

}

uint32_t grid_msg_packet_body_get_parameter(struct grid_msg_packet* msg, uint32_t text_start_offset, uint8_t parameter_offset, uint8_t parameter_length){
	
	uint8_t error = 0;
	
	return grid_msg_string_read_hex_string_value(&msg->body[text_start_offset + parameter_offset], parameter_length, &error);
	
}

void grid_msg_packet_body_set_parameter(struct grid_msg_packet* msg, uint32_t text_start_offset, uint8_t parameter_offset, uint8_t parameter_length, uint32_t value){
	
	return grid_msg_string_write_hex_string_value(&msg->body[text_start_offset + parameter_offset], parameter_length, value);
	
}


// ======================= MSG INIT HEADER======================//

void	grid_msg_packet_init(struct grid_msg_model* mod, struct grid_msg_packet* msg, uint8_t dx, uint8_t dy){
	
	msg->header_length = 0;
	msg->body_length = 0;
	msg->last_appended_length = 0;
	msg->footer_length = 0;
	

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
    
    uint8_t session = mod->sessionid;
    
	sprintf((char*) msg->header, GRID_BRC_frame_quick, GRID_CONST_SOH, GRID_CONST_BRC, session, dx, dy , GRID_CONST_EOB);
	msg->header_length = strlen((char*) msg->header);
	
	// grid_msg_header_set_dx(msg, dx);
	// grid_msg_header_set_dy(msg, dy);

	// grid_msg_header_set_sx(msg, GRID_SYS_DEFAULT_POSITION);
	// grid_msg_header_set_sy(msg, GRID_SYS_DEFAULT_POSITION);
	// grid_msg_header_set_rot(msg, rot);
	// grid_msg_header_set_age(msg, GRID_SYS_DEFAULT_AGE);

	// grid_msg_header_set_session(msg, session);
	
	
}

void grid_msg_packet_to_chunk(struct grid_msg_packet* msg, char* chunk){

	uint16_t length = grid_msg_packet_get_length(msg);

	for (uint16_t i=0; i<length; i++){

		chunk[i] = grid_msg_packet_send_char_by_char(msg, i);

	}

	chunk[length] = '\0';

}


// ======================= MSG RECEIVE CHAR ======================//
void	grid_msg_packet_receive_char_by_char(struct grid_msg_packet* msg, uint8_t nextchar){
	
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

uint8_t	grid_msg_packet_send_char_by_char(struct grid_msg_packet* msg, uint32_t charindex){
	
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



void grid_msg_packet_close(struct grid_msg_model* mod, struct grid_msg_packet* msg){
	

	
	sprintf((char*) &msg->footer[msg->footer_length], "%c", GRID_CONST_EOT);
	msg->footer_length += strlen((char*) &msg->footer[msg->footer_length]);
	
	grid_msg_header_set_len(msg, msg->header_length + msg->body_length + msg->footer_length);
	grid_msg_header_set_session(msg, mod->sessionid);	
	grid_msg_header_set_id(msg, mod->next_broadcast_message_id);	
	
	mod->next_broadcast_message_id++;
	

	
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
	
	sprintf((char*) &msg->footer[msg->footer_length], "%02x\n", checksum);
	msg->footer_length += strlen((char*) &msg->footer[msg->footer_length]);
	
	
}





// RECENT MESSAGES

uint8_t grid_msg_recent_fingerprint_find(struct grid_msg_model* model, uint32_t fingerprint){
	
	for(GRID_MSG_RECENT_FINGERPRINT_BUFFER_INDEX_T i = 0; i<GRID_MSG_RECENT_FINGERPRINT_BUFFER_LENGTH; i++){
		
		if (model->recent_messages[i%GRID_MSG_RECENT_FINGERPRINT_BUFFER_LENGTH] == fingerprint){
			
			return 1;
			
		}
		
	}
	
	return 0;
}

void grid_msg_recent_fingerprint_store(struct grid_msg_model* model, uint32_t fingerprint){
	
	model->recent_messages_index+=1;
	model->recent_messages_index%=GRID_MSG_RECENT_FINGERPRINT_BUFFER_LENGTH;
	
	model->recent_messages[model->recent_messages_index] = fingerprint;
	
}




uint8_t grid_msg_string_calculate_checksum_of_packet_string(char* str, uint32_t length){
	
	uint8_t checksum = 0;
	for (uint32_t i=0; i<length-3; i++){
		checksum ^= str[i];
	}
	
	return checksum;
	
}

uint8_t grid_msg_string_calculate_checksum_of_string(char* str, uint32_t length){
	
	uint8_t checksum = 0;
	for (uint32_t i=0; i<length; i++){
		checksum ^= str[i];
	}
	
	return checksum;
	
}


uint8_t grid_msg_string_checksum_read(char* str, uint32_t length){
	uint8_t error_flag;
	return grid_msg_string_read_hex_string_value(&str[length-3], 2, &error_flag);
}

void grid_msg_string_checksum_write(char* message, uint32_t length, uint8_t checksum){
	
// 	uint8_t checksum_string[4];
// 
// 	sprintf((char*) checksum_string, "%02x", checksum);
// 
// 	message[length-3] = checksum_string[0];
// 	message[length-2] = checksum_string[1];
	
	grid_msg_string_write_hex_string_value(&message[length-3], 2, checksum);
	
}


// MESSAGE PARAMETER FUNCTIONS

uint32_t grid_msg_string_get_parameter(char* message, uint16_t offset, uint8_t length, uint8_t* error){
		
	return grid_msg_string_read_hex_string_value(&message[offset], length, error);	
}

void grid_msg_string_set_parameter(char* message, uint16_t offset, uint8_t length, uint32_t value, uint8_t* error){
	
	grid_msg_string_write_hex_string_value(&message[offset], length, value);
	
}


uint8_t grid_msg_string_read_hex_char_value(uint8_t ascii, uint8_t* error_flag){
		
	uint8_t result = 0;
	
	if (ascii>47 && ascii<58){
		result = ascii-48;
	}
	else if(ascii>96 && ascii<103){
		result = ascii - 97 + 10;
	}
	else{
		// wrong input
		if (error_flag != 0){
			*error_flag = ascii;
		}
	}
	
	return result;	
}

uint32_t grid_msg_string_read_hex_string_value(char* start_location, uint8_t length, uint8_t* error_flag){
	
	uint32_t result  = 0;
	
	for(uint8_t i=0; i<length; i++){
		
		result += grid_msg_string_read_hex_char_value(start_location[i], error_flag) << (length-i-1)*4;

		
	}

	return result;
}

void grid_msg_string_write_hex_string_value(char* start_location, uint8_t size, uint32_t value){
	
	uint8_t str[10];
	
	sprintf((char*) str, "%08lx", value);
		
	for(uint8_t i=0; i<size; i++){	
		start_location[i] = str[8-size+i];	
	}

}

