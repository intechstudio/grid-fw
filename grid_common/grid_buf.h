#ifndef GRID_BUF_H_INCLUDED
#define GRID_BUF_H_INCLUDED


// only for uint definitions
#include  <stdint.h>
// only for malloc
#include  <stdlib.h>

#include "grid_protocol.h"


extern uint32_t grid_buffer_error_count;

 
#define GRID_BUFFER_SIZE	2000 //1000 this is the buffer for internal routing



#define GRID_DOUBLE_BUFFER_TX_SIZE	GRID_PARAMETER_PACKET_maxlength
#define GRID_DOUBLE_BUFFER_RX_SIZE	3000 //600


struct grid_buffer{
	
	uint16_t buffer_length;
	uint8_t* buffer_storage;
	
	uint16_t read_start;
	uint16_t read_stop;
	uint16_t read_active;
	
	uint16_t read_length;
	
	uint16_t write_start;
	uint16_t write_stop;
	uint16_t write_active;
	
};



uint8_t grid_buffer_init(struct grid_buffer* buf, uint16_t length);


uint16_t grid_buffer_read_next_length();
uint8_t grid_buffer_read_character(struct grid_buffer* buf);
uint8_t grid_buffer_read_acknowledge();		// OK, delete
uint8_t grid_buffer_read_nacknowledge();	// Restart packet
uint8_t grid_buffer_read_cancel();			// Discard packet

uint16_t grid_buffer_get_space(struct grid_buffer* buf);

uint16_t grid_buffer_write_init(struct grid_buffer* buf, uint16_t length);
uint8_t  grid_buffer_write_character(struct grid_buffer* buf, uint8_t character);

uint8_t grid_buffer_write_acknowledge(struct grid_buffer* buf);
uint8_t grid_buffer_write_cancel(struct grid_buffer* buf);



#endif