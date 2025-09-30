#include "grid_port.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "grid_platform.h"
#include "grid_protocol.h"

extern struct grid_decoder_collection* grid_decoder_to_ui_reference;
extern struct grid_decoder_collection* grid_decoder_to_usb_reference;

int grid_port_decode_class(struct grid_decoder_collection* decoder_collection, uint16_t class, char* header, char* chunk);

char grid_port_dir_to_code(enum grid_port_dir dir) {

  assert(dir < GRID_PORT_DIR_COUNT);

  return dir + GRID_CONST_NORTH;
}

enum grid_port_dir grid_port_code_to_dir(char code) {

  assert(GRID_CONST_WEST - GRID_CONST_NORTH == 3);
  assert(code >= GRID_CONST_NORTH && code <= GRID_CONST_WEST);

  return code - GRID_CONST_NORTH;
}

void grid_ping_init(struct grid_ping* ping, enum grid_port_dir dir) {

  assert(dir < GRID_PORT_DIR_COUNT);

  char* data = (char*)ping->data;

  char dircode = grid_port_dir_to_code(dir);
  sprintf(data, "%c%c%c%c%02x%02x%02x%c00\n", GRID_CONST_SOH, GRID_CONST_DCT, GRID_CONST_BELL, dircode, 255, 255, 255, GRID_CONST_EOT);

  ping->size = strlen(data);

  uint8_t chksum = grid_frame_calculate_checksum_packet((uint8_t*)data, ping->size);
  grid_str_checksum_set(data, ping->size, chksum);
}

void grid_partner_init(struct grid_partner* partner) {

  partner->last_time = 0;
  partner->timeout = 0;
  partner->rot = 0;
  partner->connected = false;
  partner->connected_prev = false;
}

void grid_partner_set_rot(struct grid_partner* partner, enum grid_port_dir rot) {

  assert(rot < GRID_PORT_DIR_COUNT);

  partner->rot = rot;
}

void grid_port_malloc(struct grid_port* port, int rx_capa, int tx_capa) {

  assert(rx_capa > 0);
  assert(tx_capa > 0);

  assert(grid_swsr_malloc(&port->rx, rx_capa) == 0);
  assert(grid_swsr_malloc(&port->tx, tx_capa) == 0);
}

void grid_port_free(struct grid_port* port) {

  grid_swsr_free(&port->tx);
  grid_swsr_free(&port->rx);
}

void grid_port_init(struct grid_port* port, enum grid_port_type type, enum grid_port_dir dir) {

  assert(type < GRID_PORT_TYPE_COUNT);
  assert(dir < GRID_PORT_DIR_COUNT);

  port->type = type;
  port->dir = dir;
  port->dx = 0;
  port->dy = 0;

  grid_partner_init(&port->partner);

  if (port->type == GRID_PORT_USART) {

    grid_ping_init(&port->ping, port->dir);

    int8_t dx[4] = {0, 1, 0, -1};
    int8_t dy[4] = {1, 0, -1, 0};

    port->dx = dx[port->dir];
    port->dy = dy[port->dir];
  }

  if (port->type == GRID_PORT_USB || port->type == GRID_PORT_UI) {

    grid_port_connect(port);
  }
}

struct grid_swsr_t* grid_port_get_tx(struct grid_port* port) { return &port->tx; }

struct grid_swsr_t* grid_port_get_rx(struct grid_port* port) { return &port->rx; }

bool grid_port_connected(struct grid_port* port) {

  if (!port->partner.connected) {
    return false;
  }

  switch (port->type) {
  case GRID_PORT_USART: {

    uint64_t last_time = port->partner.last_time;
    uint64_t elapsed = grid_platform_rtc_get_elapsed_time(last_time);

    return elapsed < GRID_PARAMETER_DISCONNECTTIMEOUT_us;

  } break;
  default:
    return true;
  }
}

bool grid_port_disconnected(struct grid_port* port) {

  assert(port->type == GRID_PORT_USART);

  return port->partner.connected && !grid_port_connected(port);
}

void grid_port_keepalive(struct grid_port* port) { port->partner.last_time = grid_platform_rtc_get_micros(); }

void grid_port_connect(struct grid_port* port) {

  grid_port_keepalive(port);

  port->partner.connected = true;
}

