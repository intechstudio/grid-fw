#include "grid_port.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

// #include "grid_decode.h"
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

  uint8_t chk = grid_str_calculate_checksum_of_packet_string(data, ping->size);
  grid_str_checksum_write(data, ping->size, chk);
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

  int ret = grid_swsr_cspn(tx, '\n');

  if (ret < 0) {
    return;
  }

  grid_platform_send_frame(tx, ret + 1, port->dir);
}

void grid_port_send_usb(struct grid_port* port) {

  assert(port->type == GRID_PORT_USB);

  struct grid_swsr_t* tx = grid_port_get_tx(port);

  int ret = grid_swsr_cspn(tx, '\n');

  if (ret < 0) {
    return;
  }

  assert(ret < GRID_PARAMETER_SPI_TRANSACTION_length);

  // Allocated statically due to the implementation of usb serial writes
  // sometimes requiring their buffer to live until the transfer completes
  static char temp[GRID_PARAMETER_SPI_TRANSACTION_length + 1];

  assert(grid_swsr_readable(tx, ret + 1));
  grid_swsr_read(tx, temp, ret + 1);
  temp[ret + 1] = '\0';

  for (size_t i = 0; i < ret + 1; ++i) {

    if (temp[i] != GRID_CONST_STX) {
      continue;
    }

    uint8_t error = 0;

    uint16_t offset = GRID_PARAMETER_CLASSCODE_offset;
    uint8_t length = GRID_PARAMETER_CLASSCODE_length;
    uint32_t class = grid_str_get_parameter(&temp[i], offset, length, &error);

    struct grid_decoder_collection* dec_coll = grid_decoder_to_usb_reference;
    grid_port_decode_class(dec_coll, class, temp, &temp[i]);
  }

  if (grid_platform_usb_serial_ready()) {

    grid_platform_usb_serial_write(temp, ret + 1);
  }
}

void grid_port_send_ui(struct grid_port* port) {

  assert(port->type == GRID_PORT_UI);

  struct grid_swsr_t* tx = grid_port_get_tx(port);

  int ret = grid_swsr_cspn(tx, '\n');

  if (ret < 0) {
    return;
  }

  assert(ret < GRID_PARAMETER_SPI_TRANSACTION_length);
  char temp[GRID_PARAMETER_SPI_TRANSACTION_length + 1];

  assert(grid_swsr_readable(tx, ret + 1));
  grid_swsr_read(tx, temp, ret + 1);
  temp[ret + 1] = '\0';

  for (size_t i = 0; i < ret + 1; ++i) {

    if (temp[i] != GRID_CONST_STX) {
      continue;
    }

    uint8_t error = 0;

    uint16_t offset = GRID_PARAMETER_CLASSCODE_offset;
    uint8_t length = GRID_PARAMETER_CLASSCODE_length;
    uint32_t class = grid_str_get_parameter(&temp[i], offset, length, &error);

    struct grid_decoder_collection* dec_coll = grid_decoder_to_ui_reference;
    grid_port_decode_class(dec_coll, class, temp, &temp[i]);
  }
}

bool grid_msg_pos_transformable(int8_t recv_x, int8_t recv_y) {

  int8_t x = recv_x + GRID_PARAMETER_DEFAULT_POSITION;
  int8_t y = recv_y + GRID_PARAMETER_DEFAULT_POSITION;

  // Editor-generated global message
  if (x == 0 && y == 0) {
    return false;
  }

  // Grid-generated global message
  if (x == -1 && y == -1) {
    return false;
  }

  return true;
}

void grid_str_transform_brc_params(char* msg, int8_t dx, int8_t dy, uint8_t partner_rot) {

  assert(partner_rot < GRID_PORT_DIR_COUNT);

  if (msg[1] != GRID_CONST_BRC) {
    return;
  }

  uint8_t error = 0;

  // uint8_t recv_session = grid_str_get_parameter(msg, GRID_BRC_SESSION_offset, GRID_BRC_SESSION_length, &error);
  uint8_t recv_age = grid_str_get_parameter(msg, GRID_BRC_MSGAGE_offset, GRID_BRC_MSGAGE_length, &error);
  uint8_t new_age = recv_age + 1;

  int8_t recv_dx = grid_str_get_parameter(msg, GRID_BRC_DX_offset, GRID_BRC_DX_length, &error) - GRID_PARAMETER_DEFAULT_POSITION;
  int8_t recv_dy = grid_str_get_parameter(msg, GRID_BRC_DY_offset, GRID_BRC_DY_length, &error) - GRID_PARAMETER_DEFAULT_POSITION;

  int8_t recv_sx = grid_str_get_parameter(msg, GRID_BRC_SX_offset, GRID_BRC_SX_length, &error) - GRID_PARAMETER_DEFAULT_POSITION;
  int8_t recv_sy = grid_str_get_parameter(msg, GRID_BRC_SY_offset, GRID_BRC_SY_length, &error) - GRID_PARAMETER_DEFAULT_POSITION;

  uint8_t recv_rot = grid_str_get_parameter(msg, GRID_BRC_ROT_offset, GRID_BRC_ROT_length, &error);
  uint8_t new_rot = (recv_rot + partner_rot) % 4;

  int8_t sign_x[4] = {1, -1, -1, 1};
  int8_t sign_y[4] = {1, 1, -1, -1};

  uint8_t cross = partner_rot % 2;
  int8_t rot_dx = sign_x[partner_rot] * (recv_dx * !cross + recv_dy * cross);
  int8_t rot_dy = sign_y[partner_rot] * (recv_dy * !cross + recv_dx * cross);
  int8_t rot_sx = sign_x[partner_rot] * (recv_sx * !cross + recv_sy * cross);
  int8_t rot_sy = sign_y[partner_rot] * (recv_sy * !cross + recv_sx * cross);

  uint8_t new_dx = rot_dx + GRID_PARAMETER_DEFAULT_POSITION + dx;
  uint8_t new_dy = rot_dy + GRID_PARAMETER_DEFAULT_POSITION + dy;
  uint8_t new_sx = rot_sx + GRID_PARAMETER_DEFAULT_POSITION + dx;
  uint8_t new_sy = rot_sy + GRID_PARAMETER_DEFAULT_POSITION + dy;

  if (grid_msg_pos_transformable(recv_dx, recv_dy)) {

    grid_str_set_parameter(msg, GRID_BRC_DX_offset, GRID_BRC_DX_length, new_dx, &error);
    grid_str_set_parameter(msg, GRID_BRC_DY_offset, GRID_BRC_DY_length, new_dy, &error);
  }

  if (grid_msg_pos_transformable(recv_sx, recv_sy)) {

    grid_str_set_parameter(msg, GRID_BRC_SX_offset, GRID_BRC_SX_length, new_sx, &error);
    grid_str_set_parameter(msg, GRID_BRC_SY_offset, GRID_BRC_SY_length, new_sy, &error);
  }

  grid_str_set_parameter(msg, GRID_BRC_MSGAGE_offset, GRID_BRC_MSGAGE_length, new_age, &error);
  grid_str_set_parameter(msg, GRID_BRC_ROT_offset, GRID_BRC_ROT_length, new_rot, &error);
  grid_str_set_parameter(msg, GRID_BRC_PORTROT_offset, GRID_BRC_PORTROT_length, partner_rot, &error);

  // Recalculate and update checksum
  uint16_t length = strlen(msg);
  uint8_t chk = grid_str_calculate_checksum_of_packet_string(msg, length);
  grid_str_checksum_write(msg, length, chk);
}
