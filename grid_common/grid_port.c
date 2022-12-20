/*
 * grid_port.c
 *
 * Created: 4/12/2019 5:27:13 PM
 * Author : SUKU WC
*/

#include "grid_port.h"



volatile struct grid_port GRID_PORT_N;
volatile struct grid_port GRID_PORT_E;
volatile struct grid_port GRID_PORT_S;
volatile struct grid_port GRID_PORT_W;

volatile struct grid_port GRID_PORT_U;
volatile struct grid_port GRID_PORT_H;





void grid_port_init(struct grid_port* por, uint8_t type, uint8_t dir){
	
	grid_buffer_init(&por->tx_buffer, GRID_BUFFER_SIZE);
	grid_buffer_init(&por->rx_buffer, GRID_BUFFER_SIZE);
	
	
	por->cooldown = 0;
	
	
	por->direction = dir;
	
	por->type		= type;
	
	por->tx_double_buffer_status	= 0;
	por->rx_double_buffer_status	= 0;
	
	
	for (uint32_t i=0; i<GRID_DOUBLE_BUFFER_TX_SIZE; i++){
		por->tx_double_buffer[i] = 0;		
	}
	for (uint32_t i=0; i<GRID_DOUBLE_BUFFER_RX_SIZE; i++){
		por->rx_double_buffer[i] = 0;
	}
	
	por->partner_fi = 0;
	
	por->partner_hwcfg = 0;
	por->partner_status = 1;
	
	por->ping_local_token = 255;
	por->ping_partner_token = 255;
	
	por->ping_flag = 0;
	
	if (type == GRID_PORT_TYPE_USART){	
		
		por->partner_status = 0;
		por->partner_fi = 0;
		
		
		sprintf((char*) por->ping_packet, "%c%c%c%c%02lx%02x%02x%c00\n", GRID_CONST_SOH, GRID_CONST_DCT, GRID_CONST_BELL, por->direction, grid_sys_get_hwcfg(&grid_sys_state), 255, 255, GRID_CONST_EOT);
		
		por->ping_packet_length = strlen((char*) por->ping_packet);	
			
		grid_msg_string_checksum_write(por->ping_packet, por->ping_packet_length, grid_msg_string_calculate_checksum_of_packet_string(por->ping_packet, por->ping_packet_length));
		

		
		if (por->direction == GRID_CONST_NORTH){
			por->dx = 0;
			por->dy = 1;
		}
		else if (por->direction == GRID_CONST_EAST){
			por->dx = 1;
			por->dy = 0;
		}
		else if (por->direction == GRID_CONST_SOUTH){
			por->dx = 0;
			por->dy = -1;
		}
		else if (por->direction == GRID_CONST_WEST){
			por->dx = -1;
			por->dy = 0;
		}
		
	}
	else{
		por->partner_status = 1; //UI AND USB are considered to be connected by default
	}
	
}

void grid_port_init_all(void){
	
	grid_port_init((struct grid_port*) &GRID_PORT_N, GRID_PORT_TYPE_USART, GRID_CONST_NORTH);
	grid_port_init((struct grid_port*) &GRID_PORT_E,  GRID_PORT_TYPE_USART, GRID_CONST_EAST);
	grid_port_init((struct grid_port*) &GRID_PORT_S, GRID_PORT_TYPE_USART, GRID_CONST_SOUTH);
	grid_port_init((struct grid_port*) &GRID_PORT_W,  GRID_PORT_TYPE_USART, GRID_CONST_WEST);
	
	grid_port_init((struct grid_port*) &GRID_PORT_U, GRID_PORT_TYPE_UI, 0);
	grid_port_init((struct grid_port*) &GRID_PORT_H, GRID_PORT_TYPE_USB, 0);	
	
	GRID_PORT_U.partner_status = 1; // UI IS ALWAYS CONNECTED
	GRID_PORT_H.partner_status = 1; // HOST IS ALWAYS CONNECTED (Not really!)
	
	
}






void grid_port_debug_print_text(char* debug_string){
	
	
	struct grid_msg_packet message;
	
	grid_msg_packet_init(&grid_msg_state, &message, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

	grid_msg_packet_body_append_printf(&message, GRID_CLASS_DEBUGTEXT_frame_start);
	grid_msg_packet_body_append_parameter(&message, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);
	grid_msg_packet_body_append_printf(&message, debug_string);
	grid_msg_packet_body_append_printf(&message, GRID_CLASS_DEBUGTEXT_frame_end);

	grid_msg_packet_close(&grid_msg_state, &message);
	grid_port_packet_send_everywhere(&message);
		
}


void grid_port_websocket_print_text(char* debug_string){
	
	
	struct grid_msg_packet message;
	
	grid_msg_packet_init(&grid_msg_state, &message, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

	grid_msg_packet_body_append_printf(&message, GRID_CLASS_WEBSOCKET_frame_start);
	grid_msg_packet_body_append_parameter(&message, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);
	grid_msg_packet_body_append_printf(&message, debug_string);
	grid_msg_packet_body_append_printf(&message, GRID_CLASS_WEBSOCKET_frame_end);

	grid_msg_packet_close(&grid_msg_state, &message);
	grid_port_packet_send_everywhere(&message);
		
}

void grid_port_debug_printf(char const *fmt, ...){

	va_list ap;


	char temp[100] = {0};

	va_start(ap, fmt);

	vsnprintf(temp, 99, fmt, ap);

	va_end(ap);

	grid_port_debug_print_text(temp);

	return;
}




uint8_t	grid_port_packet_send_everywhere(struct grid_msg_packet* msg){
	
	uint32_t message_length = grid_msg_packet_get_length(msg);
	
	if (grid_buffer_write_init((struct grid_buffer*) &GRID_PORT_U.rx_buffer, message_length)){

		for(uint32_t i = 0; i<message_length; i++){

			grid_buffer_write_character((struct grid_buffer*) &GRID_PORT_U.rx_buffer, grid_msg_packet_send_char_by_char(msg, i));
		}

		grid_buffer_write_acknowledge((struct grid_buffer*) &GRID_PORT_U.rx_buffer);

		return 1;
	}
	else{
		
		return 0;
	}
	
	
}
