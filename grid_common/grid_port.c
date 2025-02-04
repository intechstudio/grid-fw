#include "grid_port.h"
#include "grid_decode.h"

struct grid_transport_model grid_transport_state;

void grid_transport_init(struct grid_transport_model* transport) {

  transport->port_array_length = 0;
  memset(transport->port_array, 0, sizeof(transport->port_array));

  transport->buffer_array_length = 0;
  memset(transport->buffer_tx_array, 0, sizeof(transport->buffer_tx_array));
  memset(transport->buffer_rx_array, 0, sizeof(transport->buffer_rx_array));

  transport->port_array_length = 0;
  memset(transport->doublebuffer_tx_array, 0, sizeof(transport->doublebuffer_tx_array));
  memset(transport->doublebuffer_rx_array, 0, sizeof(transport->doublebuffer_rx_array));
}

void grid_transport_register_port(struct grid_transport_model* transport, struct grid_port* port) {

  port->index = transport->port_array_length;
  port->parent = transport;

  transport->port_array[transport->port_array_length] = port;
  transport->port_array_length++;
}

void grid_transport_register_buffer(struct grid_transport_model* transport, struct grid_buffer* buffer_tx, struct grid_buffer* buffer_rx) {

  buffer_tx->index = transport->buffer_array_length;
  buffer_tx->parent = transport;

  buffer_rx->index = transport->buffer_array_length;
  buffer_rx->parent = transport;

  transport->buffer_tx_array[transport->buffer_array_length] = buffer_tx;
  transport->buffer_rx_array[transport->buffer_array_length] = buffer_rx;
  transport->buffer_array_length++;
}

void grid_transport_register_doublebuffer(struct grid_transport_model* transport, struct grid_doublebuffer* doublebuffer_tx, struct grid_doublebuffer* doublebuffer_rx) {

  if (doublebuffer_tx != NULL) {
    doublebuffer_tx->index = transport->doublebuffer_array_length;
    doublebuffer_tx->parent = transport;
  }

  if (doublebuffer_rx != NULL) {
    doublebuffer_rx->index = transport->doublebuffer_array_length;
    doublebuffer_rx->parent = transport;
  }

  transport->doublebuffer_tx_array[transport->doublebuffer_array_length] = doublebuffer_tx;
  transport->doublebuffer_rx_array[transport->doublebuffer_array_length] = doublebuffer_rx;
  transport->doublebuffer_array_length++;
}

struct grid_port* grid_transport_get_port_first_of_type(struct grid_transport_model* transport, enum grid_port_type type) {

  for (uint8_t i = 0; i < transport->port_array_length; i++) {

    if (transport->port_array[i]->type == type) {

      return transport->port_array[i];
    }
  }

  return NULL;
}

uint8_t grid_transport_get_port_array_length(struct grid_transport_model* transport) { return transport->port_array_length; }

struct grid_port* grid_transport_get_port(struct grid_transport_model* transport, uint8_t index) { return transport->port_array[index]; }
struct grid_buffer* grid_transport_get_buffer_tx(struct grid_transport_model* transport, uint8_t index) { return transport->buffer_tx_array[index]; }
struct grid_buffer* grid_transport_get_buffer_rx(struct grid_transport_model* transport, uint8_t index) { return transport->buffer_rx_array[index]; }
struct grid_doublebuffer* grid_transport_get_doublebuffer_tx(struct grid_transport_model* transport, uint8_t index) { return transport->doublebuffer_tx_array[index]; }
struct grid_doublebuffer* grid_transport_get_doublebuffer_rx(struct grid_transport_model* transport, uint8_t index) { return transport->doublebuffer_rx_array[index]; }

char grid_port_get_name_char(struct grid_port* por) {

  // Print Direction for debugging
  char direction_lookup[4] = {'N', 'E', 'S', 'W'};
  uint8_t direction_index = (por->direction + 7) % 4;

  return direction_lookup[direction_index];
}

int grid_port_should_uart_timeout_disconect_now(struct grid_port* por) {

  if (por->type != GRID_PORT_TYPE_USART) {
    return 0;
  }

  if (grid_platform_rtc_get_elapsed_time(por->partner_last_timestamp) < GRID_PARAMETER_DISCONNECTTIMEOUT_us) {
    // no need to disconnect yet!
    return 0;
  }

  if (por->partner_status == 0) {
    // was already reset, ready to receive
    return 0;
  }

  return 1;
}

static uint8_t grid_port_rxdobulebuffer_check_overrun(struct grid_doublebuffer* doublebuffer_rx) {

  uint8_t overrun_condition_1 = (doublebuffer_rx->seek_start_index == doublebuffer_rx->read_start_index - 1);
  uint8_t overrun_condition_2 = (doublebuffer_rx->seek_start_index == doublebuffer_rx->buffer_size - 1 && doublebuffer_rx->read_start_index == 0);
  uint8_t overrun_condition_3 = (doublebuffer_rx->buffer_storage[(doublebuffer_rx->read_start_index + doublebuffer_rx->buffer_size - 1) % doublebuffer_rx->buffer_size] != 0);

  return (overrun_condition_1 || overrun_condition_2 || overrun_condition_3);
}

