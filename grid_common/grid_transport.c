#include "grid_transport.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

struct grid_transport grid_transport_state;

void grid_transport_malloc(struct grid_transport* transport, size_t port_count) {

  assert(port_count > 0);

  transport->port_count = port_count;

  transport->ports = malloc(transport->port_count * sizeof(struct grid_port));

  for (size_t i = 0; i < transport->port_count; ++i) {
    grid_port_malloc(&transport->ports[i], GRID_PORT_SWSR_SIZE, GRID_PORT_SWSR_SIZE * 2);
  }

  transport->usart_send_offset = 0;
}

void grid_transport_free(struct grid_transport* transport) {

  assert(transport->ports);

  for (size_t i = 0; i < transport->port_count; ++i) {
    grid_port_free(&transport->ports[i]);
  }

  free(transport->ports);
}

struct grid_port* grid_transport_get_port(struct grid_transport* transport, size_t idx, enum grid_port_type type, enum grid_port_dir dir) {

  assert(idx < transport->port_count);

  struct grid_port* port = &transport->ports[idx];

  assert(port->type == type);
  assert(port->dir == dir);

  return port;
}

uint8_t trailing_zeroes(uint8_t x) {

  uint8_t res = 0;

  while (x > 0 && !(x & 1)) {
    ++res;
    x >>= 1;
  }

  return res;
}

void grid_transport_recv_usart(struct grid_transport* transport, uint8_t* msg, size_t size) {

  assert(size <= GRID_PARAMETER_SPI_TRANSACTION_length);

  // The number of trailing zeroes in the source flag
  // indexes the destination UART port of the message
  uint8_t src_flags = msg[GRID_PARAMETER_SPI_SOURCE_FLAGS_index];
  uint8_t tz = trailing_zeroes(src_flags);

  // If the trailing zeroes indicate an invalid port, return early
  if (tz >= 4) {
    return;
  }

  struct grid_port* port = grid_transport_get_port(transport, tz, GRID_PORT_USART, tz);

  grid_port_recv_msg(port, msg, size);
}

void grid_transport_recv_usb(struct grid_transport* transport, uint8_t* msg, size_t size) {

  struct grid_port* port = grid_transport_get_port(transport, 5, GRID_PORT_USB, 0);

  grid_str_transform_brc_params((char*)msg, port->dx, port->dy, port->partner.rot);

  struct grid_swsr_t* rx = grid_port_get_rx(port);

  if (!grid_swsr_writable(rx, size)) {
    return;
  }

  grid_swsr_write(rx, msg, size);
}

void grid_transport_ping_all(struct grid_transport* transport) {

  for (uint8_t i = 0; i < 4; ++i) {

    struct grid_port* port = grid_transport_get_port(transport, i, GRID_PORT_USART, i);

    struct grid_swsr_t* swsr = grid_port_get_tx(port);

    if (!grid_swsr_writable(swsr, port->ping.size)) {
      continue;
    }

    grid_swsr_write(swsr, port->ping.data, port->ping.size);
  }
}

void grid_transport_send_usart_cyclic_offset(struct grid_transport* transport) {

  for (uint8_t i = 0; i < 4; ++i) {

    uint8_t j = (transport->usart_send_offset + i) % 4;

    struct grid_port* port = grid_transport_get_port(transport, j, GRID_PORT_USART, j);

    grid_port_send_usart(port);
  }

  transport->usart_send_offset = (transport->usart_send_offset + 1) % 4;
}

void grid_msg_packet_to_swsr(struct grid_msg_packet* pkt, struct grid_swsr_t* swsr) {

  grid_swsr_write(swsr, pkt->header, pkt->header_length);
  grid_swsr_write(swsr, pkt->body, pkt->body_length);
  grid_swsr_write(swsr, pkt->footer, pkt->footer_length);
}

void grid_transport_send_msg_packet_to_all(struct grid_transport* transport, struct grid_msg_packet* pkt) {

  struct grid_port* port = grid_transport_get_port(transport, 4, GRID_PORT_UI, 0);

  uint32_t length = grid_msg_packet_get_length(pkt);

  struct grid_swsr_t* rx = grid_port_get_rx(port);

  if (!grid_swsr_writable(rx, length)) {
    return;
  }

  grid_msg_packet_to_swsr(pkt, rx);
}

