/*
 * grid_port.c
 *
 * Created: 4/12/2019 5:27:13 PM
 * Author : SUKU WC
*/

#include "grid_port.h"



struct grid_port volatile * GRID_PORT_N;
struct grid_port volatile * GRID_PORT_E;
struct grid_port volatile * GRID_PORT_S;
struct grid_port volatile * GRID_PORT_W;

struct grid_port volatile * GRID_PORT_U;
struct grid_port volatile * GRID_PORT_H;

char grid_port_get_name_char(struct grid_port* por){

	// Print Direction for debugging
	char direction_lookup[4] = {'N', 'E', 'S', 'W'};
	uint8_t direction_index = (por->direction+7)%4;

	return direction_lookup[direction_index];

}


static void grid_port_timeout_try_disconect(struct grid_port* por){

	if (grid_platform_rtc_get_elapsed_time(por->rx_double_buffer_timestamp) < 1000*1000){
		// no need to disconnect yet!
		return;
	}

	if (por->partner_status == 1){
	
			// Print Direction for debugging
			grid_platform_printf("Disconnect %c\r\n", grid_port_get_name_char(por));	
			
			grid_port_receiver_softreset(por);	

			grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_RED, 50);	
			grid_led_set_alert_frequency(&grid_led_state, -2);	
			grid_led_set_alert_phase(&grid_led_state, 100);	
	}
	else{
	
		if (por->rx_double_buffer_read_start_index == 0 && por->rx_double_buffer_seek_start_index == 0){
			// Ready to receive
			//grid_platform_printf("RtR\r\n");
			grid_port_receiver_softreset(por);
		}
		else{
		
			// Print Direction for debugging
			grid_platform_printf("Timeout Disconnect 2 %c (R%d S%d W%d)\r\n", grid_port_get_name_char(por), por->rx_double_buffer_read_start_index, por->rx_double_buffer_seek_start_index, por->rx_double_buffer_write_index);
			grid_port_receiver_softreset(por);
		}
	
	}
}


static uint8_t grid_port_rxdobulebuffer_check_overrun(struct grid_port* por){

	uint8_t overrun_condition_1 = (por->rx_double_buffer_seek_start_index == por->rx_double_buffer_read_start_index-1);
	uint8_t overrun_condition_2 = (por->rx_double_buffer_seek_start_index == GRID_DOUBLE_BUFFER_RX_SIZE-1 && por->rx_double_buffer_read_start_index == 0);
	uint8_t overrun_condition_3 = (por->rx_double_buffer[(por->rx_double_buffer_read_start_index + GRID_DOUBLE_BUFFER_RX_SIZE -1)%GRID_DOUBLE_BUFFER_RX_SIZE] !=0);

	return (overrun_condition_1 || overrun_condition_2 || overrun_condition_3);

}

static void grid_port_rxdobulebuffer_seek_newline(struct grid_port* por){


	for(uint16_t i = 0; i<490; i++){ // 490 is the max processing length
				
		if (por->rx_double_buffer[por->rx_double_buffer_seek_start_index] == 10){ // \n
				
			por->rx_double_buffer_status = 1;
				
			break;
		}
		else if (por->rx_double_buffer[por->rx_double_buffer_seek_start_index] == 0){
			
			break;
		}
			
		// Buffer overrun error 1, 2, 3
		if (grid_port_rxdobulebuffer_check_overrun(por)){

			grid_platform_printf("Overrun%d\r\n", por->direction);
			grid_platform_printf("R%d S%d W%d\r\n", por->rx_double_buffer_read_start_index, por->rx_double_buffer_seek_start_index, por->rx_double_buffer_write_index);

			grid_port_receiver_hardreset(por);	

			grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_RED, 50);	
			grid_led_set_alert_frequency(&grid_led_state, -2);	
			grid_led_set_alert_phase(&grid_led_state, 100);	
			return;
		}
			
		// Increment seek pointer
		if (por->rx_double_buffer_seek_start_index < GRID_DOUBLE_BUFFER_RX_SIZE-1){
				
			por->rx_double_buffer_seek_start_index++;
		}
		else{
				
			por->rx_double_buffer_seek_start_index=0;
		}
			
	}
}


void grid_port_receive_task(struct grid_port* por){

	//parity error
	
	if (por->usart_error_flag == 1){
		
		por->usart_error_flag = 0;
		
		grid_platform_printf("Parity\r\n");
		grid_port_receiver_hardreset(por);
		grid_port_debug_printf("Parity error");

		grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_RED, 50);	
		grid_led_set_alert_frequency(&grid_led_state, -2);	
		grid_led_set_alert_phase(&grid_led_state, 100);	
		
	}
	

	///////////////////// PART 1 Old receive task

	if	(por->rx_double_buffer_status == 0){
		
		if (por->type == GRID_PORT_TYPE_USART){ // This is GRID usart port

			grid_port_timeout_try_disconect(por);					
		}
		
		grid_port_rxdobulebuffer_seek_newline(por);
	}
	
	////////////////// PART 2
	
	// No complete message in buffer
	if (por->rx_double_buffer_status == 0){
		return;
	}


	uint32_t length = 0;
	
	if (por->rx_double_buffer_read_start_index < por->rx_double_buffer_seek_start_index){
		length = por->rx_double_buffer_seek_start_index - por->rx_double_buffer_read_start_index + 1;
	}
	else{
		length = GRID_DOUBLE_BUFFER_RX_SIZE + por->rx_double_buffer_seek_start_index - por->rx_double_buffer_read_start_index + 1;
	}
	

	grid_port_receive_decode(por, length);
	

	
	por->rx_double_buffer_status = 0;
	
	
	
}

uint8_t grid_msg_is_position_transformable(int8_t received_x, int8_t received_y){
	// Position is transformabe if x and y positions do not indicate global message or editor message

	if (received_x + GRID_PARAMETER_DEFAULT_POSITION == 0 && received_y + GRID_PARAMETER_DEFAULT_POSITION == 0)
	{
		// EDITOR GENERATED GLOBAL MESSAGE
		return false;
		
	}
	else if (received_x + GRID_PARAMETER_DEFAULT_POSITION == 255 && received_y + GRID_PARAMETER_DEFAULT_POSITION == 255){
		
		// GRID GENERATED GLOBAL MESSAGE
		return false;
		
	}
	else{
		
		// Normal grid message
		return true;

	}

}

void grid_msg_string_transform_brc_params(char* message, int8_t dx, int8_t dy, uint8_t partner_fi){

	uint8_t error = 0;

	uint8_t received_session = grid_msg_string_get_parameter(message, GRID_BRC_SESSION_offset, GRID_BRC_SESSION_length, &error);
	uint8_t received_msgage = grid_msg_string_get_parameter(message, GRID_BRC_MSGAGE_offset, GRID_BRC_MSGAGE_length, &error);
	
	// Read the received destination X Y values (SIGNED INT)
	int8_t received_dx  = grid_msg_string_get_parameter(message, GRID_BRC_DX_offset, GRID_BRC_DX_length, &error) - GRID_PARAMETER_DEFAULT_POSITION;
	int8_t received_dy  = grid_msg_string_get_parameter(message, GRID_BRC_DY_offset, GRID_BRC_DY_length, &error) - GRID_PARAMETER_DEFAULT_POSITION;
	
	// Read the received source X Y values (SIGNED INT)
	int8_t received_sx  = grid_msg_string_get_parameter(message, GRID_BRC_SX_offset, GRID_BRC_SX_length, &error) - GRID_PARAMETER_DEFAULT_POSITION;
	int8_t received_sy  = grid_msg_string_get_parameter(message, GRID_BRC_SY_offset, GRID_BRC_SY_length, &error) - GRID_PARAMETER_DEFAULT_POSITION;
	
	uint8_t received_rot = grid_msg_string_get_parameter(message, GRID_BRC_ROT_offset, GRID_BRC_ROT_length, &error);
	

	// DO THE DX DY AGE calculations
	
	
	int8_t rotated_dx = 0;
	int8_t rotated_dy = 0;

	int8_t rotated_sx = 0;
	int8_t rotated_sy = 0;
	
	uint8_t updated_rot = (received_rot + partner_fi)%4;

	// APPLY THE 2D ROTATION MATRIX
	
	if (partner_fi == 0){ // 0 deg
		rotated_dx  += received_dx;
		rotated_dy  += received_dy;

		rotated_sx  += received_sx;
		rotated_sy  += received_sy;
	}
	else if(partner_fi == 1){ // 90 deg
		rotated_dx  -= received_dy;
		rotated_dy  += received_dx;

		rotated_sx  -= received_sy;
		rotated_sy  += received_sx;
	}
	else if(partner_fi == 2){ // 180 deg
		rotated_dx  -= received_dx;
		rotated_dy  -= received_dy;

		rotated_sx  -= received_sx;
		rotated_sy  -= received_sy;
	}
	else if(partner_fi == 3){ // 270 deg
		rotated_dx  += received_dy;
		rotated_dy  -= received_dx;

		rotated_sx  += received_sy;
		rotated_sy  -= received_sx;
	}
	else{
		// TRAP INVALID MESSAGE
	}
	
	uint8_t updated_dx = rotated_dx + GRID_PARAMETER_DEFAULT_POSITION + dx;
	uint8_t updated_dy = rotated_dy + GRID_PARAMETER_DEFAULT_POSITION + dy;

	uint8_t updated_sx = rotated_sx + GRID_PARAMETER_DEFAULT_POSITION + dx;
	uint8_t updated_sy = rotated_sy + GRID_PARAMETER_DEFAULT_POSITION + dy;
	
	
	
	uint8_t updated_msgage = received_msgage+1;
	

	if (grid_msg_is_position_transformable(updated_dx, updated_dy)){
		
		// Update message with the new values
		grid_msg_string_set_parameter(message, GRID_BRC_DX_offset, GRID_BRC_DX_length, updated_dx, &error);
		grid_msg_string_set_parameter(message, GRID_BRC_DY_offset, GRID_BRC_DY_length, updated_dy, &error);

	}
	
	if (grid_msg_is_position_transformable(updated_sx, updated_sy)){
		
		// Update message with the new values
		grid_msg_string_set_parameter(message, GRID_BRC_SX_offset, GRID_BRC_SX_length, updated_sx, &error);
		grid_msg_string_set_parameter(message, GRID_BRC_SY_offset, GRID_BRC_SY_length, updated_sy, &error);
	}
	
	grid_msg_string_set_parameter(message, GRID_BRC_MSGAGE_offset, GRID_BRC_MSGAGE_length, updated_msgage, &error);
	grid_msg_string_set_parameter(message, GRID_BRC_ROT_offset, GRID_BRC_ROT_length, updated_rot, &error);
	grid_msg_string_set_parameter(message, GRID_BRC_PORTROT_offset, GRID_BRC_PORTROT_length, partner_fi, &error);

	// Recalculate and update the checksum
	uint16_t length = strlen(message);
	grid_msg_string_checksum_write(message, length, grid_msg_string_calculate_checksum_of_packet_string(message, length));



}



