#ifndef GRID_PORT_H
#define GRID_PORT_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "grid_decode.h"
#include "grid_msg.h"
#include "grid_platform.h"
#include "grid_protocol.h"
#include "grid_swsr.h"

enum grid_port_type {
  GRID_PORT_UNDEFINED = 0, // TODO necessary?
  GRID_PORT_USART,
  GRID_PORT_USB,
  GRID_PORT_UI,
  GRID_PORT_TYPE_COUNT,
};
/*
enum grid_port_txrx {
  GRID_PORT_TX = 0,
  GRID_PORT_RX,
};
*/
enum grid_port_dir {
  GRID_PORT_NORTH = 0,
  GRID_PORT_EAST,
  GRID_PORT_SOUTH,
  GRID_PORT_WEST,
  GRID_PORT_DIR_COUNT,
};

char grid_port_dir_to_code(enum grid_port_dir dir);

struct grid_ping {
  uint8_t data[15];
  size_t size;
};

void grid_ping_init(struct grid_ping* ping, enum grid_port_dir dir);

struct grid_partner {
  uint64_t last_time;
  uint64_t timeout;
  enum grid_port_dir rot;
  bool connected;
  bool connected_prev;
};

void grid_partner_init(struct grid_partner* partner);
void grid_partner_set_rot(struct grid_partner* partner, enum grid_port_dir rot);

struct grid_port {

  enum grid_port_type type;
  // enum grid_port_txrx txrx;
  enum grid_port_dir dir;
  struct grid_partner partner;
  struct grid_ping ping;
  struct grid_swsr_t tx;
  struct grid_swsr_t rx;
  int8_t dx;
  int8_t dy;
};

void grid_port_malloc(struct grid_port* port, int swsr_capa);
void grid_port_free(struct grid_port* port);
void grid_port_init(struct grid_port* port, enum grid_port_type type, enum grid_port_dir dir);
struct grid_swsr_t* grid_port_get_tx(struct grid_port* port);
struct grid_swsr_t* grid_port_get_rx(struct grid_port* port);

bool grid_port_connected(struct grid_port* port);
bool grid_port_disconnected(struct grid_port* port);
void grid_port_connect(struct grid_port* port);
bool grid_port_connected_changed(struct grid_port* port);
void grid_port_connected_update(struct grid_port* port);

void grid_port_softreset(struct grid_port* port);

void grid_port_recv_msg(struct grid_port* port, uint8_t* msg, size_t size);
void grid_port_recv_msg_direct(struct grid_port* port, uint8_t* msg, size_t size);
void grid_port_recv_msg_broadcast(struct grid_port* port, uint8_t* msg, size_t size);
void grid_port_send_usart(struct grid_port* port);
void grid_port_send_usb(struct grid_port* port);
void grid_port_send_ui(struct grid_port* port);

void grid_str_transform_brc_params(char* msg, int8_t dx, int8_t dy, uint8_t partner_rot);

void grid_port_debug_print_text(char* str);
void grid_port_debug_printf(const char* fmt, ...);
void grid_port_websocket_print_text(char* str);
void grid_port_package_print_text(char* str);

#endif /* GRID_PORT_H */
