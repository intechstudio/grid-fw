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

extern int32_t grid_platform_usb_serial_write(char *buffer, uint32_t length);

extern void grid_platform_printf(char const *fmt, ...);

extern void *grid_platform_allocate_volatile(size_t size);

extern void grid_platform_system_reset();
extern void grid_platform_nvm_defrag();

#define GRID_PORT_TYPE_UNDEFINED 0
#define GRID_PORT_TYPE_USART 1
#define GRID_PORT_TYPE_USB 2
#define GRID_PORT_TYPE_UI 3
#define GRID_PORT_TYPE_TELEMETRY 4

struct grid_port {

  uint32_t cooldown;

  uint8_t type; // 0 undefined, 1 usart, 2 usb, 3 ui, 4 telemetry
  uint8_t direction;
  uint8_t usart_error_flag;

  volatile uint16_t tx_double_buffer_status;
  volatile uint32_t tx_double_buffer_ack_fingerprint;
  volatile uint32_t tx_double_buffer_ack_timeout;

  volatile uint64_t
      rx_double_buffer_timestamp;            // is packet ready for verification
  volatile uint32_t rx_double_buffer_status; // is packet ready for verification
  volatile uint32_t
      rx_double_buffer_seek_start_index; // offset of next received byte in
                                         // buffer
  volatile uint32_t rx_double_buffer_read_start_index;
  volatile uint32_t rx_double_buffer_write_index;

  volatile char tx_double_buffer[GRID_DOUBLE_BUFFER_TX_SIZE];
  volatile char rx_double_buffer[GRID_DOUBLE_BUFFER_RX_SIZE];

  struct grid_buffer tx_buffer;
  struct grid_buffer rx_buffer;

  uint32_t partner_hwcfg;
  uint8_t partner_fi;

  uint8_t ping_local_token;
  uint8_t ping_partner_token;

  char ping_packet[20];
  uint8_t ping_packet_length;

  uint8_t ping_flag;

  uint8_t inbound_loopback;

  int8_t dx;
  int8_t dy;

  uint8_t partner_status;
};

struct grid_transport_model {
  uint8_t port_array_length;
  struct grid_port *port_array[10];
};

extern struct grid_transport_model grid_transport_state;

void grid_transport_init(struct grid_transport_model *transport);
void grid_transport_register_port(struct grid_transport_model *transport,
                                  struct grid_port *port);
struct grid_port *
grid_transport_get_port_first_of_type(struct grid_transport_model *transport,
                                      uint8_t type);
uint8_t
grid_transport_get_port_array_length(struct grid_transport_model *transport);
struct grid_port *
grid_transport_get_port(struct grid_transport_model *transport, uint8_t index);

void grid_port_receive_task(struct grid_port *por);
void grid_port_receive_decode(struct grid_port *por, uint16_t len);

uint8_t grid_port_process_inbound(struct grid_port *por);

char grid_port_get_name_char(struct grid_port *por);

void grid_port_init_all(void);
struct grid_port *grid_port_allocate(void);
void grid_port_init(struct grid_port *por, uint8_t type, uint8_t dir,
                    uint8_t inbound_loopback);

uint8_t grid_port_process_outbound_usart(struct grid_port *por);
uint8_t grid_port_process_outbound_usb(
    volatile struct grid_port *por); // dependency: USB ACM

void grid_port_receiver_softreset(struct grid_port *por);
void grid_port_receiver_hardreset(struct grid_port *por);

void grid_port_debug_print_text(char *str);
void grid_port_websocket_print_text(char *str);

void grid_port_debug_printf(char const *fmt, ...);

uint8_t grid_port_packet_send_everywhere(struct grid_msg_packet *msg);

void grid_port_ping_try_everywhere(void);

void grid_protocol_nvm_erase_succcess_callback();
void grid_protocol_nvm_clear_succcess_callback();
void grid_protocol_nvm_read_succcess_callback();
void grid_protocol_nvm_store_succcess_callback();
void grid_protocol_nvm_defrag_succcess_callback();

void grid_protocol_send_heartbeat();

void grid_port_process_outbound_ui(
    struct grid_port *por); // dependency: UI Page Load

#endif