uint32_t grid_msg_recent_fingerprint_calculate(char* message){

	uint8_t error = 0;
	

	uint8_t received_id  = grid_msg_string_get_parameter(message, GRID_BRC_ID_offset, GRID_BRC_ID_length, &error);
	uint8_t received_session = grid_msg_string_get_parameter(message, GRID_BRC_SESSION_offset, GRID_BRC_SESSION_length, &error);
	int8_t updated_sx  = grid_msg_string_get_parameter(message, GRID_BRC_SX_offset, GRID_BRC_SX_length, &error) - GRID_PARAMETER_DEFAULT_POSITION;
	int8_t updated_sy  = grid_msg_string_get_parameter(message, GRID_BRC_SY_offset, GRID_BRC_SY_length, &error) - GRID_PARAMETER_DEFAULT_POSITION;
	
	uint32_t fingerprint = received_id*256*256*256 + updated_sx*256*256 + updated_sy*256 + received_session;

	return fingerprint;

}


void grid_port_receive_decode(struct grid_port* por, uint16_t len){

	// Copy data from cyrcular buffer to temporary linear array;
	char* message;
	
	uint16_t length = len;
	char buffer[length+1];

	// Store message in temporary buffer (MAXMSGLEN = 250 character)
	for (uint16_t i = 0; i<length; i++){
		buffer[i] = por->rx_double_buffer[(por->rx_double_buffer_read_start_index + i)%GRID_DOUBLE_BUFFER_RX_SIZE];
		por->rx_double_buffer[(por->rx_double_buffer_read_start_index + i)%GRID_DOUBLE_BUFFER_RX_SIZE]=0;
	}
	buffer[length] = 0;


	message = &buffer[0];
	
	// Clear data from rx double buffer
	for (uint16_t i = 0; i<length; i++){
		por->rx_double_buffer[(por->rx_double_buffer_read_start_index + i)%GRID_DOUBLE_BUFFER_RX_SIZE] = 0;
	}
	
	uint32_t readstartindex = por->rx_double_buffer_read_start_index;
	
	por->rx_double_buffer_read_start_index = (por->rx_double_buffer_read_start_index + length)%GRID_DOUBLE_BUFFER_RX_SIZE;
	por->rx_double_buffer_seek_start_index =  por->rx_double_buffer_read_start_index;
	
	por->rx_double_buffer_status = 0;
	

		
	// Correct the incorrect frame start location
	for (uint16_t i = 1; i<length; i++){
		
		if (buffer[i] == GRID_CONST_SOH){

			grid_platform_printf("FRAME START OFFSET: ");

			//grid_platform_printf("\r\n");
			grid_port_debug_printf("Frame Start Offset %d %d %d", buffer[0], buffer[1], i);
	
			
			length -= i;
			message = &buffer[i];
		
		}

		if (buffer[i] == '\n' && i<length-1){

			grid_port_debug_printf("Frame End Offset");
			length = i;
			break;
		}
		
	}

	// close the message string with terminating zero character
	message[length] = '\0';
	
	
	// frame validator
	if (message[0] != GRID_CONST_SOH || message[length-1] != GRID_CONST_LF){
		
		grid_port_debug_printf("Frame Error %d ", length);
		return;

	}
	

	// checksum validator
	uint8_t checksum_received = grid_msg_string_checksum_read(message, length);
	uint8_t checksum_calculated = grid_msg_string_calculate_checksum_of_packet_string(message, length);
	
	if (checksum_calculated != checksum_received){

		// INVALID CHECKSUM
		uint8_t error = 0;

		uint16_t packet_length  = grid_msg_string_get_parameter(message, GRID_BRC_LEN_offset, GRID_BRC_LEN_length, &error);
		grid_platform_printf("##CHK %d %d\r\n",  packet_length, length);
		grid_port_debug_printf("Checksum %02x %02x", checksum_calculated, checksum_received);
		return;
	}


			
	if (message[1] == GRID_CONST_BRC){ // Broadcast message
		
		uint8_t error=0;

		// update age, sx, sy, dx, dy, rot etc...
		grid_msg_string_transform_brc_params(message, por->dx, por->dy, por->partner_fi);

		uint32_t fingerprint = grid_msg_recent_fingerprint_calculate(message);		

		if (grid_msg_recent_fingerprint_find(&grid_msg_state, fingerprint)){
			// WE HAVE NOT HEARD THIS MESSAGE BEFORE
			// grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_PURPLE, 20);
			return;
		}
		
		// Check if we can store the message in rx buffer
		if (grid_buffer_write_size(&por->rx_buffer) >= length){
			
			grid_buffer_write_chunk(&por->rx_buffer, message, length);
			
			grid_msg_recent_fingerprint_store(&grid_msg_state, fingerprint);	
		}
			
	}
	else if (message[1] == GRID_CONST_DCT || message[2] == GRID_CONST_BELL){ // Direct Message
		
		uint8_t error=0;

		// reset timout counter
		por->rx_double_buffer_timestamp = grid_platform_rtc_get_micros();

		if (por->partner_status == 0){

			// CONNECT
			por->partner_fi = (message[3] - por->direction + 6)%4;
			por->partner_hwcfg = grid_msg_string_read_hex_string_value(&message[length-10], 2, &error);
			por->partner_status = 1;				
			
			// Print Direction for debugging
			grid_platform_printf("Connect %c\r\n", grid_port_get_name_char(por));	

			grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_GREEN, 50);	
			grid_led_set_alert_frequency(&grid_led_state, -2);	
			grid_led_set_alert_phase(&grid_led_state, 100);	

		}
		
	}
	else{ // Unknown Message Type
		
		grid_port_debug_printf("Unknown message type\r\n");
		
	}
	
	
}





//=============================== PROCESS INBOUND ==============================//


uint8_t grid_port_process_inbound(struct grid_port* por, uint8_t loopback){
	
	uint16_t packet_size = grid_buffer_read_size(&por->rx_buffer);
	
	if (packet_size == 0){	
		// NO PACKET IN RX BUFFER
		return 0;
	}
			
	const uint8_t port_count = 6;
	struct grid_port* default_port_array[] = {GRID_PORT_N, GRID_PORT_E, GRID_PORT_S, GRID_PORT_W, GRID_PORT_U, GRID_PORT_H};
		
	struct grid_port* target_port_array[port_count];

	uint8_t j=0;
	
	for(uint8_t i=0; i<port_count; i++){

		struct grid_port* next_port = default_port_array[i];

		if (next_port->partner_status == 0){
			continue;
		}

		if (next_port == por && loopback == false){
			continue;
		}

		target_port_array[j] = next_port;
		j++;	
	}
	uint8_t target_port_count = j;
	

		
	// Check all of the tx buffers for sufficient storage space
	
	for (uint8_t i=0; i<target_port_count; i++)
	{
		
		if (packet_size > grid_buffer_write_size(&target_port_array[i]->tx_buffer)){
			
			grid_platform_printf("Buffer Error: %d/%d \r\n", i, target_port_count);
			grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_BLUE, 128);
			
			// sorry one of the buffers cannot store the packet, we will try later
			return 0;
		}		
	}
	
	// Copy packet from source buffer to temp array
	 
	char buffer[packet_size];
	grid_buffer_read_chunk(&por->rx_buffer, buffer, packet_size);


	// Copy packet from temp array to target port buffer	
	for (uint8_t i=0; i<target_port_count; i++)
	{

		struct grid_port* target_port = target_port_array[i];
		grid_buffer_write_chunk(&target_port->tx_buffer, buffer, packet_size);

	}

	return 1;
	
		
}






void grid_port_init(struct grid_port** por, uint8_t type, uint8_t dir){

	
	grid_buffer_init(&(*por)->tx_buffer, GRID_BUFFER_SIZE);
	grid_buffer_init(&(*por)->rx_buffer, GRID_BUFFER_SIZE);
	
	
	(*por)->cooldown = 0;
	
	
	(*por)->direction = dir;
	
	(*por)->type		= type;
	
	(*por)->tx_double_buffer_status	= 0;
	(*por)->rx_double_buffer_status	= 0;
	(*por)->rx_double_buffer_read_start_index = 0;
	(*por)->rx_double_buffer_seek_start_index = 0;
	(*por)->rx_double_buffer_write_index = 0;
	
	
	for (uint32_t i=0; i<GRID_DOUBLE_BUFFER_TX_SIZE; i++){
		(*por)->tx_double_buffer[i] = 0;		
	}
	for (uint32_t i=0; i<GRID_DOUBLE_BUFFER_RX_SIZE; i++){
		(*por)->rx_double_buffer[i] = 0;
	}
	
	(*por)->partner_fi = 0;
	
	(*por)->partner_hwcfg = 0;
	(*por)->partner_status = 1;
	
	(*por)->ping_local_token = 255;
	(*por)->ping_partner_token = 255;
	
	(*por)->ping_flag = 0;
	
	if (type == GRID_PORT_TYPE_USART){	
		
		(*por)->partner_status = 0;
		(*por)->partner_fi = 0;
		
		
		sprintf((char*) (*por)->ping_packet, "%c%c%c%c%02lx%02x%02x%c00\n", GRID_CONST_SOH, GRID_CONST_DCT, GRID_CONST_BELL, (*por)->direction, grid_sys_get_hwcfg(&grid_sys_state), 255, 255, GRID_CONST_EOT);
		
		(*por)->ping_packet_length = strlen((char*) (*por)->ping_packet);	
			
		grid_msg_string_checksum_write((*por)->ping_packet, (*por)->ping_packet_length, grid_msg_string_calculate_checksum_of_packet_string((*por)->ping_packet, (*por)->ping_packet_length));
		

		
		if ((*por)->direction == GRID_CONST_NORTH){
			(*por)->dx = 0;
			(*por)->dy = 1;
		}
		else if ((*por)->direction == GRID_CONST_EAST){
			(*por)->dx = 1;
			(*por)->dy = 0;
		}
		else if ((*por)->direction == GRID_CONST_SOUTH){
			(*por)->dx = 0;
			(*por)->dy = -1;
		}
		else if ((*por)->direction == GRID_CONST_WEST){
			(*por)->dx = -1;
			(*por)->dy = 0;
		}
		
	}
	else{
		(*por)->partner_status = 1; //UI AND USB are considered to be connected by default
	}
	
}