int grid_port_rxdobulebuffer_seek_newline(struct grid_port* por, struct grid_doublebuffer* doublebuffer_rx) {

  while (true) {

    if (doublebuffer_rx->buffer_storage[doublebuffer_rx->seek_start_index] == 10) { // \n

      doublebuffer_rx->status = 1;
      return 0;
    } else if (doublebuffer_rx->buffer_storage[doublebuffer_rx->seek_start_index] == 0) {

      return 0;
    }

    // Buffer overrun error 1, 2, 3
    if (grid_port_rxdobulebuffer_check_overrun(doublebuffer_rx)) {
      return 1;
    }

    // Increment seek pointer
    if (doublebuffer_rx->seek_start_index < doublebuffer_rx->buffer_size - 1) {

      doublebuffer_rx->seek_start_index++;
    } else {

      doublebuffer_rx->seek_start_index = 0;
    }
  }
}

void grid_port_rxdobulebuffer_to_linear(struct grid_port* por, struct grid_doublebuffer* doublebuffer_rx, char* message, uint16_t* length) {

  // set double buffer status to 1 if newline is found
  int ret_status = grid_port_rxdobulebuffer_seek_newline(por, doublebuffer_rx);
  if (ret_status != 0) {
    // overrun happened

    grid_port_receiver_hardreset(por, doublebuffer_rx);

    grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_RED, 50);
    grid_alert_all_set_frequency(&grid_led_state, -2);
    grid_alert_all_set_phase(&grid_led_state, 100);
  }

  // No complete message in buffer
  if (doublebuffer_rx->status == 0) {
    return;
  }

  uint16_t len = 0;

  if (doublebuffer_rx->read_start_index < doublebuffer_rx->seek_start_index) {
    len = doublebuffer_rx->seek_start_index - doublebuffer_rx->read_start_index + 1;
  } else {
    len = doublebuffer_rx->buffer_size + doublebuffer_rx->seek_start_index - doublebuffer_rx->read_start_index + 1;
  }

  // Store message in temporary buffer
  for (uint16_t i = 0; i < len; i++) {
    message[i] = doublebuffer_rx->buffer_storage[(doublebuffer_rx->read_start_index + i) % doublebuffer_rx->buffer_size];
    doublebuffer_rx->buffer_storage[(doublebuffer_rx->read_start_index + i) % doublebuffer_rx->buffer_size] = 0;
  }

  // Clear data from rx double buffer
  for (uint16_t i = 0; i < len; i++) {
    doublebuffer_rx->buffer_storage[(doublebuffer_rx->read_start_index + i) % doublebuffer_rx->buffer_size] = 0;
  }

  uint32_t readstartindex = doublebuffer_rx->read_start_index;

  doublebuffer_rx->read_start_index = (doublebuffer_rx->read_start_index + len) % doublebuffer_rx->buffer_size;
  doublebuffer_rx->seek_start_index = doublebuffer_rx->read_start_index;

  doublebuffer_rx->status = 0;

  message[len] = '\0';
  *length = len;
}

uint8_t grid_msg_is_position_transformable(int8_t received_x, int8_t received_y) {
  // Position is transformabe if x and y positions do not indicate global
  // message or editor message

  if (received_x + GRID_PARAMETER_DEFAULT_POSITION == 0 && received_y + GRID_PARAMETER_DEFAULT_POSITION == 0) {
    // EDITOR GENERATED GLOBAL MESSAGE
    return false;
  } else if (received_x + GRID_PARAMETER_DEFAULT_POSITION == 255 && received_y + GRID_PARAMETER_DEFAULT_POSITION == 255) {

    // GRID GENERATED GLOBAL MESSAGE
    return false;
  } else {

    // Normal grid message
    return true;
  }
}

