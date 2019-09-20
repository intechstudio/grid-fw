#ifndef GRID_BUF_H_INCLUDED
#define GRID_BUF_H_INCLUDED

#include "sam.h"




typedef struct grid_buffer{
	
	uint16_t buffer_length;
	uint8_t* buffer_storage;
	
	uint16_t read_start;
	uint16_t read_stop;
	uint16_t read_active;
	
	uint16_t read_length;
	
	uint16_t write_start;
	uint16_t write_stop;
	uint16_t write_active;
	
} GRID_BUFFER_t;


typedef struct grid_port{
	
	GRID_BUFFER_t tx_buffer;
	GRID_BUFFER_t rx_buffer;
		
	uint32_t partner_hwcfg;
	uint8_t partner_fi;
	uint8_t partner_dx;
	uint8_t partner_dy;
	
	uint8_t partner_status;
		
} GRID_PORT_t;



uint8_t grid_port_packet_length(struct grid_port* por);

uint8_t grid_port_packet_start(struct grid_port* por);



uint8_t grid_buffer_init(struct grid_buffer* buf, uint16_t length);


uint16_t grid_buffer_read_next_length();
uint8_t grid_buffer_read_character(struct grid_buffer* buf);
uint8_t grid_buffer_read_acknowledge();		// OK, delete
uint8_t grid_buffer_read_nacknowledge();	// Restart packet
uint8_t grid_buffer_read_cancel();			// Discard packet



uint16_t grid_buffer_write_init(struct grid_buffer* buf, uint16_t length);
uint8_t  grid_buffer_write_character(struct grid_buffer* buf, uint8_t character);

uint8_t grid_buffer_write_acknowledge(struct grid_buffer* buf);
uint8_t grid_buffer_write_cancel(struct grid_buffer* buf);




#endif