void grid_port_init_all(void){
	
	grid_port_init(&GRID_PORT_N, GRID_PORT_TYPE_USART, GRID_CONST_NORTH);
	grid_port_init(&GRID_PORT_E,  GRID_PORT_TYPE_USART, GRID_CONST_EAST);
	grid_port_init(&GRID_PORT_S, GRID_PORT_TYPE_USART, GRID_CONST_SOUTH);
	grid_port_init(&GRID_PORT_W,  GRID_PORT_TYPE_USART, GRID_CONST_WEST);
	
	grid_port_init(&GRID_PORT_U, GRID_PORT_TYPE_UI, 0);
	grid_port_init(&GRID_PORT_H, GRID_PORT_TYPE_USB, 0);	
	
}



uint8_t grid_port_process_outbound_usart(struct grid_port* por){
	
	if (por->tx_double_buffer_status != 0){
		//port is busy, a transmission is already in progress!
		return 0;
	}

		
	uint16_t packet_size = grid_buffer_read_size(&por->tx_buffer);
	
	if (!packet_size){
		
		// NO PACKET IN RX BUFFER
		return 0;
	}else{
		
		// Let's transfer the packet to local memory
		grid_buffer_read_init(&por->tx_buffer);
		
		por->tx_double_buffer_status = packet_size;
		
		for (uint16_t i = 0; i<packet_size; i++){
			
			uint8_t character = grid_buffer_read_character(&por->tx_buffer);
			por->tx_double_buffer[i] = character;
			
		}
	

		// Let's acknowledge the transaction
		grid_buffer_read_acknowledge(&por->tx_buffer);
		
		//grid_platform_printf("%d %d ", por->tx_double_buffer_status, packet_size);
		
		grid_platform_send_grid_message(por->direction, por->tx_double_buffer, packet_size);
		
		
		return 1;
	}
	

	
}

uint8_t grid_port_process_outbound_usb(volatile struct grid_port* por){
	
			

	// OLD DEBUG IMPLEMENTATION
	
	
	uint16_t length = grid_buffer_read_size(&por->tx_buffer);
	
	if (!length){		
		// NO PACKET IN RX BUFFER
		return 0;	
	}
	
	
	// Clear the tx double buffer	
	for(uint16_t i=0; i<GRID_DOUBLE_BUFFER_TX_SIZE; i++){
		por->tx_double_buffer[i] = 0;
	}
		
	struct grid_msg_packet message;

	//grid_msg_packet_init(&grid_msg_state, &message, 0, 0);
	message.header_length = 0;
	message.body_length = 0;
	message.last_appended_length = 0;
	message.footer_length = 0;
	
	// Let's transfer the packet to local memory
	grid_buffer_read_init(&por->tx_buffer);
		
	for (uint16_t i = 0; i<length; i++){
			
		uint8_t nextchar = grid_buffer_read_character(&por->tx_buffer);
		
		grid_msg_packet_receive_char_by_char(&message, nextchar);
		por->tx_double_buffer[i] = nextchar;	
			
	}
				
	// Let's acknowledge the transfer	(should wait for partner to send ack)
	grid_buffer_read_acknowledge(&por->tx_buffer);
		

	// GRID-2-HOST TRANSLATOR
		
	uint8_t error=0;
				
	uint8_t current_start		= 0;
	uint8_t current_stop		= 0;
		
		
	uint8_t error_flag = 0;
							
	for (uint16_t i=0; i<message.body_length; i++){
			
		if (message.body[i] == GRID_CONST_STX){
			current_start = i;
		}
		else if (message.body[i] == GRID_CONST_ETX && current_start!=0){
			current_stop = i;
			uint8_t msg_class = grid_msg_packet_body_get_parameter(&message, current_start, GRID_PARAMETER_CLASSCODE_offset, GRID_PARAMETER_CLASSCODE_length);
			uint8_t msg_instr = grid_msg_packet_body_get_parameter(&message, current_start, GRID_INSTR_offset, GRID_INSTR_length);
											
			if (msg_class == GRID_CLASS_MIDI_code && msg_instr == GRID_INSTR_EXECUTE_code){
					
										
				uint8_t midi_channel = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_MIDI_CHANNEL_offset,		GRID_CLASS_MIDI_CHANNEL_length);
				uint8_t midi_command = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_MIDI_COMMAND_offset ,	GRID_CLASS_MIDI_COMMAND_length);
				uint8_t midi_param1  = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_MIDI_PARAM1_offset  ,	GRID_CLASS_MIDI_PARAM1_length);
				uint8_t midi_param2  = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_MIDI_PARAM2_offset  ,	GRID_CLASS_MIDI_PARAM2_length);
				
				//grid_platform_printf("midi: %d %d %d %d \r\n", midi_channel, midi_command, midi_param1, midi_param2);
				struct grid_midi_event_desc midievent;
								
				midievent.byte0 = 0<<4|midi_command>>4;
				midievent.byte1 = midi_command|midi_channel;
				midievent.byte2 = midi_param1;
				midievent.byte3 = midi_param2;
				
				if (grid_midi_tx_push(midievent)){
					grid_port_debug_print_text("MIDI TX: Packet Dropped!");
				};
				grid_midi_tx_pop(midievent);				
				
													
			}
			if (msg_class == GRID_CLASS_MIDISYSEX_code && msg_instr == GRID_INSTR_EXECUTE_code){
					

				uint16_t length = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_MIDISYSEX_LENGTH_offset,		GRID_CLASS_MIDISYSEX_LENGTH_length);

				//grid_platform_printf("midi: %d %d %d %d \r\n", midi_channel, midi_command, midi_param1, midi_param2);

								
				//grid_port_debug_printf("Midi Sysex received: %d", length);

				// https://www.usb.org/sites/default/files/midi10.pdf page 17 Table 4-2: Examples of Parsed MIDI Events in 32 -bit USB-MIDI Event Packets
				

				uint8_t first = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_MIDISYSEX_PAYLOAD_offset, GRID_CLASS_MIDISYSEX_PAYLOAD_length);
				uint8_t last = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_MIDISYSEX_PAYLOAD_offset + (length-1)*2, GRID_CLASS_MIDISYSEX_PAYLOAD_length);

				if (first != 0xF0 || last != 0xF7){
					grid_port_debug_printf("Sysex invalid: %d %d", first, last);
				}

				uint32_t number_of_packets_dropped = 0;

				struct grid_midi_event_desc midievent;
				for (uint16_t i=0; i<length;){

					midievent.byte0 = 0;
					midievent.byte1 = 0;
					midievent.byte2 = 0;
					midievent.byte3 = 0;

					midievent.byte1 = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_MIDISYSEX_PAYLOAD_offset+i*2, GRID_CLASS_MIDISYSEX_PAYLOAD_length);
					i++;
					if (i<length){
						midievent.byte2 = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_MIDISYSEX_PAYLOAD_offset+i*2, GRID_CLASS_MIDISYSEX_PAYLOAD_length);
						i++;
					}
					if (i<length){
						midievent.byte3 =  grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_MIDISYSEX_PAYLOAD_offset+i*2, GRID_CLASS_MIDISYSEX_PAYLOAD_length);
						i++;
					}

					if (length<4){  //shortsysex
						if (length == 2){
							midievent.byte0 = 0<<4 | 6;
						}
						if (length == 3){
							midievent.byte0 = 0<<4 | 7;
						}
					}
					else if (i<4){ //first eventpacket of longsysex
						midievent.byte0 = 0<<4 | 4;
					}
					else{ // how many useful bytes are in this eventpacket
						if (i%3 == 0){ // 3
							midievent.byte0 = 0<<4 | 7;
						}
						else if (i%3 == 1){ // 1
							midievent.byte0 = 0<<4 | 5;
						}
						else if (i%3 == 2){ // 2
							midievent.byte0 = 0<<4 | 6;
						}
					}

					//grid_port_debug_printf("Packet: %d %d %d %d", midievent.byte0, midievent.byte1, midievent.byte2, midievent.byte3);
					number_of_packets_dropped += grid_midi_tx_push(midievent);
					// try to pop
					grid_midi_tx_pop(midievent);	
					
				}

				if (number_of_packets_dropped){
					grid_port_debug_printf("MIDI TX: %d Packet(s) Dropped!", number_of_packets_dropped);
				};
							
				
													
			}
			// else if (msg_class == GRID_CLASS_HIDMOUSEBUTTONIMMEDIATE_code && msg_instr == GRID_INSTR_EXECUTE_code){
					
										
			// 	uint8_t state = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_HIDMOUSEBUTTON_STATE_offset, GRID_CLASS_HIDMOUSEBUTTON_STATE_length);
			// 	uint8_t button = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_HIDMOUSEBUTTON_BUTTON_offset ,	GRID_CLASS_HIDMOUSEBUTTON_BUTTON_length);
			
			// 	//grid_port_debug_printf("MouseButton: %d %d", state, button);	
				
			// 	hiddf_mouse_button_change(state, button);
													
			// }
			// else if (msg_class == GRID_CLASS_HIDMOUSEMOVEIMMEDIATE_code && msg_instr == GRID_INSTR_EXECUTE_code){
					
										
			// 	uint8_t position_raw = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_HIDMOUSEMOVE_POSITION_offset, GRID_CLASS_HIDMOUSEMOVE_POSITION_length);
			// 	uint8_t axis = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_HIDMOUSEMOVE_AXIS_offset ,	GRID_CLASS_HIDMOUSEMOVE_AXIS_length);
			
			// 	int8_t position = position_raw - 128;

			// 	//grid_port_debug_printf("MouseMove: %d %d", position, axis);	
				
			// 	hiddf_mouse_move(position, axis);
													
			// }
			else if (msg_class == GRID_CLASS_HIDMOUSEBUTTON_code && msg_instr == GRID_INSTR_EXECUTE_code){
					
										
				uint8_t state = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_HIDMOUSEBUTTON_STATE_offset, GRID_CLASS_HIDMOUSEBUTTON_STATE_length);
				uint8_t button = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_HIDMOUSEBUTTON_BUTTON_offset ,	GRID_CLASS_HIDMOUSEBUTTON_BUTTON_length);
			
				//grid_port_debug_printf("MouseButton: %d %d", state, button);	

				struct grid_keyboard_event_desc key;
			
				key.ismodifier 	= 3; // 0: no, 1: yes, 2: mousemove, 3: mousebutton, f: delay
				key.ispressed 	= state;
				key.keycode 	= button;
				key.delay 		= 1;

				if (0 != grid_keyboard_tx_push(key)){
					grid_port_debug_printf("MOUSE: Packet Dropped!");
				};
													
			}
			else if (msg_class == GRID_CLASS_HIDMOUSEMOVE_code && msg_instr == GRID_INSTR_EXECUTE_code){
											
				uint8_t position_raw = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_HIDMOUSEMOVE_POSITION_offset, GRID_CLASS_HIDMOUSEMOVE_POSITION_length);
				uint8_t axis = grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_HIDMOUSEMOVE_AXIS_offset ,	GRID_CLASS_HIDMOUSEMOVE_AXIS_length);
			
				int8_t position = position_raw - 128;

				struct grid_keyboard_event_desc key;
			
				key.ismodifier 	= 2; // 0: no, 1: yes, 2: mousemove, 3: mousebutton, f: delay
				key.ispressed 	= position_raw;
				key.keycode 	= axis;
				key.delay 		= 1;

				if (0 != grid_keyboard_tx_push(key)){
					grid_port_debug_printf("MOUSE: Packet Dropped!");
				};
													
			}
			else if (msg_class == GRID_CLASS_HIDKEYBOARD_code && msg_instr == GRID_INSTR_EXECUTE_code){
				
				uint16_t length =	grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_HIDKEYBOARD_LENGTH_offset,		GRID_CLASS_HIDKEYBOARD_LENGTH_length);
				
				uint8_t default_delay =	grid_msg_packet_body_get_parameter(&message, current_start, GRID_CLASS_HIDKEYBOARD_DEFAULTDELAY_offset,		GRID_CLASS_HIDKEYBOARD_DEFAULTDELAY_length);
				

				uint32_t number_of_packets_dropped = 0;

				for(uint16_t j=0; j<length*4; j+=4){
					
					uint8_t key_ismodifier =	grid_msg_packet_body_get_parameter(&message, current_start+j, GRID_CLASS_HIDKEYBOARD_KEYISMODIFIER_offset,	GRID_CLASS_HIDKEYBOARD_KEYISMODIFIER_length);
					uint8_t key_state  =		grid_msg_packet_body_get_parameter(&message, current_start+j, GRID_CLASS_HIDKEYBOARD_KEYSTATE_offset,		GRID_CLASS_HIDKEYBOARD_KEYSTATE_length);
					uint8_t key_code =			grid_msg_packet_body_get_parameter(&message, current_start+j, GRID_CLASS_HIDKEYBOARD_KEYCODE_offset,		GRID_CLASS_HIDKEYBOARD_KEYCODE_length);


					struct grid_keyboard_event_desc key;

					
					if (key_ismodifier == 0 || key_ismodifier == 1){

						key.ismodifier 	= key_ismodifier;
						key.ispressed 	= key_state;
						key.keycode 	= key_code;
						key.delay 		= default_delay;

						if (key_state == 2){ // combined press and release

							key.ispressed 	= 1;
							number_of_packets_dropped += grid_keyboard_tx_push(key);
							key.ispressed 	= 0;
							number_of_packets_dropped += grid_keyboard_tx_push(key);

						}
						else{ // single press or release

							number_of_packets_dropped += grid_keyboard_tx_push(key);

						}

					}
					else if (key_ismodifier == 0xf){
						// Special delay event

						uint16_t delay = grid_msg_packet_body_get_parameter(&message, current_start+j, GRID_CLASS_HIDKEYBOARD_DELAY_offset, GRID_CLASS_HIDKEYBOARD_DELAY_length);

						key.ismodifier 	= key_ismodifier;
						key.ispressed 	= 0;
						key.keycode 	= 0;
						key.delay 		= delay;

						number_of_packets_dropped += grid_keyboard_tx_push(key);

					}
					else{
						grid_platform_printf("invalid key_ismodifier parameter %d\r\n", key_ismodifier);
					}
					
					// key change fifo buffer

				}
				
				if (number_of_packets_dropped){
					grid_port_debug_printf("KEYBOARD: %d Packet(s) Dropped!", number_of_packets_dropped);
				};

			}
			else{
				
				// sprintf(&por->tx_double_buffer[output_cursor], "[UNKNOWN] -> Protocol: %d\n", msg_protocol);
				// output_cursor += strlen(&por->tx_double_buffer[output_cursor]);		
			}
				
			current_start = 0;
			current_stop = 0;
		}
			
						
	}		
		
		
	uint32_t packet_length = grid_msg_packet_get_length(&message);
	
	for (uint32_t i=0; i<packet_length; i++){
		
		por->tx_double_buffer[i] = grid_msg_packet_send_char_by_char(&message, i);

	}
			
	// Let's send the packet through USB
	grid_platform_usb_serial_write(por->tx_double_buffer, packet_length);


	return 0;
}



