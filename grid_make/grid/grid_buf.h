#ifndef GRID_BUF_H_INCLUDED
#define GRID_BUF_H_INCLUDED

#include "grid_module.h"
#include "grid_sys.h"

#define GRID_BUFFER_SIZE	2000 //1000 this is the buffer for internal routing

#define GRID_DOUBLE_BUFFER_TX_SIZE	GRID_PARAMETER_PACKET_maxlength
#define GRID_DOUBLE_BUFFER_RX_SIZE	3000 //600



volatile uint8_t  grid_buffer_error_count;

struct grid_buffer{
	
	uint16_t buffer_length;
	uint8_t buffer_storage[GRID_BUFFER_SIZE];
	
	uint16_t read_start;
	uint16_t read_stop;
	uint16_t read_active;
	
	uint16_t read_length;
	
	uint16_t write_start;
	uint16_t write_stop;
	uint16_t write_active;
	
};


struct grid_port{
	
	uint32_t cooldown;


	struct usart_async_descriptor*    usart;
	uint8_t type;     // 0 undefined, 1 usart, 2 usb, 3 ui, 4 telemetry
	uint8_t direction;
	
	uint8_t dma_channel;
	
	uint16_t tx_double_buffer_status;
	
	uint32_t tx_double_buffer_ack_fingerprint;
	uint32_t tx_double_buffer_ack_timeout;
	
	uint8_t usart_error_flag;
	
	uint32_t rx_double_buffer_timeout; // is packet ready for verification
	
	uint32_t rx_double_buffer_status; // is packet ready for verification
	uint32_t rx_double_buffer_seek_start_index; // offset of next received byte in buffer
	uint32_t rx_double_buffer_read_start_index;
	
	uint8_t tx_double_buffer[GRID_DOUBLE_BUFFER_TX_SIZE];
	uint8_t rx_double_buffer[GRID_DOUBLE_BUFFER_RX_SIZE];
		
	
	struct grid_buffer tx_buffer;
	struct grid_buffer rx_buffer;
	
	uint32_t partner_hwcfg;
	uint8_t partner_fi;
	
	uint8_t ping_local_token;
	uint8_t ping_partner_token;
	
	uint8_t ping_packet[20];
	uint8_t ping_packet_length;
	
	uint8_t ping_flag;
		
	
	
	int8_t dx;
	int8_t dy;
	
	uint8_t partner_status;
	
};


volatile struct grid_port GRID_PORT_N;
volatile struct grid_port GRID_PORT_E;
volatile struct grid_port GRID_PORT_S;
volatile struct grid_port GRID_PORT_W;

volatile struct grid_port GRID_PORT_U;
volatile struct grid_port GRID_PORT_H;


void grid_port_reset_receiver(struct grid_port* por);

void grid_port_receive_decode(struct grid_port* por, uint16_t startcommand, uint16_t len);

void grid_port_receive_task(struct grid_port* por);



#define GRID_PORT_TYPE_UNDEFINED	0
#define GRID_PORT_TYPE_USART		1
#define GRID_PORT_TYPE_USB			2
#define GRID_PORT_TYPE_UI			3
#define GRID_PORT_TYPE_TELEMETRY	4






uint8_t grid_buffer_init(struct grid_buffer*  buf, uint16_t length);


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


void grid_port_init_all(void);

void grid_port_init(volatile struct grid_port* por, struct usart_async_descriptor*  usart, uint8_t type, uint8_t dir, uint8_t dma);

uint8_t grid_port_process_outbound_usart(struct grid_port* por);
uint8_t grid_port_process_outbound_usb(struct grid_port* por);
uint8_t grid_port_process_outbound_ui(struct grid_port* por);


void grid_debug_print_text(uint8_t* str);
void grid_websocket_print_text(uint8_t* str);

void grid_debug_printf(char const *fmt, ...);


uint8_t	grid_sys_packet_send_everywhere(struct grid_msg* msg);

#endif