void grid_str_transform_brc_params(char* message, int8_t dx, int8_t dy, uint8_t partner_fi) {

  if (message[1] != GRID_CONST_BRC) {
    return;
  }

  uint8_t error = 0;

  uint8_t received_session = grid_str_get_parameter(message, GRID_BRC_SESSION_offset, GRID_BRC_SESSION_length, &error);
  uint8_t received_msgage = grid_str_get_parameter(message, GRID_BRC_MSGAGE_offset, GRID_BRC_MSGAGE_length, &error);

  // Read the received destination X Y values (SIGNED INT)
  int8_t received_dx = grid_str_get_parameter(message, GRID_BRC_DX_offset, GRID_BRC_DX_length, &error) - GRID_PARAMETER_DEFAULT_POSITION;
  int8_t received_dy = grid_str_get_parameter(message, GRID_BRC_DY_offset, GRID_BRC_DY_length, &error) - GRID_PARAMETER_DEFAULT_POSITION;

  // Read the received source X Y values (SIGNED INT)
  int8_t received_sx = grid_str_get_parameter(message, GRID_BRC_SX_offset, GRID_BRC_SX_length, &error) - GRID_PARAMETER_DEFAULT_POSITION;
  int8_t received_sy = grid_str_get_parameter(message, GRID_BRC_SY_offset, GRID_BRC_SY_length, &error) - GRID_PARAMETER_DEFAULT_POSITION;

  uint8_t received_rot = grid_str_get_parameter(message, GRID_BRC_ROT_offset, GRID_BRC_ROT_length, &error);

  // DO THE DX DY AGE calculations

  int8_t rotated_dx = 0;
  int8_t rotated_dy = 0;

  int8_t rotated_sx = 0;
  int8_t rotated_sy = 0;

  uint8_t updated_rot = (received_rot + partner_fi) % 4;

  // APPLY THE 2D ROTATION MATRIX

  if (partner_fi == 0) { // 0 deg
    rotated_dx += received_dx;
    rotated_dy += received_dy;

    rotated_sx += received_sx;
    rotated_sy += received_sy;
  } else if (partner_fi == 1) { // 90 deg
    rotated_dx -= received_dy;
    rotated_dy += received_dx;

    rotated_sx -= received_sy;
    rotated_sy += received_sx;
  } else if (partner_fi == 2) { // 180 deg
    rotated_dx -= received_dx;
    rotated_dy -= received_dy;

    rotated_sx -= received_sx;
    rotated_sy -= received_sy;
  } else if (partner_fi == 3) { // 270 deg
    rotated_dx += received_dy;
    rotated_dy -= received_dx;

    rotated_sx += received_sy;
    rotated_sy -= received_sx;
  } else {
    // TRAP INVALID MESSAGE
  }

  uint8_t updated_dx = rotated_dx + GRID_PARAMETER_DEFAULT_POSITION + dx;
  uint8_t updated_dy = rotated_dy + GRID_PARAMETER_DEFAULT_POSITION + dy;

  uint8_t updated_sx = rotated_sx + GRID_PARAMETER_DEFAULT_POSITION + dx;
  uint8_t updated_sy = rotated_sy + GRID_PARAMETER_DEFAULT_POSITION + dy;

  uint8_t updated_msgage = received_msgage + 1;

  if (grid_msg_is_position_transformable(received_dx, received_dy)) {

    // Update message with the new values
    grid_str_set_parameter(message, GRID_BRC_DX_offset, GRID_BRC_DX_length, updated_dx, &error);
    grid_str_set_parameter(message, GRID_BRC_DY_offset, GRID_BRC_DY_length, updated_dy, &error);
  }

  if (grid_msg_is_position_transformable(received_sx, received_sy)) {

    // Update message with the new values
    grid_str_set_parameter(message, GRID_BRC_SX_offset, GRID_BRC_SX_length, updated_sx, &error);
    grid_str_set_parameter(message, GRID_BRC_SY_offset, GRID_BRC_SY_length, updated_sy, &error);
  }

  grid_str_set_parameter(message, GRID_BRC_MSGAGE_offset, GRID_BRC_MSGAGE_length, updated_msgage, &error);
  grid_str_set_parameter(message, GRID_BRC_ROT_offset, GRID_BRC_ROT_length, updated_rot, &error);
  grid_str_set_parameter(message, GRID_BRC_PORTROT_offset, GRID_BRC_PORTROT_length, partner_fi, &error);

  // Recalculate and update the checksum
  uint16_t length = strlen(message);
  grid_str_checksum_write(message, length, grid_str_calculate_checksum_of_packet_string(message, length));
}

void grid_port_rxdobulebuffer_receive_to_buffer(struct grid_port* por, struct grid_doublebuffer* doublebuffer_rx, char* buffer, uint16_t length) {

  // Store message in temporary buffer (MAXMSGLEN = 250 character)
  for (uint16_t i = 0; i < length; i++) {
    buffer[i] = doublebuffer_rx->buffer_storage[(doublebuffer_rx->read_start_index + i) % doublebuffer_rx->buffer_size];
    doublebuffer_rx->buffer_storage[(doublebuffer_rx->read_start_index + i) % doublebuffer_rx->buffer_size] = 0;
  }
  buffer[length] = 0;

  // Clear data from rx double buffer
  for (uint16_t i = 0; i < length; i++) {
    doublebuffer_rx->buffer_storage[(doublebuffer_rx->read_start_index + i) % doublebuffer_rx->buffer_size] = 0;
  }

  uint32_t readstartindex = doublebuffer_rx->read_start_index;

  doublebuffer_rx->read_start_index = (doublebuffer_rx->read_start_index + length) % doublebuffer_rx->buffer_size;
  doublebuffer_rx->seek_start_index = doublebuffer_rx->read_start_index;

  doublebuffer_rx->status = 0;
}

