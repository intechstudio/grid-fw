#ifndef GRID_PORT_H_INCLUDED
#define GRID_PORT_H_INCLUDED

#include "grid_buf.h"
#include "grid_decode.h"
#include "grid_led.h"
#include "grid_lua_api.h"
#include "grid_msg.h"
#include "grid_protocol.h"
#include "grid_sys.h"
#include "grid_ui.h"
#include "grid_usb.h"

#include <stdarg.h>
#include <stdint.h>
#include <string.h>

extern uint64_t grid_platform_rtc_get_micros(void);

extern uint8_t grid_platform_disable_grid_transmitter(uint8_t direction);
extern uint8_t grid_platform_reset_grid_transmitter(uint8_t direction);
extern uint8_t grid_platform_enable_grid_transmitter(uint8_t direction);
extern uint32_t grid_plaform_get_nvm_nextwriteoffset();

extern int32_t grid_platform_usb_serial_write(char* buffer, uint32_t length);

extern void grid_platform_printf(char const* fmt, ...);

extern void* grid_platform_allocate_volatile(size_t size);

extern void grid_platform_system_reset();
extern void grid_platform_nvm_defrag();

enum grid_port_type { GRID_PORT_TYPE_UNDEFINED = 0, GRID_PORT_TYPE_USART, GRID_PORT_TYPE_USB, GRID_PORT_TYPE_UI };

// double buffer for ciclical use
struct grid_doublebuffer {

  uint8_t index;
  struct grid_transport* parent;

  uint16_t status;           // is packet ready for verification
  uint16_t seek_start_index; // offset of next received byte in buffer
  uint16_t read_start_index; // beginning of current packet
  uint16_t write_index;
  char* buffer_storage;
  size_t buffer_size;
};

struct grid_port {

  uint8_t index;
  struct grid_transport* parent;

  enum grid_port_type type;
  uint8_t direction;

  uint8_t partner_status;
  uint32_t partner_hwcfg;
  uint8_t partner_fi;

  uint64_t partner_last_timestamp;

  char ping_packet[20];
  uint8_t ping_packet_length;

  uint8_t ping_flag;

  int8_t dx;
  int8_t dy;
};

struct grid_transport_model {
  uint8_t port_array_length;
  struct grid_port* port_array[10];

  uint8_t doublebuffer_array_length;
  struct grid_doublebuffer* doublebuffer_tx_array[10];
  struct grid_doublebuffer* doublebuffer_rx_array[10];

  uint8_t buffer_array_length;
  struct grid_buffer* buffer_tx_array[10];
  struct grid_buffer* buffer_rx_array[10];
};

void grid_port_try_uart_timeout_disconect(struct grid_port* por, struct grid_doublebuffer* doublebuffer_rx);

extern struct grid_transport_model grid_transport_state;

void grid_transport_init(struct grid_transport_model* transport);

void grid_transport_register_port(struct grid_transport_model* transport, struct grid_port* port);
void grid_transport_register_buffer(struct grid_transport_model* transport, struct grid_buffer* buffer_tx, struct grid_buffer* buffer_rx);
void grid_transport_register_doublebuffer(struct grid_transport_model* transport, struct grid_doublebuffer* doublebuffer_tx, struct grid_doublebuffer* doublebuffer_rx);

struct grid_port* grid_transport_get_port_first_of_type(struct grid_transport_model* transport, enum grid_port_type type);
uint8_t grid_transport_get_port_array_length(struct grid_transport_model* transport);
struct grid_port* grid_transport_get_port(struct grid_transport_model* transport, uint8_t index);
struct grid_buffer* grid_transport_get_buffer_tx(struct grid_transport_model* transport, uint8_t index);
struct grid_buffer* grid_transport_get_buffer_rx(struct grid_transport_model* transport, uint8_t index);
struct grid_doublebuffer* grid_transport_get_doublebuffer_tx(struct grid_transport_model* transport, uint8_t index);
struct grid_doublebuffer* grid_transport_get_doublebuffer_rx(struct grid_transport_model* transport, uint8_t index);

void grid_port_rxdobulebuffer_to_linear(struct grid_port* por, struct grid_doublebuffer* doublebuffer_rx, char* message, uint16_t* length);
void grid_port_receive_decode(struct grid_port* por, struct grid_msg_recent_buffer* rec, char* message, uint16_t length);

void grid_port_receive_broadcast_message(struct grid_port* por, struct grid_buffer* rx_buffer, struct grid_msg_recent_buffer* rec, char* message, uint16_t length);
void grid_port_receive_direct_message(struct grid_port* por, char* message, uint16_t length);

uint8_t grid_port_process_inbound(struct grid_port* por, struct grid_buffer* rx_buffer);

char grid_port_get_name_char(struct grid_port* por);

struct grid_port* grid_port_allocate_init(uint8_t type, uint8_t dir);
struct grid_buffer* grid_buffer_allocate_init(size_t length);
struct grid_doublebuffer* grid_doublebuffer_allocate_init(size_t length);

uint8_t grid_port_process_outbound_usart(struct grid_port* por, struct grid_buffer* tx_buffer, struct grid_doublebuffer* tx_doublebuffer);
uint8_t grid_port_process_outbound_usb(struct grid_port* por, struct grid_buffer* tx_buffer, struct grid_doublebuffer* tx_doublebuffer);

void grid_port_receiver_softreset(struct grid_port* por, struct grid_doublebuffer* rx_doublebuffer);
void grid_port_receiver_hardreset(struct grid_port* por, struct grid_doublebuffer* rx_doublebuffer);

void grid_port_debug_print_text(char* str);
void grid_port_websocket_print_text(char* str);

void grid_port_debug_printf(char const* fmt, ...);

uint8_t grid_port_packet_send_everywhere(struct grid_msg_packet* msg);

void grid_port_ping_try_everywhere(void);

void grid_protocol_nvm_erase_succcess_callback(uint8_t lastheader_id);
void grid_protocol_nvm_clear_succcess_callback(uint8_t lastheader_id);
void grid_protocol_nvm_read_succcess_callback(uint8_t lastheader_id);
void grid_protocol_nvm_store_succcess_callback(uint8_t lastheader_id);

void grid_protocol_send_heartbeat();

void grid_port_process_outbound_ui(struct grid_port* por, struct grid_buffer* tx_buffer); // dependency: UI Page Load

#endif