void grid_transport_heartbeat(struct grid_transport* transport, uint8_t type, uint32_t hwcfg, uint8_t activepage) {

  // Port state bitfield
  uint8_t portstate = 0;
  for (uint8_t i = 0; i < 4; ++i) {

    struct grid_port* port = grid_transport_get_port(transport, i, GRID_PORT_USART, i);

    portstate |= grid_port_connected(port) << i;
  }

  // Heartbeat packet
  struct grid_msg_packet pkt;

  grid_msg_packet_init(&grid_msg_state, &pkt, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);
  grid_msg_packet_body_append_printf(&pkt, GRID_CLASS_HEARTBEAT_frame);
  grid_msg_packet_body_append_parameter(&pkt, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);
  grid_msg_packet_body_append_parameter(&pkt, GRID_CLASS_HEARTBEAT_TYPE_offset, GRID_CLASS_HEARTBEAT_TYPE_length, type);
  grid_msg_packet_body_append_parameter(&pkt, GRID_CLASS_HEARTBEAT_HWCFG_offset, GRID_CLASS_HEARTBEAT_HWCFG_length, hwcfg);
  grid_msg_packet_body_append_parameter(&pkt, GRID_CLASS_HEARTBEAT_VMAJOR_offset, GRID_CLASS_HEARTBEAT_VMAJOR_length, GRID_PROTOCOL_VERSION_MAJOR);
  grid_msg_packet_body_append_parameter(&pkt, GRID_CLASS_HEARTBEAT_VMINOR_offset, GRID_CLASS_HEARTBEAT_VMINOR_length, GRID_PROTOCOL_VERSION_MINOR);
  grid_msg_packet_body_append_parameter(&pkt, GRID_CLASS_HEARTBEAT_VPATCH_offset, GRID_CLASS_HEARTBEAT_VPATCH_length, GRID_PROTOCOL_VERSION_PATCH);
  grid_msg_packet_body_append_parameter(&pkt, GRID_CLASS_HEARTBEAT_PORTSTATE_offset, GRID_CLASS_HEARTBEAT_PORTSTATE_length, portstate);

  if (type == 1) {

    grid_msg_packet_body_append_printf(&pkt, GRID_CLASS_PAGEACTIVE_frame);
    grid_msg_packet_body_append_parameter(&pkt, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);
    grid_msg_packet_body_append_parameter(&pkt, GRID_CLASS_PAGEACTIVE_PAGENUMBER_offset, GRID_CLASS_PAGEACTIVE_PAGENUMBER_length, activepage);
  }

  grid_msg_packet_close(&grid_msg_state, &pkt);

  grid_transport_send_msg_packet_to_all(transport, &pkt);
}

void grid_transport_rx_broadcast_tx(struct grid_transport* transport, struct grid_port* port, grid_brc_between_t between) {

  if (!grid_port_connected(port)) {
    return;
  }

  struct grid_swsr_t* rx = grid_port_get_rx(port);

  int ret = grid_swsr_until_msg_end(rx);

  if (ret < 0) {
    return;
  }

  uint32_t rx_size = ret + 1;

  assert(grid_swsr_readable(rx, rx_size));

  for (size_t i = 0; i < transport->port_count; ++i) {

    enum grid_port_type type = transport->ports[i].type;
    enum grid_port_dir dir = transport->ports[i].dir;
    struct grid_port* next = grid_transport_get_port(transport, i, type, dir);

    if (!grid_port_connected(next)) {
      continue;
    }

    if (between && !between(port->type, type)) {
      continue;
    }

    // Loopback only for UI
    if (next == port && port->type != GRID_PORT_UI) {
      continue;
    }

    struct grid_swsr_t* tx = grid_port_get_tx(next);

    if (!grid_swsr_writable(tx, rx_size)) {

      if (type == GRID_PORT_USART) {
        grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_BLUE, 128);
      }

      continue;
    }

    grid_swsr_copy(rx, tx, rx_size);
  }

  grid_swsr_read(rx, NULL, rx_size);
}

void grid_port_debug_print_text(char* str) {

  struct grid_msg_packet pkt;

  grid_msg_packet_init(&grid_msg_state, &pkt, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

  grid_msg_packet_body_append_printf(&pkt, GRID_CLASS_DEBUGTEXT_frame_start);
  grid_msg_packet_body_append_parameter(&pkt, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);

  if (grid_msg_packet_body_append_nprintf(&pkt, "%s", str) <= 0) {
    return;
  }

  if (grid_msg_packet_body_append_nprintf(&pkt, GRID_CLASS_DEBUGTEXT_frame_end) <= 0) {
    return;
  }

  grid_msg_packet_close(&grid_msg_state, &pkt);
  grid_transport_send_msg_packet_to_all(&grid_transport_state, &pkt);
}

enum { PORT_PRINTF_MAX = 300 };

#define PORT_PRINTF_TRUNCATION " ... (message truncated)"

void grid_port_debug_printf(const char* fmt, ...) {

  char temp[PORT_PRINTF_MAX];

  va_list va;

  static int truncation_len = sizeof(PORT_PRINTF_TRUNCATION) - 1;

  va_start(va, fmt);
  int ret = vsnprintf(temp, PORT_PRINTF_MAX - truncation_len, fmt, va);
  va_end(va);

  if (ret >= PORT_PRINTF_MAX - truncation_len) {
    strcat(temp, PORT_PRINTF_TRUNCATION);
  }

  grid_port_debug_print_text(temp);
}

void grid_port_websocket_print_text(char* str) {

  struct grid_msg_packet message;

  grid_msg_packet_init(&grid_msg_state, &message, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

  grid_msg_packet_body_append_printf(&message, GRID_CLASS_WEBSOCKET_frame_start);
  grid_msg_packet_body_append_parameter(&message, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);
  grid_msg_packet_body_append_printf(&message, str);
  grid_msg_packet_body_append_printf(&message, GRID_CLASS_WEBSOCKET_frame_end);

  grid_msg_packet_close(&grid_msg_state, &message);
  grid_transport_send_msg_packet_to_all(&grid_transport_state, &message);
}

void grid_port_package_print_text(char* str) {

  struct grid_msg_packet message;

  grid_msg_packet_init(&grid_msg_state, &message, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

  grid_msg_packet_body_append_printf(&message, GRID_CLASS_PACKAGE_frame_start);
  grid_msg_packet_body_append_parameter(&message, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);
  grid_msg_packet_body_append_printf(&message, str);
  grid_msg_packet_body_append_printf(&message, GRID_CLASS_PACKAGE_frame_end);

  grid_msg_packet_close(&grid_msg_state, &message);
  grid_transport_send_msg_packet_to_all(&grid_transport_state, &message);
}