int grid_port_receive_broadcast_message(struct grid_port* por, struct grid_buffer* rx_buffer, struct grid_msg_recent_buffer* rec, char* message, uint16_t length) {

  uint32_t fingerprint = grid_msg_recent_fingerprint_calculate(message);

  if (grid_msg_recent_fingerprint_find(rec, fingerprint)) {
    // WE HAVE HEARD THIS MESSAGE BEFORE
    // grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_PURPLE, 20);
    return 1;
  }

  // Check if we can store the message in rx buffer
  if (grid_buffer_write_size(rx_buffer) < length) {
    // Insufficient space in buffer
    return 1;
  }

  grid_buffer_write_from_chunk(rx_buffer, message, length);
  grid_msg_recent_fingerprint_store(rec, fingerprint);

  return 0; // success
}

void grid_port_decode_direct_message(struct grid_port* por, char* message, uint16_t length) {

  if (message[2] == GRID_CONST_BELL) {

    uint8_t error = 0;

    // reset timeout counter
    por->partner_last_timestamp = grid_platform_rtc_get_micros();

    if (por->partner_status == 0) {

      // CONNECT
      por->partner_fi = (message[3] - por->direction + 6) % 4; // 0, 1, 2, 3 base on relative rotation of the modules
      por->partner_status = 1;
    }
  }
}

void grid_port_receive_decode(struct grid_port* por, struct grid_msg_recent_buffer* rec, char* message, uint16_t length) {

  if (0 != grid_str_verify_frame(message)) {
    // message has invalid fram or checksum, cannot be decoded safely
    return;
  }

  if (message[1] == GRID_CONST_BRC) { // Broadcast message

    if (por->partner_status == 0) {
      // dont allow message to reach rx_buffer if we are not connected because partner_phi based transform will be invalid!
      return;
    }

    struct grid_buffer* rx_buffer = grid_transport_get_buffer_rx(por->parent, por->index);
    grid_port_receive_broadcast_message(por, rx_buffer, rec, message, length);

  } else if (message[1] == GRID_CONST_DCT) { // Direct Message

    grid_port_decode_direct_message(por, message, length);
  } else { // Unknown Message Type

    grid_port_debug_printf("Unknown message type\r\n");
  }

  return;
}

//=============================== PROCESS INBOUND
//==============================//

uint8_t grid_port_process_inbound(struct grid_port* por, struct grid_buffer* rx_buffer) {

  uint16_t packet_size = grid_buffer_read_size(rx_buffer);

  if (packet_size == 0) {
    // NO PACKET IN RX BUFFER
    return 0;
  }

  const uint8_t port_count = grid_transport_get_port_array_length(&grid_transport_state);

  struct grid_port* target_port_array[port_count];

  uint8_t target_port_count = 0;

  for (uint8_t i = 0; i < port_count; i++) {

    struct grid_port* next_port = grid_transport_get_port(&grid_transport_state, i);
    struct grid_buffer* next_tx_buffer = grid_transport_get_buffer_tx(&grid_transport_state, i);

    if (next_port->partner_status == 0) {
      continue;
    }

    if (next_port == por && por->type != GRID_PORT_TYPE_UI) { // inbound_loopback only when GRID_PORT_TYPE_UI
      continue;
    }

    if (packet_size > grid_buffer_write_size(next_tx_buffer)) {
      // one of the targetports do not have enough space to store the packet
      grid_platform_printf("Buffer Error: %d | %d | %d \r\n", i, target_port_count, next_port->direction);
      grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_BLUE, 128);
      return 0;
    }

    target_port_array[target_port_count] = next_port;
    target_port_count++;
  }

  // Copy packet from source buffer to temp array

  char buffer[packet_size];
  grid_buffer_read_to_chunk(rx_buffer, buffer, packet_size);

  // Copy packet from temp array to target port buffer
  for (uint8_t i = 0; i < target_port_count; i++) {

    struct grid_port* target_port = target_port_array[i];
    struct grid_buffer* target_tx_buffer = grid_transport_get_buffer_tx(target_port->parent, target_port->index);

    grid_buffer_write_from_chunk(target_tx_buffer, buffer, packet_size);
  }

  return 1;
}

void grid_port_init(struct grid_port* por, uint8_t type, uint8_t dir) {
  // PART 2: INIT

  por->direction = dir;

  por->type = type;

  por->partner_fi = 0;
  por->dx = 0;
  por->dy = 0;

  por->partner_status = 1; // UI AND USB are considered to be connected by default
  por->ping_flag = 0;

  if (type == GRID_PORT_TYPE_USART) {

    por->partner_status = 0;
    por->partner_fi = 0;

    sprintf((char*)por->ping_packet, "%c%c%c%c%02x%02x%02x%c00\n", GRID_CONST_SOH, GRID_CONST_DCT, GRID_CONST_BELL, por->direction, 255, 255, 255, GRID_CONST_EOT);

    por->ping_packet_length = strlen((char*)por->ping_packet);

    grid_str_checksum_write(por->ping_packet, por->ping_packet_length, grid_str_calculate_checksum_of_packet_string(por->ping_packet, por->ping_packet_length));

    if (por->direction == GRID_CONST_NORTH) {
      por->dx = 0;
      por->dy = 1;
    } else if (por->direction == GRID_CONST_EAST) {
      por->dx = 1;
      por->dy = 0;
    } else if (por->direction == GRID_CONST_SOUTH) {
      por->dx = 0;
      por->dy = -1;
    } else if (por->direction == GRID_CONST_WEST) {
      por->dx = -1;
      por->dy = 0;
    }
  }
}

