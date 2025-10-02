#ifndef GRID_TRANSPORT_H
#define GRID_TRANSPORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "grid_led.h"
#include "grid_port.h"
#include "grid_protocol.h"
#include "grid_swsr.h"

enum { GRID_PORT_SWSR_SIZE = GRID_PARAMETER_SPI_TRANSACTION_length * 2 };

typedef bool (*grid_brc_between_t)(enum grid_port_type t1, enum grid_port_type t2);

extern struct grid_transport grid_transport_state;

struct grid_transport {

  size_t port_count;
  struct grid_port* ports;
  uint8_t usart_send_offset;
};

void grid_transport_malloc(struct grid_transport* transport, size_t port_count);
void grid_transport_free(struct grid_transport* transport);
struct grid_port* grid_transport_get_port(struct grid_transport* transport, size_t idx, enum grid_port_type type, enum grid_port_dir dir);
void grid_transport_recv_usart(struct grid_transport* transport, uint8_t* msg, size_t size);
void grid_transport_recv_usb(struct grid_transport* transport, uint8_t* msg, size_t size);
void grid_transport_ping_all(struct grid_transport* transport);
void grid_transport_sendfull(struct grid_transport* transport);
void grid_transport_send_usart_cyclic_offset(struct grid_transport* transport);
void grid_transport_send_msg_to_all(struct grid_transport* transport, struct grid_msg* msg);
void grid_transport_heartbeat(struct grid_transport* transport, uint8_t type, uint32_t hwcfg, uint8_t activepage);
void grid_transport_rx_broadcast_tx(struct grid_transport* transport, struct grid_port* port, grid_brc_between_t between);

#endif /* GRID_TRANSPORT_H */