void grid_port_receiver_softreset(struct grid_port* por){

	por->partner_status = 0;
	por->rx_double_buffer_timestamp = grid_platform_rtc_get_micros();
	

	por->rx_double_buffer_seek_start_index = 0;
	por->rx_double_buffer_read_start_index = 0;
	por->rx_double_buffer_write_index = 0; 

	grid_platform_reset_grid_transmitter(por->direction);

	for(uint16_t i=0; i<GRID_DOUBLE_BUFFER_RX_SIZE; i++){
		por->rx_double_buffer[i] = 0;
	}
	
	for(uint16_t i=0; i<GRID_DOUBLE_BUFFER_TX_SIZE; i++){
		por->tx_double_buffer[i] = 0;
	}

}


void grid_port_receiver_hardreset(struct grid_port* por){

	grid_platform_printf("HARD: ");

	if (por == GRID_PORT_E){

		grid_platform_printf("*");
	}


	grid_platform_disable_grid_transmitter(por->direction);


	por->partner_status = 0;
	
	
	por->ping_partner_token = 255;
	por->ping_local_token = 255;
	
	grid_msg_string_write_hex_string_value(&por->ping_packet[8], 2, por->ping_partner_token);
	grid_msg_string_write_hex_string_value(&por->ping_packet[6], 2, por->ping_local_token);
	grid_msg_string_checksum_write(por->ping_packet, por->ping_packet_length, grid_msg_string_calculate_checksum_of_packet_string(por->ping_packet, por->ping_packet_length));


	
	por->rx_double_buffer_timestamp = grid_platform_rtc_get_micros();
	grid_platform_reset_grid_transmitter(por->direction);
	


	por->rx_double_buffer_seek_start_index = 0;
	por->rx_double_buffer_read_start_index = 0;
	por->rx_double_buffer_write_index = 0; 

	for(uint16_t i=0; i<GRID_DOUBLE_BUFFER_RX_SIZE; i++){
		por->rx_double_buffer[i] = 0;
	}
	
	for(uint16_t i=0; i<GRID_DOUBLE_BUFFER_TX_SIZE; i++){
		por->tx_double_buffer[i] = 0;
	}
	
	grid_platform_enable_grid_transmitter(por->direction);
	
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
	
	if (grid_buffer_write_size(&GRID_PORT_U->rx_buffer) >= message_length){

		grid_buffer_write_packet(&GRID_PORT_U->rx_buffer, msg);

		return 1;
	}
	else{
		
		return 0;
	}
	
	
}



void grid_port_ping_try_everywhere(void){

	//NEW PING
	struct grid_port* port[4] = {GRID_PORT_N, GRID_PORT_E, GRID_PORT_S, GRID_PORT_W};

	for (uint8_t i = 0; i<4; i++){

		struct grid_port* next_port = port[i];
		
		if (next_port->ping_flag == 0){
			// no need to ping yet!
			continue;
		}
		
		if (grid_buffer_write_size(&next_port->tx_buffer) < next_port->ping_packet_length){
			// not enough space in buffer!
			continue;
		}


		grid_buffer_write_chunk(&next_port->tx_buffer, next_port->ping_packet, next_port->ping_packet_length);

		next_port->ping_flag = 0;

	}	

}




void grid_protocol_nvm_erase_succcess_callback(){

	uint8_t lastheader_id = grid_msg_get_lastheader_id(&grid_msg_state, GRID_MSG_LASTHEADER_ERASE_INDEX);
		
	grid_msg_clear_lastheader(&grid_msg_state, GRID_MSG_LASTHEADER_ERASE_INDEX);

	// Generate ACKNOWLEDGE RESPONSE
	struct grid_msg_packet response;
		
	grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

	// acknowledge
	grid_msg_packet_body_append_printf(&response, GRID_CLASS_NVMERASE_frame);
	grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);
	grid_msg_packet_body_append_parameter(&response, GRID_CLASS_NVMERASE_LASTHEADER_offset, GRID_CLASS_NVMERASE_LASTHEADER_length, lastheader_id);		


	// debugtext
	grid_msg_packet_body_append_printf(&response, GRID_CLASS_DEBUGTEXT_frame_start);		
	grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);
	grid_msg_packet_body_append_printf(&response, "xxerase complete");				
	grid_msg_packet_body_append_printf(&response, GRID_CLASS_DEBUGTEXT_frame_end);	

	grid_msg_packet_close(&grid_msg_state, &response);

	grid_port_packet_send_everywhere(&response);


		
	grid_keyboard_enable(&grid_keyboard_state);

	grid_ui_page_load(&grid_ui_state, grid_ui_page_get_activepage(&grid_ui_state));

}



void grid_protocol_nvm_clear_succcess_callback(){


	uint8_t lastheader_id = grid_msg_get_lastheader_id(&grid_msg_state, GRID_MSG_LASTHEADER_CLEAR_INDEX);
	grid_msg_clear_lastheader(&grid_msg_state, GRID_MSG_LASTHEADER_CLEAR_INDEX);

	struct grid_msg_packet response;

	grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

	// acknowledge
	grid_msg_packet_body_append_printf(&response, GRID_CLASS_PAGECLEAR_frame);
	grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);
	grid_msg_packet_body_append_parameter(&response, GRID_CLASS_PAGECLEAR_LASTHEADER_offset, GRID_CLASS_PAGECLEAR_LASTHEADER_length, lastheader_id);		
				
	// debugtext
	grid_msg_packet_body_append_printf(&response, GRID_CLASS_DEBUGTEXT_frame_start);		
	grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);
	grid_msg_packet_body_append_printf(&response, "xxclear complete");				
	grid_msg_packet_body_append_printf(&response, GRID_CLASS_DEBUGTEXT_frame_end);

	grid_msg_packet_close(&grid_msg_state, &response);
	grid_port_packet_send_everywhere(&response);



	// clear template variable after clear command
	grid_ui_page_clear_template_parameters(&grid_ui_state, grid_ui_page_get_activepage(&grid_ui_state));

	grid_ui_page_load(&grid_ui_state, grid_ui_page_get_activepage(&grid_ui_state));

}