struct grid_port* grid_port_allocate_init(uint8_t type, uint8_t dir) {

  // PART 1: ALLOCATE

  struct grid_port* por = (struct grid_port*)grid_platform_allocate_volatile(sizeof(struct grid_port));
  memset(por, 0, sizeof(struct grid_port));

  grid_port_init(por, type, dir);

  return por;
}

enum grid_doublebuffer_type {
  GRID_DOBULEBUFFER_TYPE_TX = 0,
  GRID_DOBULEBUFFER_TYPE_RX,
};

struct grid_buffer* grid_buffer_allocate_init(size_t length) {

  struct grid_buffer* buffer = (struct grid_buffer*)malloc(sizeof(struct grid_buffer));
  memset(buffer, 0, sizeof(struct grid_buffer));

  buffer->buffer_storage = (char*)malloc(length * sizeof(char));
  memset(buffer->buffer_storage, 0, length * sizeof(char));

  grid_buffer_init(buffer, length);

  return buffer;
}

struct grid_doublebuffer* grid_doublebuffer_allocate_init(size_t length) {

  struct grid_doublebuffer* doublebuffer = (struct grid_doublebuffer*)grid_platform_allocate_volatile(sizeof(struct grid_doublebuffer));
  memset(doublebuffer, 0, sizeof(struct grid_doublebuffer));

  doublebuffer->buffer_storage = (char*)grid_platform_allocate_volatile(length * sizeof(char));
  memset(doublebuffer->buffer_storage, 0, length * sizeof(char));

  doublebuffer->buffer_size = length;

  return doublebuffer;
}

uint8_t grid_port_process_outbound_usart(struct grid_port* por, struct grid_buffer* tx_buffer, struct grid_doublebuffer* tx_doublebuffer) {

  if (tx_doublebuffer->status != 0) {
    // port is busy, a transmission is already in progress!
    return 0;
  }

  uint16_t packet_size = grid_buffer_read_size(tx_buffer);

  if (!packet_size) {

    // NO PACKET IN RX BUFFER
    return 0;
  }

  // Let's transfer the packet to local memory
  grid_buffer_read_init(tx_buffer);

  tx_doublebuffer->status = packet_size;

  for (uint16_t i = 0; i < packet_size; i++) {

    uint8_t character = grid_buffer_read_character(tx_buffer);
    tx_doublebuffer->buffer_storage[i] = character;
  }

  // Let's acknowledge the transaction
  grid_buffer_read_acknowledge(tx_buffer);

  grid_platform_send_grid_message(por->direction, tx_doublebuffer->buffer_storage, packet_size);

  return 1;
}

uint8_t grid_port_process_outbound_usb(struct grid_port* por, struct grid_buffer* tx_buffer, struct grid_doublebuffer* tx_doublebuffer) {

  uint16_t length = grid_buffer_read_size(tx_buffer);

  if (!length) {
    // NO PACKET IN RX BUFFER
    return 0;
  }

  // Clear the tx double buffer
  for (uint16_t i = 0; i < tx_doublebuffer->buffer_size; i++) {
    tx_doublebuffer->buffer_storage[i] = 0;
  }

  grid_buffer_read_to_chunk(tx_buffer, tx_doublebuffer->buffer_storage, length);

  // GRID-2-HOST TRANSLATOR

  uint8_t error = 0;

  for (uint16_t i = 0; i < length; i++) {

    if (tx_doublebuffer->buffer_storage[i] != GRID_CONST_STX) {

      continue;
    }

    // chunk is the specific part of the usb tx doublebuffer that we are
    // currently trying to decode
    char* chunk = &tx_doublebuffer->buffer_storage[i];
    char* header = &tx_doublebuffer->buffer_storage[0];

    uint16_t msg_class = grid_str_get_parameter(chunk, GRID_PARAMETER_CLASSCODE_offset, GRID_PARAMETER_CLASSCODE_length, &error);
    uint8_t msg_instr = grid_str_get_parameter(chunk, GRID_INSTR_offset, GRID_INSTR_length, &error);
    grid_port_decode_class(grid_decoder_to_usb_reference, msg_class, header, chunk);
  }

  // Let's send the packet through USB
  grid_platform_usb_serial_write(tx_doublebuffer->buffer_storage, length);

  return 0;
}

