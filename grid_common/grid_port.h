#ifndef GRID_PORT_H_INCLUDED
#define GRID_PORT_H_INCLUDED


#include "grid_buf.h"
#include "grid_msg.h"
#include "grid_sys.h"
#include "grid_protocol.h"

#include <stdint.h>
#include <stdarg.h>
#include <string.h>



extern uint8_t grid_platform_disable_grid_transmitter(uint8_t direction);
extern uint8_t grid_platform_reset_grid_transmitter(uint8_t direction);
extern uint8_t grid_platform_enable_grid_transmitter(uint8_t direction);


#define GRID_PORT_TYPE_UNDEFINED	0
#define GRID_PORT_TYPE_USART		1
#define GRID_PORT_TYPE_USB			2
#define GRID_PORT_TYPE_UI			3
#define GRID_PORT_TYPE_TELEMETRY	4



struct grid_port{
	
	uint32_t cooldown;

	uint8_t type;     // 0 undefined, 1 usart, 2 usb, 3 ui, 4 telemetry
	uint8_t direction;
	
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
	
	char ping_packet[20];
	uint8_t ping_packet_length;
	
	uint8_t ping_flag;
		
	
	
	int8_t dx;
	int8_t dy;
	
	uint8_t partner_status;
	
};



extern volatile struct grid_port GRID_PORT_N;
extern volatile struct grid_port GRID_PORT_E;
extern volatile struct grid_port GRID_PORT_S;
extern volatile struct grid_port GRID_PORT_W;

extern volatile struct grid_port GRID_PORT_U;
extern volatile struct grid_port GRID_PORT_H;




void grid_port_init_all(void);

void grid_port_init(struct grid_port* por, uint8_t type, uint8_t dir);


void grid_port_receiver_softreset(struct grid_port* por);
void grid_port_receiver_hardreset(struct grid_port* por);

void grid_port_debug_print_text(char* str);
void grid_port_websocket_print_text(char* str);

void grid_port_debug_printf(char const *fmt, ...);


uint8_t	grid_port_packet_send_everywhere(struct grid_msg_packet* msg);


void grid_port_ping_try_everywhere(void);



#endif