void grid_protocol_nvm_read_succcess_callback(){

	uint8_t lastheader_id = grid_msg_get_lastheader_id(&grid_msg_state, GRID_MSG_LASTHEADER_DISCARD_INDEX);
	grid_msg_clear_lastheader(&grid_msg_state, GRID_MSG_LASTHEADER_DISCARD_INDEX);

	// Generate ACKNOWLEDGE RESPONSE
	struct grid_msg_packet response;	
	grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);
	grid_msg_packet_body_append_printf(&response, GRID_CLASS_PAGEDISCARD_frame);
	grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);
	grid_msg_packet_body_append_parameter(&response, GRID_CLASS_PAGEDISCARD_LASTHEADER_offset, GRID_CLASS_PAGEDISCARD_LASTHEADER_length, lastheader_id);

	grid_msg_packet_body_append_printf(&response, GRID_CLASS_DEBUGTEXT_frame_start);		
	grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);
	grid_msg_packet_body_append_printf(&response, "xxread complete");				
	grid_msg_packet_body_append_printf(&response, GRID_CLASS_DEBUGTEXT_frame_end);

	grid_msg_packet_close(&grid_msg_state, &response);
	grid_port_packet_send_everywhere(&response);



	grid_keyboard_enable(&grid_keyboard_state);


	// phase out the animation
	grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_WHITE_DIM, 100);
	grid_led_set_alert_timeout_automatic(&grid_led_state);

}

void grid_protocol_nvm_store_succcess_callback(){

	uint8_t lastheader_id = grid_msg_get_lastheader_id(&grid_msg_state, GRID_MSG_LASTHEADER_STORE_INDEX);
	grid_msg_clear_lastheader(&grid_msg_state, GRID_MSG_LASTHEADER_STORE_INDEX);


	struct grid_msg_packet response;

	grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

	// acknowledge
	grid_msg_packet_body_append_printf(&response, GRID_CLASS_PAGESTORE_frame);
	grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);
	grid_msg_packet_body_append_parameter(&response, GRID_CLASS_PAGESTORE_LASTHEADER_offset, GRID_CLASS_PAGESTORE_LASTHEADER_length, lastheader_id);		

	// debugtext
	grid_msg_packet_body_append_printf(&response, GRID_CLASS_DEBUGTEXT_frame_start);		
	grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);
	grid_msg_packet_body_append_printf(&response, "xxstore complete offset 0x%x", grid_plaform_get_nvm_nextwriteoffset());				
	grid_msg_packet_body_append_printf(&response, GRID_CLASS_DEBUGTEXT_frame_end);

	grid_msg_packet_close(&grid_msg_state, &response);
	grid_port_packet_send_everywhere(&response);


	//enable keyboard
	grid_keyboard_enable(&grid_keyboard_state);

	// phase out the animation
	grid_led_set_alert_timeout_automatic(&grid_led_state);

	// clear template variable after store command

	grid_ui_page_clear_template_parameters(&grid_ui_state, grid_ui_page_get_activepage(&grid_ui_state));

	// reload configuration

	grid_ui_page_load(&grid_ui_state, grid_ui_page_get_activepage(&grid_ui_state));

}


void grid_protocol_nvm_defrag_succcess_callback(){

	uint8_t lastheader_id = grid_msg_get_lastheader_id(&grid_msg_state, GRID_MSG_LASTHEADER_ERASE_INDEX);
		
	grid_msg_clear_lastheader(&grid_msg_state, GRID_MSG_LASTHEADER_ERASE_INDEX);

}




void grid_protocol_send_heartbeat(){

	

	uint8_t portstate = (GRID_PORT_N->partner_status<<0) | (GRID_PORT_E->partner_status<<1) | (GRID_PORT_S->partner_status<<2) | (GRID_PORT_W->partner_status<<3);



	struct grid_msg_packet response;

	grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

	grid_msg_packet_body_append_printf(&response, GRID_CLASS_HEARTBEAT_frame);

	grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);

	grid_msg_packet_body_append_parameter(&response, GRID_CLASS_HEARTBEAT_TYPE_offset, GRID_CLASS_HEARTBEAT_TYPE_length, grid_msg_get_heartbeat_type(&grid_msg_state));
	grid_msg_packet_body_append_parameter(&response, GRID_CLASS_HEARTBEAT_HWCFG_offset, GRID_CLASS_HEARTBEAT_HWCFG_length, grid_sys_get_hwcfg(&grid_sys_state));
	grid_msg_packet_body_append_parameter(&response, GRID_CLASS_HEARTBEAT_VMAJOR_offset, GRID_CLASS_HEARTBEAT_VMAJOR_length, GRID_PROTOCOL_VERSION_MAJOR);
	grid_msg_packet_body_append_parameter(&response, GRID_CLASS_HEARTBEAT_VMINOR_offset, GRID_CLASS_HEARTBEAT_VMINOR_length, GRID_PROTOCOL_VERSION_MINOR);
	grid_msg_packet_body_append_parameter(&response, GRID_CLASS_HEARTBEAT_VPATCH_offset, GRID_CLASS_HEARTBEAT_VPATCH_length, GRID_PROTOCOL_VERSION_PATCH);

	grid_msg_packet_body_append_parameter(&response, GRID_CLASS_HEARTBEAT_PORTSTATE_offset, GRID_CLASS_HEARTBEAT_PORTSTATE_length, portstate);
		
	

	if (grid_msg_get_heartbeat_type(&grid_msg_state) == 1){	// I am usb connected deevice

		
		grid_msg_packet_body_append_printf(&response, GRID_CLASS_PAGEACTIVE_frame);
		grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);
		grid_msg_packet_body_append_parameter(&response, GRID_CLASS_PAGEACTIVE_PAGENUMBER_offset, GRID_CLASS_PAGEACTIVE_PAGENUMBER_length, grid_ui_state.page_activepage);

		//printf("DEBUG: %s\r\n", response.body);
	}



	grid_msg_packet_close(&grid_msg_state, &response);

	grid_port_packet_send_everywhere(&response);



}

//=============================== PROCESS OUTBOUND ==============================//