void grid_port_receiver_softreset(struct grid_port* por, struct grid_doublebuffer* rx_doublebuffer) {

  por->partner_status = 0;
  por->partner_last_timestamp = grid_platform_rtc_get_micros();

  grid_platform_reset_grid_transmitter(por->direction);

  if (rx_doublebuffer == NULL) {
    return;
  }

  rx_doublebuffer->seek_start_index = 0;
  rx_doublebuffer->read_start_index = 0;
  rx_doublebuffer->write_index = 0;

  for (uint16_t i = 0; i < rx_doublebuffer->buffer_size; i++) {
    rx_doublebuffer->buffer_storage[i] = 0;
  }
}

void grid_port_receiver_hardreset(struct grid_port* por, struct grid_doublebuffer* rx_doublebuffer) {

  grid_platform_printf("HARD: ");

  grid_platform_disable_grid_transmitter(por->direction);

  por->partner_status = 0;

  grid_str_checksum_write(por->ping_packet, por->ping_packet_length, grid_str_calculate_checksum_of_packet_string(por->ping_packet, por->ping_packet_length));

  grid_port_receiver_softreset(por, rx_doublebuffer);

  grid_platform_enable_grid_transmitter(por->direction);
}

void grid_port_debug_print_text(char* debug_string) {

  struct grid_msg_packet message;

  grid_msg_packet_init(&grid_msg_state, &message, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

  grid_msg_packet_body_append_printf(&message, GRID_CLASS_DEBUGTEXT_frame_start);
  grid_msg_packet_body_append_parameter(&message, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);
  grid_msg_packet_body_append_printf(&message, debug_string);
  grid_msg_packet_body_append_printf(&message, GRID_CLASS_DEBUGTEXT_frame_end);

  grid_msg_packet_close(&grid_msg_state, &message);
  grid_port_packet_send_everywhere(&message);
}

void grid_port_websocket_print_text(char* debug_string) {

  struct grid_msg_packet message;

  grid_msg_packet_init(&grid_msg_state, &message, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

  grid_msg_packet_body_append_printf(&message, GRID_CLASS_WEBSOCKET_frame_start);
  grid_msg_packet_body_append_parameter(&message, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);
  grid_msg_packet_body_append_printf(&message, debug_string);
  grid_msg_packet_body_append_printf(&message, GRID_CLASS_WEBSOCKET_frame_end);

  grid_msg_packet_close(&grid_msg_state, &message);
  grid_port_packet_send_everywhere(&message);
}

void grid_port_package_print_text(char* debug_string) {

  struct grid_msg_packet message;

  grid_msg_packet_init(&grid_msg_state, &message, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

  grid_msg_packet_body_append_printf(&message, GRID_CLASS_PACKAGE_frame_start);
  grid_msg_packet_body_append_parameter(&message, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);
  grid_msg_packet_body_append_printf(&message, debug_string);
  grid_msg_packet_body_append_printf(&message, GRID_CLASS_PACKAGE_frame_end);

  grid_msg_packet_close(&grid_msg_state, &message);
  grid_port_packet_send_everywhere(&message);
}

void grid_port_debug_printf(char const* fmt, ...) {

  va_list ap;

  const int len = 300;
  const char truncation[] = " ... (message truncated)";

  char temp[len];

  va_start(ap, fmt);

  vsnprintf(temp, len - 1 - strlen(truncation), fmt, ap);

  va_end(ap);

  if (strlen(temp) == len - 2 - strlen(truncation)) {
    strcat(temp, truncation);
  }

  grid_port_debug_print_text(temp);

  return;
}

uint8_t grid_port_packet_send_everywhere(struct grid_msg_packet* msg) {

  uint32_t message_length = grid_msg_packet_get_length(msg);

  struct grid_port* ui_port = grid_transport_get_port_first_of_type(&grid_transport_state, GRID_PORT_TYPE_UI);
  struct grid_buffer* ui_rx_buffer = grid_transport_get_buffer_rx(ui_port->parent, ui_port->index);

  if (grid_buffer_write_size(ui_rx_buffer) >= message_length) {

    grid_buffer_write_from_packet(ui_rx_buffer, msg);

    return 1;
  } else {

    return 0;
  }
}

void grid_port_ping_try_everywhere(void) {

  // NEW PING

  for (uint8_t i = 0; i < 4; i++) {

    struct grid_port* next_port = grid_transport_get_port(&grid_transport_state, i);
    struct grid_buffer* next_tx_buffer = grid_transport_get_buffer_tx(next_port->parent, next_port->index);

    if (next_port->ping_flag == 0) {
      // no need to ping yet!
      continue;
    }

    if (grid_buffer_write_size(next_tx_buffer) < next_port->ping_packet_length) {
      // not enough space in buffer!
      continue;
    }

    grid_buffer_write_from_chunk(next_tx_buffer, next_port->ping_packet, next_port->ping_packet_length);

    next_port->ping_flag = 0;
  }
}

void grid_protocol_nvm_erase_succcess_callback(uint8_t lastheader_id) {

  // Generate ACKNOWLEDGE RESPONSE
  struct grid_msg_packet response;

  grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

  // acknowledge
  grid_msg_packet_body_append_printf(&response, GRID_CLASS_NVMERASE_frame);
  grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);
  grid_msg_packet_body_append_parameter(&response, GRID_CLASS_NVMERASE_LASTHEADER_offset, GRID_CLASS_NVMERASE_LASTHEADER_length, lastheader_id);

  // debugtext
  grid_msg_packet_body_append_printf(&response, GRID_CLASS_DEBUGTEXT_frame_start);
  grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);
  grid_msg_packet_body_append_printf(&response, "xxerase complete");
  grid_msg_packet_body_append_printf(&response, GRID_CLASS_DEBUGTEXT_frame_end);

  grid_msg_packet_close(&grid_msg_state, &response);

  grid_port_packet_send_everywhere(&response);

  grid_usb_keyboard_enable(&grid_usb_keyboard_state);

  grid_ui_page_load(&grid_ui_state, grid_ui_page_get_activepage(&grid_ui_state));
}

void grid_protocol_nvm_clear_succcess_callback(uint8_t lastheader_id) {

  struct grid_msg_packet response;

  grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

  // acknowledge
  grid_msg_packet_body_append_printf(&response, GRID_CLASS_PAGECLEAR_frame);
  grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);
  grid_msg_packet_body_append_parameter(&response, GRID_CLASS_PAGECLEAR_LASTHEADER_offset, GRID_CLASS_PAGECLEAR_LASTHEADER_length, lastheader_id);

  // debugtext
  grid_msg_packet_body_append_printf(&response, GRID_CLASS_DEBUGTEXT_frame_start);
  grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);
  grid_msg_packet_body_append_printf(&response, "xxclear complete");
  grid_msg_packet_body_append_printf(&response, GRID_CLASS_DEBUGTEXT_frame_end);

  grid_msg_packet_close(&grid_msg_state, &response);
  grid_port_packet_send_everywhere(&response);

  // phase out the animation
  grid_alert_all_set_timeout_automatic(&grid_led_state);

  // clear template variable after clear command
  grid_ui_page_clear_template_parameters(&grid_ui_state, grid_ui_page_get_activepage(&grid_ui_state));

  grid_ui_page_load(&grid_ui_state, grid_ui_page_get_activepage(&grid_ui_state));
}

