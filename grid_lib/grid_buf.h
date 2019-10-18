#ifndef GRID_BUF_H_INCLUDED
#define GRID_BUF_H_INCLUDED

#include "sam.h"


#define GRID_BUFFER_TX_SIZE	200
#define GRID_BUFFER_RX_SIZE	200

#define GRID_DOUBLE_BUFFER_TX_SIZE	200
#define GRID_DOUBLE_BUFFER_RX_SIZE	200

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


#define GRID_PORT_TYPE_UNDEFINED	0
#define GRID_PORT_TYPE_USART		1
#define GRID_PORT_TYPE_USB			2
#define GRID_PORT_TYPE_UI			3
#define GRID_PORT_TYPE_TELEMETRY	4

typedef struct grid_port{

	struct usart_async_descriptor*    usart;	
	uint8_t type;     // 0 undefined, 1 usart, 2 usb, 3 ui, 4 telemetry
	uint8_t direction;
	
	uint8_t dma_channel;
	
	uint16_t tx_double_buffer_status;
	
	
	uint16_t rx_double_buffer_timeout; // is packet ready for verification
	
	uint16_t rx_double_buffer_status; // is packet ready for verification
	uint16_t rx_double_buffer_seek_start_index; // offset of next received byte in buffer
	uint16_t rx_double_buffer_read_start_index;
	
	uint8_t tx_double_buffer[GRID_DOUBLE_BUFFER_TX_SIZE];
	uint8_t rx_double_buffer[GRID_DOUBLE_BUFFER_RX_SIZE];
	
	
	
	
	
	
	GRID_BUFFER_t tx_buffer;
	GRID_BUFFER_t rx_buffer;
	
	uint32_t partner_hwcfg;
	uint8_t partner_fi;
	uint8_t partner_dx;
	uint8_t partner_dy;
	
	uint8_t partner_status;
	
} GRID_PORT_t;


volatile GRID_PORT_t GRID_PORT_N;
volatile GRID_PORT_t GRID_PORT_E;
volatile GRID_PORT_t GRID_PORT_S;
volatile GRID_PORT_t GRID_PORT_W;

volatile GRID_PORT_t GRID_PORT_U;
volatile GRID_PORT_t GRID_PORT_H;


uint8_t grid_port_packet_length(GRID_BUFFER_t* por);

uint8_t grid_port_packet_start(GRID_BUFFER_t* por);



uint8_t grid_buffer_init(GRID_BUFFER_t*  buf, uint16_t length);


uint16_t grid_buffer_read_next_length();
uint8_t grid_buffer_read_character(GRID_BUFFER_t* buf);
uint8_t grid_buffer_read_acknowledge();		// OK, delete
uint8_t grid_buffer_read_nacknowledge();	// Restart packet
uint8_t grid_buffer_read_cancel();			// Discard packet



uint16_t grid_buffer_write_init(GRID_BUFFER_t* buf, uint16_t length);
uint8_t  grid_buffer_write_character(GRID_BUFFER_t* buf, uint8_t character);

uint8_t grid_buffer_write_acknowledge(GRID_BUFFER_t* buf);
uint8_t grid_buffer_write_cancel(GRID_BUFFER_t* buf);




#endif