void grid_port_process_outbound_ui(struct grid_port* por){
	
	
	uint16_t length = grid_buffer_read_size(&por->tx_buffer);
	
	if (length == 0){
		// NO PACKET IN RX BUFFER
		return;
	}

		
	char message[GRID_PARAMETER_PACKET_maxlength] = {0};
	
	// Let's transfer the packet to local memory
	grid_buffer_read_init(&por->tx_buffer);
	
	for (uint16_t i = 0; i<length; i++){
		
		message[i] = grid_buffer_read_character(&por->tx_buffer);
		//usb_tx_double_buffer[i] = character;
				
	}

	grid_buffer_read_acknowledge(&por->tx_buffer);
	
	// GRID-2-UI TRANSLATOR
	
	uint8_t error=0;

	uint8_t id = grid_msg_string_get_parameter(message, GRID_BRC_ID_offset, GRID_BRC_ID_length, &error);
		
	uint8_t dx = grid_msg_string_get_parameter(message, GRID_BRC_DX_offset, GRID_BRC_DX_length, &error);
	uint8_t dy = grid_msg_string_get_parameter(message, GRID_BRC_DY_offset, GRID_BRC_DY_length, &error);

	uint8_t sx = grid_msg_string_get_parameter(message, GRID_BRC_SX_offset, GRID_BRC_SX_length, &error);
	uint8_t sy = grid_msg_string_get_parameter(message, GRID_BRC_SY_offset, GRID_BRC_SY_length, &error);

	uint8_t rot = grid_msg_string_get_parameter(message, GRID_BRC_ROT_offset, GRID_BRC_ROT_length, &error);
	uint8_t portrot = grid_msg_string_get_parameter(message, GRID_BRC_PORTROT_offset, GRID_BRC_PORTROT_length, &error);
		
	uint8_t position_is_me = 0;
	uint8_t position_is_global = 0;
	uint8_t position_is_local = 0;
		
	if (dx == GRID_PARAMETER_DEFAULT_POSITION && dy == GRID_PARAMETER_DEFAULT_POSITION){
		position_is_me = 1;
	}
	else if (dx == GRID_PARAMETER_GLOBAL_POSITION && dy==GRID_PARAMETER_GLOBAL_POSITION){
		position_is_global = 1;
	}
	else if (dx == GRID_PARAMETER_LOCAL_POSITION && dy==GRID_PARAMETER_LOCAL_POSITION){
		position_is_local = 1;
	}
	
		
	uint16_t current_start		= 0;
	uint16_t current_stop		= 0;
	uint16_t current_length		= 0;

	uint16_t start_count		= 0;
	uint16_t stop_count		= 0;
		
		
	uint8_t error_flag = 0;	
	
	
	for (uint16_t i=0; i<length; i++){

		if (message[i] == GRID_CONST_STX){

			current_start = i;
			start_count++;
		}
		else if (message[i] == GRID_CONST_ETX && current_start!=0 && (start_count-stop_count) == 1){	
			current_stop = i;
			current_length		= current_stop-current_start;
			stop_count++;

			uint8_t msg_class = grid_msg_string_read_hex_string_value(&message[current_start+GRID_PARAMETER_CLASSCODE_offset], GRID_PARAMETER_CLASSCODE_length, &error_flag);
			uint8_t msg_instr = grid_msg_string_read_hex_string_value(&message[current_start+GRID_INSTR_offset], GRID_INSTR_length, &error_flag);
	

	
			if (msg_class == GRID_CLASS_PAGEACTIVE_code){ // dont check address!
					
				uint8_t page = grid_msg_string_read_hex_string_value(&message[current_start+GRID_CLASS_PAGEACTIVE_PAGENUMBER_offset], GRID_CLASS_PAGEACTIVE_PAGENUMBER_length, &error_flag);
							
				
				if (msg_instr == GRID_INSTR_EXECUTE_code){ //SET BANK

					

					if (grid_ui_state.page_change_enabled == 1){

						//grid_port_debug_printf("TRY");
						grid_ui_page_load(&grid_ui_state, page);
						grid_sys_set_bank(&grid_sys_state, page);

					}
					else{

						//grid_port_debug_printf("DISABLE");
					}
												
				}

				if (msg_instr == GRID_INSTR_REPORT_code){ //GET BANK

			
					//printf("RX: %d %d\r\n", sx, sy);

					if (!(sx==GRID_PARAMETER_DEFAULT_POSITION && sy==GRID_PARAMETER_DEFAULT_POSITION)){

						//printf("RX: %s\r\n", &message[current_start]);
						if (grid_ui_state.page_negotiated == 0){

							grid_ui_state.page_negotiated = 1;
							grid_ui_page_load(&grid_ui_state, page);
							grid_sys_set_bank(&grid_sys_state, page);
							
						}
						



					}
												
				}

				
			}
			if (msg_class == GRID_CLASS_MIDI_code && msg_instr == GRID_INSTR_REPORT_code){
					
								
				uint8_t midi_channel = grid_msg_string_read_hex_string_value(&message[current_start+GRID_CLASS_MIDI_CHANNEL_offset], GRID_CLASS_MIDI_CHANNEL_length, &error_flag);	
				uint8_t midi_command = grid_msg_string_read_hex_string_value(&message[current_start+GRID_CLASS_MIDI_COMMAND_offset], GRID_CLASS_MIDI_COMMAND_length, &error_flag);	
				uint8_t midi_param1 = grid_msg_string_read_hex_string_value(&message[current_start+GRID_CLASS_MIDI_PARAM1_offset], GRID_CLASS_MIDI_PARAM1_length, &error_flag);	
				uint8_t midi_param2 = grid_msg_string_read_hex_string_value(&message[current_start+GRID_CLASS_MIDI_PARAM2_offset], GRID_CLASS_MIDI_PARAM2_length, &error_flag);		
				
				//printf("M: %d %d %d %d \r\n", midi_channel, midi_command, midi_param1, midi_param2);

				char temp[130] = {0};


				grid_lua_clear_stdo(&grid_lua_state);

				// add the received midi message to the dynamic fifo and set the high water mark if necessary
				sprintf(temp, "table.insert(midi_fifo, {%d, %d, %d, %d}) if #midi_fifo > midi_fifo_highwater then midi_fifo_highwater = #midi_fifo end", midi_channel, midi_command, midi_param1, midi_param2);
				grid_lua_dostring(&grid_lua_state, temp);

				grid_lua_clear_stdo(&grid_lua_state);

				struct grid_ui_element* ele = &grid_ui_state.element_list[grid_ui_state.element_list_length-1];
				struct grid_ui_event* eve = NULL;

				eve = grid_ui_event_find(ele, GRID_UI_EVENT_MIDIRX);
				if (eve != NULL){

					grid_ui_event_trigger(eve);

				}



													
			}
			if (msg_class == GRID_CLASS_PAGECOUNT_code && (position_is_global || position_is_me)){
			
				if (msg_instr == GRID_INSTR_FETCH_code){ //get page count

					struct grid_msg_packet response;
											
					grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

					char response_payload[50] = {0};
					snprintf(response_payload, 49, GRID_CLASS_PAGECOUNT_frame);

					grid_msg_packet_body_append_text(&response, response_payload);
						
					grid_msg_packet_body_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);					
											
					grid_msg_packet_body_set_parameter(&response, 0, GRID_CLASS_PAGECOUNT_PAGENUMBER_offset, GRID_CLASS_PAGECOUNT_PAGENUMBER_length, grid_ui_state.page_count);

					grid_msg_packet_close(&grid_msg_state, &response);
					grid_port_packet_send_everywhere(&response);
						

												
				}
				
			}
			else if (msg_class == GRID_CLASS_IMEDIATE_code && msg_instr == GRID_INSTR_EXECUTE_code && (position_is_global || position_is_me || position_is_local)){

				uint16_t length = grid_msg_string_read_hex_string_value(&message[current_start+GRID_CLASS_IMEDIATE_ACTIONLENGTH_offset], GRID_CLASS_IMEDIATE_ACTIONLENGTH_length, &error_flag);
				char lua_script[200] = {0};
				strncpy(lua_script, &message[current_start+GRID_CLASS_IMEDIATE_ACTIONSTRING_offset], length);


				if (0 == strncmp(lua_script, "<?lua ", 6) && lua_script[length-3] == ' ' && lua_script[length-2] == '?' && lua_script[length-1] == '>'){
				
				
					printf("IMEDIATE %d: %s\r\n", length, lua_script);
					
					lua_script[length-3] = '\0';
					grid_lua_dostring(&grid_lua_state, &lua_script[6]);

				
				}
				else{
					printf("IMEDIATE NOT OK %d: %s\r\n", length, lua_script);
				}

				



			}
			else if (msg_class == GRID_CLASS_HEARTBEAT_code){
				
				uint8_t type  = grid_msg_string_read_hex_string_value(&message[current_start+GRID_CLASS_HEARTBEAT_TYPE_offset], GRID_CLASS_HEARTBEAT_TYPE_length, &error_flag);
				
				uint8_t editor_connected_now = 0;


				if (type == 0){
					// from other grid module
				}
				else if (type == 1){

					// from usb connected module
					int8_t received_sx = sx-GRID_PARAMETER_DEFAULT_POSITION; // convert to signed ind
					int8_t received_sy = sy-GRID_PARAMETER_DEFAULT_POSITION; // convert to signed ind
					int8_t rotated_sx = 0;
					int8_t rotated_sy = 0;

					// APPLY THE 2D ROTATION MATRIX
					
					//printf("Protrot %d \r\n", portrot);

					if (portrot == 0){ // 0 deg

						rotated_sx  -= received_sx;
						rotated_sy  -= received_sy;
					}
					else if(portrot == 1){ // 90 deg

						rotated_sx  -= received_sy;
						rotated_sy  += received_sx;
					}
					else if(portrot == 2){ // 180 deg

						rotated_sx  += received_sx;
						rotated_sy  += received_sy;
					}
					else if(portrot == 3){ // 270 deg

						rotated_sx  += received_sy;
						rotated_sy  -= received_sx;
					}
					else{
						// TRAP INVALID MESSAGE
					}

					grid_sys_set_module_x(&grid_sys_state, rotated_sx);
					grid_sys_set_module_y(&grid_sys_state, rotated_sy);
					grid_sys_set_module_rot(&grid_sys_state, rot);

				}
				else if (type >127){ // editor


					if (grid_sys_get_editor_connected_state(&grid_sys_state) == 0){

						grid_sys_set_editor_connected_state(&grid_sys_state, 1);
					
						editor_connected_now = 1;
						grid_port_debug_print_text("EDITOR connect");
					}

					grid_msg_set_editor_heartbeat_lastrealtime(&grid_msg_state, grid_platform_rtc_get_micros());

					if (type == 255){
						grid_ui_state.page_change_enabled = 1;
						//printf("255\r\n");
					}
					else{
						grid_ui_state.page_change_enabled = 0;
						//printf("254\r\n");
					}

					uint8_t ui_report_valid = 0;


					if (editor_connected_now){

						uint16_t report_length = 0;
						struct grid_msg_packet response;
												
						grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

						char response_payload[300] = {0};
						uint16_t len = 0;
						snprintf(&response_payload[len], 299, GRID_CLASS_EVENTPREVIEW_frame_start);
						len += strlen(&response_payload[len]);



						for(uint8_t j=0; j<grid_ui_state.element_list_length; j++){


							struct grid_ui_element* ele = &grid_ui_state.element_list[j];

							uint8_t element_num = ele->index;
							uint8_t element_value = 0;

							if (ele->type == GRID_UI_ELEMENT_POTENTIOMETER){

								element_value = ele->template_parameter_list[GRID_LUA_FNC_P_POTMETER_VALUE_index];
							}
							else if (ele->type == GRID_UI_ELEMENT_ENCODER){

								element_value = ele->template_parameter_list[GRID_LUA_FNC_E_ENCODER_VALUE_index];
							}
							
							report_length += 4;

							//printf("Element %d: %d\r\n", element_num, element_value);
							
							//grid_led_state.led_lowlevel_changed[j] = 0;

							ui_report_valid = 1;

							sprintf(&response_payload[len], "%02x%02x", element_num, element_value);
							len += strlen(&response_payload[len]);

						}


						sprintf(&response_payload[len], GRID_CLASS_EVENTPREVIEW_frame_end);
						len += strlen(&response_payload[len]);

						grid_msg_packet_body_append_text(&response, response_payload);
							

						grid_msg_packet_body_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);													
						grid_msg_packet_body_set_parameter(&response, 0, GRID_CLASS_EVENTPREVIEW_LENGTH_offset, GRID_CLASS_EVENTPREVIEW_LENGTH_length, report_length);
						
						grid_msg_packet_close(&grid_msg_state, &response);
						grid_port_packet_send_everywhere(&response);

						//printf(response.body);
						//printf("\r\n");
					}

					// report stringnames
					if (editor_connected_now){

						uint16_t report_length = 0;
						struct grid_msg_packet response;
												
						grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);


						// -1 to exclude system element
						for(uint8_t j=0; j<grid_ui_state.element_list_length-1; j++){

							struct grid_ui_element* ele = &grid_ui_state.element_list[j];
							
							uint8_t number = j;
							char command[26] = {0};

							sprintf(command, "gens(%d,ele[%d]:gen())", j, j);

							// lua get element name
							grid_lua_clear_stdo(&grid_lua_state);
							grid_lua_dostring(&grid_lua_state, command);

							grid_msg_packet_body_append_text(&response, grid_lua_state.stdo);

						}

						grid_msg_packet_close(&grid_msg_state, &response);
						grid_port_packet_send_everywhere(&response);

						//printf(response.body);
						//printf("\r\n");
					}


					// Report the state of the changed leds

					if (editor_connected_now){
						// reset the changed flags to force report all leds
						grid_led_change_flag_reset(&grid_led_state);
					}


					if (grid_protocol_led_change_report_length(&grid_led_state)){


						struct grid_msg_packet response;
												
						grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

						char response_payload[300] = {0};
						uint16_t len = 0;
						snprintf(response_payload, 299, GRID_CLASS_LEDPREVIEW_frame_start);
						len += strlen(&response_payload[len]);

						uint16_t report_length = grid_protocol_led_change_report_generate(&grid_led_state, -1, &response_payload[len]);

						len += strlen(&response_payload[len]);

						grid_msg_packet_body_append_text(&response, response_payload);


						grid_msg_packet_body_append_printf(&response, GRID_CLASS_LEDPREVIEW_frame_end);

						grid_msg_packet_body_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);													
						grid_msg_packet_body_set_parameter(&response, 0, GRID_CLASS_LEDPREVIEW_LENGTH_offset, GRID_CLASS_LEDPREVIEW_LENGTH_length, report_length);
						
						grid_msg_packet_close(&grid_msg_state, &response);
						grid_port_packet_send_everywhere(&response);

					}

					

					// from editor

				}
				else{
					// unknown type
				}
						
			}
			else if(msg_class == GRID_CLASS_SERIALNUMBER_code && msg_instr == GRID_INSTR_FETCH_code && (position_is_me || position_is_global)){
				
				uint32_t uniqueid[4] = {0};
				grid_sys_get_id(&grid_sys_state, uniqueid);					
				// Generate RESPONSE
				struct grid_msg_packet response;
										
				grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

				char response_payload[50] = {0};
				snprintf(response_payload, 49, GRID_CLASS_SERIALNUMBER_frame);

				grid_msg_packet_body_append_text(&response, response_payload);
					
				grid_msg_packet_body_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);					
										
				grid_msg_packet_body_set_parameter(&response, 0, GRID_CLASS_SERIALNUMBER_WORD0_offset, GRID_CLASS_SERIALNUMBER_WORD0_length, uniqueid[0]);
				grid_msg_packet_body_set_parameter(&response, 0, GRID_CLASS_SERIALNUMBER_WORD1_offset, GRID_CLASS_SERIALNUMBER_WORD1_length, uniqueid[1]);
				grid_msg_packet_body_set_parameter(&response, 0, GRID_CLASS_SERIALNUMBER_WORD2_offset, GRID_CLASS_SERIALNUMBER_WORD2_length, uniqueid[2]);
				grid_msg_packet_body_set_parameter(&response, 0, GRID_CLASS_SERIALNUMBER_WORD3_offset, GRID_CLASS_SERIALNUMBER_WORD3_length, uniqueid[3]);


										
				grid_msg_packet_close(&grid_msg_state, &response);
				grid_port_packet_send_everywhere(&response);
					

			}
			else if(msg_class == GRID_CLASS_UPTIME_code && msg_instr == GRID_INSTR_FETCH_code && (position_is_me || position_is_global)){

				// Generate RESPONSE
				struct grid_msg_packet response;
			
				grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

				char response_payload[50] = {0};
				snprintf(response_payload, 49, GRID_CLASS_UPTIME_frame);

				grid_msg_packet_body_append_text(&response, response_payload);
			
				grid_msg_packet_body_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);
			

				uint64_t uptime = grid_platform_rtc_get_micros();

				grid_msg_packet_body_set_parameter(&response, 0, GRID_CLASS_UPTIME_UPTIME_offset, GRID_CLASS_UPTIME_UPTIME_length, (uint32_t)uptime);
				
				
				uint32_t milliseconds = uptime/MS_TO_US%1000;
				uint32_t seconds = 		uptime/MS_TO_US/1000%60;
				uint32_t minutes =		uptime/MS_TO_US/1000/60%60;
				uint32_t hours =		uptime/MS_TO_US/1000/60/60%60;
				
			
				grid_msg_packet_close(&grid_msg_state, &response);
				grid_port_packet_send_everywhere(&response);
			
			
			}
			else if(msg_class == GRID_CLASS_RESETCAUSE_code && msg_instr == GRID_INSTR_FETCH_code && (position_is_me || position_is_global)){

				// Generate RESPONSE
				struct grid_msg_packet response;
				
				grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

				char response_payload[50] = {0};
				snprintf(response_payload, 49, GRID_CLASS_RESETCAUSE_frame);

				grid_msg_packet_body_append_text(&response, response_payload);
				
				grid_msg_packet_body_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);
				
				

				grid_msg_packet_body_set_parameter(&response, 0, GRID_CLASS_RESETCAUSE_CAUSE_offset, GRID_CLASS_RESETCAUSE_CAUSE_length, grid_platform_get_reset_cause());
		
				grid_msg_packet_close(&grid_msg_state, &response);
				grid_port_packet_send_everywhere(&response);
				
				
			}
			else if(msg_class == GRID_CLASS_RESET_code && msg_instr == GRID_INSTR_EXECUTE_code && (position_is_me)){

				grid_platform_system_reset();
			
			}				
		
			else if (msg_class == GRID_CLASS_PAGEDISCARD_code && msg_instr == GRID_INSTR_EXECUTE_code && (position_is_me || position_is_global)){
				

				grid_msg_store_lastheader(&grid_msg_state, GRID_MSG_LASTHEADER_DISCARD_INDEX, id);

				if (grid_ui_bulk_pageread_is_in_progress(&grid_ui_state)){

				}else{
					grid_ui_bulk_pageread_init(&grid_ui_state, &grid_protocol_nvm_read_succcess_callback);
				}


			}		
			else if (msg_class == GRID_CLASS_PAGEDISCARD_code && msg_instr == GRID_INSTR_CHECK_code && (position_is_me || position_is_global)){
				
				uint8_t state = grid_msg_get_lastheader_state(&grid_msg_state, GRID_MSG_LASTHEADER_DISCARD_INDEX);
				uint8_t id = grid_msg_get_lastheader_id(&grid_msg_state, GRID_MSG_LASTHEADER_DISCARD_INDEX);


				struct grid_msg_packet response;	
				grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);
				grid_msg_packet_body_append_printf(&response, GRID_CLASS_PAGEDISCARD_frame);
				grid_msg_packet_body_append_parameter(&response, GRID_CLASS_PAGEDISCARD_LASTHEADER_offset, GRID_CLASS_PAGEDISCARD_LASTHEADER_length, id);		


				if (state != -1){ // ACK
					grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);
				}		
				else{ // NACK
					grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_NACKNOWLEDGE_code);
				}	

				grid_msg_packet_close(&grid_msg_state, &response);
				grid_port_packet_send_everywhere(&response);	

			}			
			else if (msg_class == GRID_CLASS_PAGESTORE_code && msg_instr == GRID_INSTR_EXECUTE_code && (position_is_me || position_is_global)){
							
				grid_msg_store_lastheader(&grid_msg_state, GRID_MSG_LASTHEADER_STORE_INDEX, id);
				
				// start animation (it will be stopped in the callback function)
				grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_YELLOW_DIM, -1);		
				grid_led_set_alert_frequency(&grid_led_state, -4);	

				grid_ui_bulk_pagestore_init(&grid_ui_state, &grid_protocol_nvm_store_succcess_callback);					

			}			
			else if (msg_class == GRID_CLASS_PAGECLEAR_code && msg_instr == GRID_INSTR_EXECUTE_code && (position_is_me || position_is_global)){
			
				grid_msg_store_lastheader(&grid_msg_state, GRID_MSG_LASTHEADER_CLEAR_INDEX, id);			


				grid_ui_bulk_pageclear_init(&grid_ui_state, &grid_protocol_nvm_clear_succcess_callback);					

			}		
			else if (msg_class == GRID_CLASS_PAGESTORE_code && msg_instr == GRID_INSTR_CHECK_code && (position_is_me || position_is_global)){
				
				uint8_t state = grid_msg_get_lastheader_state(&grid_msg_state, GRID_MSG_LASTHEADER_STORE_INDEX);
				uint8_t id = grid_msg_get_lastheader_id(&grid_msg_state, GRID_MSG_LASTHEADER_STORE_INDEX);

				struct grid_msg_packet response;	
				grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);
				grid_msg_packet_body_append_printf(&response, GRID_CLASS_PAGESTORE_frame);
				grid_msg_packet_body_append_parameter(&response, GRID_CLASS_PAGESTORE_LASTHEADER_offset, GRID_CLASS_PAGESTORE_LASTHEADER_length, id);		
			
				if (state != -1 && 0 == grid_ui_bulk_pagestore_is_in_progress(&grid_ui_state)){ // ACK
					grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);
				}		
				else{ // NACK
					grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_NACKNOWLEDGE_code);
				}	

				grid_msg_packet_close(&grid_msg_state, &response);
				grid_port_packet_send_everywhere(&response);	

			}	
			else if (msg_class == GRID_CLASS_NVMERASE_code && msg_instr == GRID_INSTR_EXECUTE_code && (position_is_me || position_is_global)){

			
				if (current_length==5){ // erase all nvm configs

					grid_msg_store_lastheader(&grid_msg_state, GRID_MSG_LASTHEADER_ERASE_INDEX, id);
					grid_ui_bulk_nvmerase_init(&grid_ui_state, &grid_protocol_nvm_erase_succcess_callback);


					// start animation (it will be stopped in the callback function)
					grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_YELLOW_DIM, -1);	
					grid_led_set_alert_frequency(&grid_led_state, -2);	

					for (uint8_t i = 0; i<grid_led_get_led_count(&grid_led_state); i++){
						grid_led_set_layer_min(&grid_led_state, i, GRID_LED_LAYER_ALERT, GRID_LED_COLOR_YELLOW_DIM);
					}

				}
				else{
					grid_port_debug_printf("Erase: Invalid params!");
				}
				

			}		
			else if (msg_class == GRID_CLASS_NVMERASE_code && msg_instr == GRID_INSTR_CHECK_code && (position_is_me || position_is_global)){
				
				uint8_t state = grid_msg_get_lastheader_state(&grid_msg_state, GRID_MSG_LASTHEADER_ERASE_INDEX);
				uint8_t id = grid_msg_get_lastheader_id(&grid_msg_state, GRID_MSG_LASTHEADER_ERASE_INDEX);


				struct grid_msg_packet response;	
				grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);
				grid_msg_packet_body_append_printf(&response, GRID_CLASS_NVMERASE_frame);
				grid_msg_packet_body_append_parameter(&response, GRID_CLASS_NVMERASE_LASTHEADER_offset, GRID_CLASS_NVMERASE_LASTHEADER_length, id);		
			
				if (state != -1 && 0 == grid_ui_bulk_nvmerase_is_in_progress(&grid_ui_state)){ // ACK
					grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);
				}		
				else{ // NACK
					grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_NACKNOWLEDGE_code);
				}	

				grid_msg_packet_close(&grid_msg_state, &response);
				grid_port_packet_send_everywhere(&response);	

			}	
			else if (msg_class == GRID_CLASS_NVMDEFRAG_code && msg_instr == GRID_INSTR_EXECUTE_code && (position_is_me || position_is_global)){
			
				void grid_platform_nvm_defrag();

			}
			else if (msg_class == GRID_CLASS_CONFIG_code && msg_instr == GRID_INSTR_FETCH_code && (position_is_me || position_is_global)){
				
				//grid_platform_printf("CONFIG FETCH\r\n\r\n");
				
				uint8_t pagenumber = grid_msg_string_get_parameter(message, current_start+GRID_CLASS_CONFIG_PAGENUMBER_offset, GRID_CLASS_CONFIG_PAGENUMBER_length, NULL);
				uint8_t elementnumber = grid_msg_string_get_parameter(message, current_start+GRID_CLASS_CONFIG_ELEMENTNUMBER_offset, GRID_CLASS_CONFIG_ELEMENTNUMBER_length, NULL);
				uint8_t eventtype = grid_msg_string_get_parameter(message, current_start+GRID_CLASS_CONFIG_EVENTTYPE_offset, GRID_CLASS_CONFIG_EVENTTYPE_length, NULL);
				//uint16_t actionlength = grid_msg_string_get_parameter(message, current_start+GRID_CLASS_CONFIG_ACTIONLENGTH_offset, GRID_CLASS_CONFIG_ACTIONLENGTH_length, NULL);

				// Helper to map system element to 255
				if (elementnumber == 255){
					elementnumber = grid_ui_state.element_list_length - 1;
				}


				char temp[GRID_PARAMETER_ACTIONSTRING_maxlength]  = {0};

				grid_ui_event_recall_configuration(&grid_ui_state, pagenumber, elementnumber, eventtype, temp);
				
				//grid_platform_printf("CONFIG: %s\r\n\r\n", temp);


				if (strlen(temp) != 0){



					struct grid_msg_packet message;

					grid_msg_packet_init(&grid_msg_state, &message, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

					grid_msg_packet_body_append_printf(&message, GRID_CLASS_CONFIG_frame_start);

					grid_msg_packet_body_append_parameter(&message, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);
					
					grid_msg_packet_body_append_parameter(&message, GRID_CLASS_CONFIG_VERSIONMAJOR_offset, GRID_CLASS_CONFIG_VERSIONMAJOR_length, GRID_PROTOCOL_VERSION_MAJOR);
					grid_msg_packet_body_append_parameter(&message, GRID_CLASS_CONFIG_VERSIONMINOR_offset, GRID_CLASS_CONFIG_VERSIONMINOR_length, GRID_PROTOCOL_VERSION_MINOR);
					grid_msg_packet_body_append_parameter(&message, GRID_CLASS_CONFIG_VERSIONPATCH_offset, GRID_CLASS_CONFIG_VERSIONPATCH_length, GRID_PROTOCOL_VERSION_PATCH);

					// Helper to map system element to 255
					uint8_t element_helper = elementnumber;
					if (elementnumber == grid_ui_state.element_list_length - 1){
						element_helper = 255;
					}

					grid_msg_packet_body_append_parameter(&message, GRID_CLASS_CONFIG_PAGENUMBER_offset, GRID_CLASS_CONFIG_PAGENUMBER_length, pagenumber);
					grid_msg_packet_body_append_parameter(&message, GRID_CLASS_CONFIG_ELEMENTNUMBER_offset, GRID_CLASS_CONFIG_EVENTTYPE_length, element_helper);
					grid_msg_packet_body_append_parameter(&message, GRID_CLASS_CONFIG_EVENTTYPE_offset, GRID_CLASS_CONFIG_EVENTTYPE_length, eventtype);
					grid_msg_packet_body_append_parameter(&message, GRID_CLASS_CONFIG_ACTIONLENGTH_offset, GRID_CLASS_CONFIG_ACTIONLENGTH_length, 0);



					grid_msg_packet_body_append_parameter(&message, GRID_CLASS_CONFIG_ACTIONLENGTH_offset, GRID_CLASS_CONFIG_ACTIONLENGTH_length, strlen(temp));		
					grid_msg_packet_body_append_text(&message, temp);

					grid_msg_packet_body_append_printf(&message, GRID_CLASS_CONFIG_frame_end);


					//printf("CFG: %s\r\n", message.body);
					grid_msg_packet_close(&grid_msg_state, &message);
					grid_port_packet_send_everywhere(&message);
					

				}


			}
			else if (msg_class == GRID_CLASS_CONFIG_code && msg_instr == GRID_INSTR_EXECUTE_code){

				if (position_is_local || position_is_global || position_is_me){
					// disable hid automatically
					grid_keyboard_disable(&grid_keyboard_state);          
					//grid_port_debug_print_text("Disabling KB");

					uint8_t vmajor = grid_msg_string_get_parameter(message, current_start+GRID_CLASS_CONFIG_VERSIONMAJOR_offset, GRID_CLASS_CONFIG_VERSIONMAJOR_length, NULL);
					uint8_t vminor = grid_msg_string_get_parameter(message, current_start+GRID_CLASS_CONFIG_VERSIONMINOR_offset, GRID_CLASS_CONFIG_VERSIONMINOR_length, NULL);
					uint8_t vpatch = grid_msg_string_get_parameter(message, current_start+GRID_CLASS_CONFIG_VERSIONPATCH_offset, GRID_CLASS_CONFIG_VERSIONPATCH_length, NULL);
								
					if (vmajor == GRID_PROTOCOL_VERSION_MAJOR && vminor == GRID_PROTOCOL_VERSION_MINOR && vpatch == GRID_PROTOCOL_VERSION_PATCH){
						// version ok	
						//printf("version ok\r\n");
					}
					else{
						//printf("error.buf.config version mismatch\r\n");
					}

					uint8_t pagenumber = grid_msg_string_get_parameter(message, current_start+GRID_CLASS_CONFIG_PAGENUMBER_offset, GRID_CLASS_CONFIG_PAGENUMBER_length, NULL);
					uint8_t elementnumber = grid_msg_string_get_parameter(message, current_start+GRID_CLASS_CONFIG_ELEMENTNUMBER_offset, GRID_CLASS_CONFIG_ELEMENTNUMBER_length, NULL);
					uint8_t eventtype = grid_msg_string_get_parameter(message, current_start+GRID_CLASS_CONFIG_EVENTTYPE_offset, GRID_CLASS_CONFIG_EVENTTYPE_length, NULL);
					uint16_t actionlength = grid_msg_string_get_parameter(message, current_start+GRID_CLASS_CONFIG_ACTIONLENGTH_offset, GRID_CLASS_CONFIG_ACTIONLENGTH_length, NULL);


					if (elementnumber == 255){
						
						elementnumber = grid_ui_state.element_list_length - 1;
					}	

					char* action = &message[current_start + GRID_CLASS_CONFIG_ACTIONSTRING_offset];

					uint8_t ack = 0; // nacknowledge by default

					if (action[actionlength] == GRID_CONST_ETX){
						
						if (actionlength < GRID_PARAMETER_ACTIONSTRING_maxlength){

						action[actionlength] = 0;
						//printf("Config: %d %d %d %d -> %s\r\n", pagenumber, elementnumber, eventtype, actionlength, action);
						
							grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_WHITE, 64);
							if (pagenumber == grid_ui_state.page_activepage){

								//find event
								struct grid_ui_event* eve = grid_ui_event_find(&grid_ui_state.element_list[elementnumber], eventtype);
								
								if (eve != NULL){



									//register actionstring
									grid_ui_state.page_change_enabled = 0;
									grid_ui_event_register_actionstring(eve, action);
									//printf("Registered\r\n");
									//acknowledge
									ack = 1;

									//grid_port_debug_printf("autotrigger: %d", autotrigger);

									grid_ui_event_trigger_local(eve);	
									


								}

							}
							
							action[actionlength] = GRID_CONST_ETX;
						}
						else{
							grid_port_debug_printf("config too long");
						}

					}
					else{

						printf("Config frame invalid: %d %d %d %d end: %d %s\r\n", pagenumber, elementnumber, eventtype, actionlength, action[actionlength], message);

					}


					// Generate ACKNOWLEDGE RESPONSE
					struct grid_msg_packet response;

					grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);
					
					grid_msg_packet_body_append_printf(&response, GRID_CLASS_CONFIG_frame_check);
					
					grid_msg_packet_body_append_parameter(&response, GRID_CLASS_CONFIG_LASTHEADER_offset, GRID_CLASS_CONFIG_LASTHEADER_length, id);
					
					if (ack == 1){

						grid_msg_store_lastheader(&grid_msg_state, GRID_MSG_LASTHEADER_CONFIG_INDEX, id);
						grid_msg_clear_lastheader(&grid_msg_state, GRID_MSG_LASTHEADER_CONFIG_INDEX);

						grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);
						grid_port_debug_printf("Config %d", id);
					}
					else{
						grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_NACKNOWLEDGE_code);
						grid_port_debug_printf("Config Error %d", id);
					}
					
					grid_msg_packet_close(&grid_msg_state, &response);
					grid_port_packet_send_everywhere(&response);

				}
			}		
			else if (msg_class == GRID_CLASS_CONFIG_code && msg_instr == GRID_INSTR_CHECK_code && (position_is_me || position_is_global)){
				
				uint8_t state = grid_msg_get_lastheader_state(&grid_msg_state, GRID_MSG_LASTHEADER_CONFIG_INDEX);
				uint8_t id = grid_msg_get_lastheader_id(&grid_msg_state, GRID_MSG_LASTHEADER_CONFIG_INDEX);


				struct grid_msg_packet response;	
				grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);
				grid_msg_packet_body_append_printf(&response, GRID_CLASS_CONFIG_frame);
				grid_msg_packet_body_append_parameter(&response, GRID_CLASS_CONFIG_LASTHEADER_offset, GRID_CLASS_CONFIG_LASTHEADER_length, id);		
			
				if (state != -1){ // ACK
					grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);
				}		
				else{ // NACK
					grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_NACKNOWLEDGE_code);
				}	

				grid_msg_packet_close(&grid_msg_state, &response);
				grid_port_packet_send_everywhere(&response);	

			}	
			else if (msg_class == GRID_CLASS_HIDKEYSTATUS_code && msg_instr == GRID_INSTR_EXECUTE_code && (position_is_me || position_is_global)){
			
				uint8_t isenabled =	grid_msg_string_read_hex_string_value(&message[current_start+GRID_CLASS_HIDKEYSTATUS_ISENABLED_offset]		, GRID_CLASS_HIDKEYSTATUS_ISENABLED_length	, &error_flag);
				
				if (isenabled){
					grid_keyboard_enable(&grid_keyboard_state);
				}
				else{
					grid_keyboard_disable(&grid_keyboard_state);
				}

				
				// Generate ACKNOWLEDGE RESPONSE
				struct grid_msg_packet response;

				grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

				grid_msg_packet_body_append_printf(&response, GRID_CLASS_HIDKEYSTATUS_frame);

				grid_msg_packet_body_append_parameter(&response, GRID_CLASS_HIDKEYSTATUS_ISENABLED_offset, GRID_CLASS_HIDKEYSTATUS_ISENABLED_length, grid_keyboard_isenabled(&grid_keyboard_state));

				grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);


				grid_msg_packet_close(&grid_msg_state, &response);
				grid_port_packet_send_everywhere(&response);

			}
			else{
				//SORRY
			}
	
			current_start = 0;
			current_stop = 0;
		}


	}

	
	
	
}