void grid_protocol_nvm_read_succcess_callback(uint8_t lastheader_id) {

  // Generate ACKNOWLEDGE RESPONSE
  struct grid_msg_packet response;
  grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);
  grid_msg_packet_body_append_printf(&response, GRID_CLASS_PAGEDISCARD_frame);
  grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);
  grid_msg_packet_body_append_parameter(&response, GRID_CLASS_PAGEDISCARD_LASTHEADER_offset, GRID_CLASS_PAGEDISCARD_LASTHEADER_length, lastheader_id);

  grid_msg_packet_body_append_printf(&response, GRID_CLASS_DEBUGTEXT_frame_start);
  grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);
  grid_msg_packet_body_append_printf(&response, "xxread complete");
  grid_msg_packet_body_append_printf(&response, GRID_CLASS_DEBUGTEXT_frame_end);

  grid_msg_packet_close(&grid_msg_state, &response);
  grid_port_packet_send_everywhere(&response);

  grid_usb_keyboard_enable(&grid_usb_keyboard_state);

  // phase out the animation
  grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_WHITE_DIM, 100);
  grid_alert_all_set_timeout_automatic(&grid_led_state);
}

void grid_protocol_nvm_store_succcess_callback(uint8_t lastheader_id) {

  struct grid_msg_packet response;

  grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

  // acknowledge
  grid_msg_packet_body_append_printf(&response, GRID_CLASS_PAGESTORE_frame);
  grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);
  grid_msg_packet_body_append_parameter(&response, GRID_CLASS_PAGESTORE_LASTHEADER_offset, GRID_CLASS_PAGESTORE_LASTHEADER_length, lastheader_id);

  // debugtext
  grid_msg_packet_body_append_printf(&response, GRID_CLASS_DEBUGTEXT_frame_start);
  grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);
  grid_msg_packet_body_append_printf(&response, "xxstore complete offset 0x%x", grid_plaform_get_nvm_nextwriteoffset());
  grid_msg_packet_body_append_printf(&response, GRID_CLASS_DEBUGTEXT_frame_end);

  grid_msg_packet_close(&grid_msg_state, &response);
  grid_port_packet_send_everywhere(&response);

  // enable keyboard
  grid_usb_keyboard_enable(&grid_usb_keyboard_state);

  // phase out the animation
  grid_alert_all_set_timeout_automatic(&grid_led_state);

  // clear template variable after store command

  grid_ui_page_clear_template_parameters(&grid_ui_state, grid_ui_page_get_activepage(&grid_ui_state));

  // reload configuration

  grid_ui_page_load(&grid_ui_state, grid_ui_page_get_activepage(&grid_ui_state));
}