bool grid_port_connected_changed(struct grid_port* port) {

  assert(port->type == GRID_PORT_USART);

  return grid_port_connected(port) != port->partner.connected_prev;
}

void grid_port_connected_update(struct grid_port* port) {

  assert(port->type == GRID_PORT_USART);

  port->partner.connected = grid_port_connected(port);
  port->partner.connected_prev = port->partner.connected;
}

void grid_port_softreset(struct grid_port* port) {

  assert(port->type == GRID_PORT_USART);

  grid_platform_reset_grid_transmitter(port->dir);
}

void grid_port_recv_msg(struct grid_port* port, uint8_t* msg, size_t size) {

  switch (msg[1]) {
  case GRID_CONST_DCT: {

    grid_port_recv_msg_direct(port, msg, size);

  } break;
  case GRID_CONST_BRC: {

    if (!grid_port_connected(port)) {
      return;
    }

    grid_port_recv_msg_broadcast(port, msg, size);
  }
  }
}

void grid_port_recv_msg_direct(struct grid_port* port, uint8_t* msg, size_t size) {

  assert(msg[1] == GRID_CONST_DCT);

  if (msg[2] != GRID_CONST_BELL) {
    return;
  }

  if (!(msg[3] >= GRID_CONST_NORTH && msg[3] <= GRID_CONST_WEST)) {
    return;
  }

  grid_port_connect(port);

  enum grid_port_dir dir = grid_port_code_to_dir(msg[3]);

  grid_partner_set_rot(&port->partner, (dir - port->dir + 6) % 4);
}

void grid_port_recv_msg_broadcast(struct grid_port* port, uint8_t* msg, size_t size) {

  assert(msg[1] == GRID_CONST_BRC);

  grid_port_keepalive(port);

  struct grid_swsr_t* rx = grid_port_get_rx(port);

  if (!grid_swsr_writable(rx, size)) {
    return;
  }

  grid_swsr_write(rx, msg, size);
}

void grid_port_send_usart(struct grid_port* port) {

  assert(port->type == GRID_PORT_USART);

  if (grid_platform_get_frame_len(port->dir)) {
    return;
  }

  struct grid_swsr_t* tx = grid_port_get_tx(port);

  int ret = grid_swsr_until_msg_end(tx);

  if (ret < 0) {
    return;
  }

  grid_platform_send_frame(tx, ret + 1, port->dir);
}

void grid_port_send_usb(struct grid_port* port) {

  assert(port->type == GRID_PORT_USB);

  struct grid_swsr_t* tx = grid_port_get_tx(port);

  // Allocated statically due to the implementation of usb serial writes
  // sometimes requiring their buffer to live until the transfer completes
  static struct grid_msg msg;

  if (!grid_msg_from_swsr(&msg, tx)) {
    return;
  }

  for (size_t i = 0; i < msg.length; ++i) {

    if (msg.data[i] != GRID_CONST_STX) {
      continue;
    }

    grid_msg_set_offset(&msg, i);
    uint32_t class = grid_msg_get_parameter(&msg, PARAMETER_CLASSCODE);
    struct grid_decoder_collection* dec_coll = grid_decoder_to_usb_reference;
    grid_port_decode_class(dec_coll, class, msg.data, &msg.data[i]);
  }

  if (grid_platform_usb_serial_ready()) {
    grid_platform_usb_serial_write(msg.data, msg.length);
  }
}

void grid_port_send_ui(struct grid_port* port) {

  assert(port->type == GRID_PORT_UI);

  struct grid_swsr_t* tx = grid_port_get_tx(port);

  struct grid_msg msg;

  if (!grid_msg_from_swsr(&msg, tx)) {
    return;
  }

  for (size_t i = 0; i < msg.length; ++i) {

    if (msg.data[i] != GRID_CONST_STX) {
      continue;
    }

    grid_msg_set_offset(&msg, i);
    uint32_t class = grid_msg_get_parameter(&msg, PARAMETER_CLASSCODE);
    struct grid_decoder_collection* dec_coll = grid_decoder_to_ui_reference;
    grid_port_decode_class(dec_coll, class, msg.data, &msg.data[i]);
  }
}