void grid_protocol_send_heartbeat(uint8_t heartbeat_type, uint32_t hwcfg) {

  uint8_t portstate = 0;

  for (uint8_t i = 0; i < 4; i++) {

    portstate |= ((grid_transport_get_port(&grid_transport_state, i)->partner_status) << i);
  }

  struct grid_msg_packet response;

  grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

  grid_msg_packet_body_append_printf(&response, GRID_CLASS_HEARTBEAT_frame);

  grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);

  grid_msg_packet_body_append_parameter(&response, GRID_CLASS_HEARTBEAT_TYPE_offset, GRID_CLASS_HEARTBEAT_TYPE_length, heartbeat_type);
  grid_msg_packet_body_append_parameter(&response, GRID_CLASS_HEARTBEAT_HWCFG_offset, GRID_CLASS_HEARTBEAT_HWCFG_length, hwcfg);
  grid_msg_packet_body_append_parameter(&response, GRID_CLASS_HEARTBEAT_VMAJOR_offset, GRID_CLASS_HEARTBEAT_VMAJOR_length, GRID_PROTOCOL_VERSION_MAJOR);
  grid_msg_packet_body_append_parameter(&response, GRID_CLASS_HEARTBEAT_VMINOR_offset, GRID_CLASS_HEARTBEAT_VMINOR_length, GRID_PROTOCOL_VERSION_MINOR);
  grid_msg_packet_body_append_parameter(&response, GRID_CLASS_HEARTBEAT_VPATCH_offset, GRID_CLASS_HEARTBEAT_VPATCH_length, GRID_PROTOCOL_VERSION_PATCH);

  grid_msg_packet_body_append_parameter(&response, GRID_CLASS_HEARTBEAT_PORTSTATE_offset, GRID_CLASS_HEARTBEAT_PORTSTATE_length, portstate);

  if (heartbeat_type == 1) { // I am usb connected deevice

    grid_msg_packet_body_append_printf(&response, GRID_CLASS_PAGEACTIVE_frame);
    grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);
    grid_msg_packet_body_append_parameter(&response, GRID_CLASS_PAGEACTIVE_PAGENUMBER_offset, GRID_CLASS_PAGEACTIVE_PAGENUMBER_length, grid_ui_state.page_activepage);

    // printf("DEBUG: %s\r\n", response.body);
  }

  grid_msg_packet_close(&grid_msg_state, &response);

  grid_port_packet_send_everywhere(&response);
}

//=============================== PROCESS OUTBOUND
//==============================//

// CC=29 LoC=71
void grid_port_process_outbound_ui(struct grid_port* por, struct grid_buffer* tx_buffer) {

  uint16_t length = grid_buffer_read_size(tx_buffer);

  if (length == 0) {

    // NO PACKET IN RX BUFFER
    return;
  }

  char message[GRID_PARAMETER_PACKET_maxlength] = {0};

  // Let's transfer the packet to local memory
  grid_buffer_read_to_chunk(tx_buffer, message, length);

  // GRID-2-UI TRANSLATOR

  uint8_t error = 0;

  uint8_t id = grid_str_get_parameter(message, GRID_BRC_ID_offset, GRID_BRC_ID_length, &error);

  uint8_t dx = grid_str_get_parameter(message, GRID_BRC_DX_offset, GRID_BRC_DX_length, &error);
  uint8_t dy = grid_str_get_parameter(message, GRID_BRC_DY_offset, GRID_BRC_DY_length, &error);

  uint8_t sx = grid_str_get_parameter(message, GRID_BRC_SX_offset, GRID_BRC_SX_length, &error);
  uint8_t sy = grid_str_get_parameter(message, GRID_BRC_SY_offset, GRID_BRC_SY_length, &error);

  uint8_t rot = grid_str_get_parameter(message, GRID_BRC_ROT_offset, GRID_BRC_ROT_length, &error);
  uint8_t portrot = grid_str_get_parameter(message, GRID_BRC_PORTROT_offset, GRID_BRC_PORTROT_length, &error);

  uint8_t position_is_me = 0;
  uint8_t position_is_global = 0;
  uint8_t position_is_local = 0;

  if (dx == GRID_PARAMETER_DEFAULT_POSITION && dy == GRID_PARAMETER_DEFAULT_POSITION) {
    position_is_me = 1;
  } else if (dx == GRID_PARAMETER_GLOBAL_POSITION && dy == GRID_PARAMETER_GLOBAL_POSITION) {
    position_is_global = 1;
  } else if (dx == GRID_PARAMETER_LOCAL_POSITION && dy == GRID_PARAMETER_LOCAL_POSITION) {
    position_is_local = 1;
  }

  for (uint16_t i = 0; i < length; i++) {

    if (message[i] != GRID_CONST_STX) {
      continue;
    }

    char* header = &message[0];
    char* chunk = &message[i];

    uint16_t msg_class = grid_str_get_parameter(chunk, GRID_PARAMETER_CLASSCODE_offset, GRID_PARAMETER_CLASSCODE_length, &error);

    grid_port_decode_class(grid_decoder_to_ui_reference, msg_class, header, chunk);
  }
}
