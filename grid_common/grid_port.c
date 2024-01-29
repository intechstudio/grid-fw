#include "grid_port.h"

struct grid_transport_model grid_transport_state;

void grid_transport_init(struct grid_transport_model* transport) {
  transport->port_array_length = 0;
  memset(transport->port_array, 0, sizeof(transport->port_array));
}

void grid_transport_register_port(struct grid_transport_model* transport, struct grid_port* port) {

  transport->port_array[transport->port_array_length] = port;
  transport->port_array_length++;
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

char grid_port_get_name_char(struct grid_port* por) {

  // Print Direction for debugging
  char direction_lookup[4] = {'N', 'E', 'S', 'W'};
  uint8_t direction_index = (por->direction + 7) % 4;

  return direction_lookup[direction_index];
}

static void grid_port_timeout_try_disconect(struct grid_port* por) {

  if (grid_platform_rtc_get_elapsed_time(por->partner_last_timestamp) < 1000 * 1000) {
    // no need to disconnect yet!
    return;
  }

  if (por->partner_status == 0 && por->rx_double_buffer_read_start_index == 0 && por->rx_double_buffer_seek_start_index == 0) {
    // was already reset, ready to receive
    return;
  }

  por->partner_status = 0;
  grid_port_receiver_softreset(por);
}

static uint8_t grid_port_rxdobulebuffer_check_overrun(struct grid_port* por) {

  uint8_t overrun_condition_1 = (por->rx_double_buffer_seek_start_index == por->rx_double_buffer_read_start_index - 1);
  uint8_t overrun_condition_2 = (por->rx_double_buffer_seek_start_index == GRID_DOUBLE_BUFFER_RX_SIZE - 1 && por->rx_double_buffer_read_start_index == 0);
  uint8_t overrun_condition_3 = (por->rx_double_buffer[(por->rx_double_buffer_read_start_index + GRID_DOUBLE_BUFFER_RX_SIZE - 1) % GRID_DOUBLE_BUFFER_RX_SIZE] != 0);

  return (overrun_condition_1 || overrun_condition_2 || overrun_condition_3);
}

static void grid_port_rxdobulebuffer_seek_newline(struct grid_port* por) {

  for (uint16_t i = 0; i < 490; i++) { // 490 is the max processing length

    if (por->rx_double_buffer[por->rx_double_buffer_seek_start_index] == 10) { // \n

      por->rx_double_buffer_status = 1;

      break;
    } else if (por->rx_double_buffer[por->rx_double_buffer_seek_start_index] == 0) {

      break;
    }

    // Buffer overrun error 1, 2, 3
    if (grid_port_rxdobulebuffer_check_overrun(por)) {

      grid_platform_printf("Overrun%d\r\n", por->direction);
      grid_platform_printf("R%d S%d W%d\r\n", por->rx_double_buffer_read_start_index, por->rx_double_buffer_seek_start_index, por->rx_double_buffer_write_index);

      grid_port_receiver_hardreset(por);

      grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_RED, 50);
      grid_alert_all_set_frequency(&grid_led_state, -2);
      grid_alert_all_set_phase(&grid_led_state, 100);
      return;
    }

    // Increment seek pointer
    if (por->rx_double_buffer_seek_start_index < GRID_DOUBLE_BUFFER_RX_SIZE - 1) {

      por->rx_double_buffer_seek_start_index++;
    } else {

      por->rx_double_buffer_seek_start_index = 0;
    }
  }
}

void grid_port_receive_task(struct grid_port* por) {

  ///////////////////// PART 1 Old receive task

  if (por->rx_double_buffer_status == 0) {

    if (por->type == GRID_PORT_TYPE_USART) { // This is GRID usart port

      grid_port_timeout_try_disconect(por);
    }

    grid_port_rxdobulebuffer_seek_newline(por);
  }

  ////////////////// PART 2

  // No complete message in buffer
  if (por->rx_double_buffer_status == 0) {
    return;
  }

  uint32_t length = 0;

  if (por->rx_double_buffer_read_start_index < por->rx_double_buffer_seek_start_index) {
    length = por->rx_double_buffer_seek_start_index - por->rx_double_buffer_read_start_index + 1;
  } else {
    length = GRID_DOUBLE_BUFFER_RX_SIZE + por->rx_double_buffer_seek_start_index - por->rx_double_buffer_read_start_index + 1;
  }

  grid_port_receive_decode(por, length);

  por->rx_double_buffer_status = 0;
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

void grid_msg_string_transform_brc_params(char* message, int8_t dx, int8_t dy, uint8_t partner_fi) {

  uint8_t error = 0;

  uint8_t received_session = grid_msg_string_get_parameter(message, GRID_BRC_SESSION_offset, GRID_BRC_SESSION_length, &error);
  uint8_t received_msgage = grid_msg_string_get_parameter(message, GRID_BRC_MSGAGE_offset, GRID_BRC_MSGAGE_length, &error);

  // Read the received destination X Y values (SIGNED INT)
  int8_t received_dx = grid_msg_string_get_parameter(message, GRID_BRC_DX_offset, GRID_BRC_DX_length, &error) - GRID_PARAMETER_DEFAULT_POSITION;
  int8_t received_dy = grid_msg_string_get_parameter(message, GRID_BRC_DY_offset, GRID_BRC_DY_length, &error) - GRID_PARAMETER_DEFAULT_POSITION;

  // Read the received source X Y values (SIGNED INT)
  int8_t received_sx = grid_msg_string_get_parameter(message, GRID_BRC_SX_offset, GRID_BRC_SX_length, &error) - GRID_PARAMETER_DEFAULT_POSITION;
  int8_t received_sy = grid_msg_string_get_parameter(message, GRID_BRC_SY_offset, GRID_BRC_SY_length, &error) - GRID_PARAMETER_DEFAULT_POSITION;

  uint8_t received_rot = grid_msg_string_get_parameter(message, GRID_BRC_ROT_offset, GRID_BRC_ROT_length, &error);

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
    grid_msg_string_set_parameter(message, GRID_BRC_DX_offset, GRID_BRC_DX_length, updated_dx, &error);
    grid_msg_string_set_parameter(message, GRID_BRC_DY_offset, GRID_BRC_DY_length, updated_dy, &error);
  }

  if (grid_msg_is_position_transformable(received_sx, received_sy)) {

    // Update message with the new values
    grid_msg_string_set_parameter(message, GRID_BRC_SX_offset, GRID_BRC_SX_length, updated_sx, &error);
    grid_msg_string_set_parameter(message, GRID_BRC_SY_offset, GRID_BRC_SY_length, updated_sy, &error);
  }

  grid_msg_string_set_parameter(message, GRID_BRC_MSGAGE_offset, GRID_BRC_MSGAGE_length, updated_msgage, &error);
  grid_msg_string_set_parameter(message, GRID_BRC_ROT_offset, GRID_BRC_ROT_length, updated_rot, &error);
  grid_msg_string_set_parameter(message, GRID_BRC_PORTROT_offset, GRID_BRC_PORTROT_length, partner_fi, &error);

  // Recalculate and update the checksum
  uint16_t length = strlen(message);
  grid_msg_string_checksum_write(message, length, grid_msg_string_calculate_checksum_of_packet_string(message, length));
}

uint32_t grid_msg_recent_fingerprint_calculate(char* message) {

  uint8_t error = 0;

  uint8_t received_id = grid_msg_string_get_parameter(message, GRID_BRC_ID_offset, GRID_BRC_ID_length, &error);
  uint8_t received_session = grid_msg_string_get_parameter(message, GRID_BRC_SESSION_offset, GRID_BRC_SESSION_length, &error);
  int8_t updated_sx = grid_msg_string_get_parameter(message, GRID_BRC_SX_offset, GRID_BRC_SX_length, &error) - GRID_PARAMETER_DEFAULT_POSITION;
  int8_t updated_sy = grid_msg_string_get_parameter(message, GRID_BRC_SY_offset, GRID_BRC_SY_length, &error) - GRID_PARAMETER_DEFAULT_POSITION;

  uint32_t fingerprint = received_id * 256 * 256 * 256 + updated_sx * 256 * 256 + updated_sy * 256 + received_session;

  return fingerprint;
}

static void grid_port_rxdobulebuffer_receive_to_buffer(struct grid_port* por, char* buffer, uint16_t length) {

  // Store message in temporary buffer (MAXMSGLEN = 250 character)
  for (uint16_t i = 0; i < length; i++) {
    buffer[i] = por->rx_double_buffer[(por->rx_double_buffer_read_start_index + i) % GRID_DOUBLE_BUFFER_RX_SIZE];
    por->rx_double_buffer[(por->rx_double_buffer_read_start_index + i) % GRID_DOUBLE_BUFFER_RX_SIZE] = 0;
  }
  buffer[length] = 0;

  // Clear data from rx double buffer
  for (uint16_t i = 0; i < length; i++) {
    por->rx_double_buffer[(por->rx_double_buffer_read_start_index + i) % GRID_DOUBLE_BUFFER_RX_SIZE] = 0;
  }

  uint32_t readstartindex = por->rx_double_buffer_read_start_index;

  por->rx_double_buffer_read_start_index = (por->rx_double_buffer_read_start_index + length) % GRID_DOUBLE_BUFFER_RX_SIZE;
  por->rx_double_buffer_seek_start_index = por->rx_double_buffer_read_start_index;

  por->rx_double_buffer_status = 0;
}

void grid_port_receive_broadcast_message(struct grid_port* por, char* message, uint16_t length) {

  uint8_t error = 0;

  // update age, sx, sy, dx, dy, rot etc...
  grid_msg_string_transform_brc_params(message, por->dx, por->dy, por->partner_fi);

  uint32_t fingerprint = grid_msg_recent_fingerprint_calculate(message);

  if (grid_msg_recent_fingerprint_find(&grid_msg_state, fingerprint)) {
    // WE HAVE NOT HEARD THIS MESSAGE BEFORE
    // grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_PURPLE, 20);
    return;
  }

  // Check if we can store the message in rx buffer
  if (grid_buffer_write_size(&por->rx_buffer) >= length) {

    grid_buffer_write_from_chunk(&por->rx_buffer, message, length);

    grid_msg_recent_fingerprint_store(&grid_msg_state, fingerprint);
  }
}

void grid_port_receive_direct_message(struct grid_port* por, char* message, uint16_t length) {

  if (message[2] == GRID_CONST_BELL) {

    uint8_t error = 0;

    // reset timeout counter
    por->partner_last_timestamp = grid_platform_rtc_get_micros();

    if (por->partner_status == 0) {

      // CONNECT
      por->partner_fi = (message[3] - por->direction + 6) % 4;
      por->partner_hwcfg = grid_msg_string_read_hex_string_value(&message[length - 10], 2, &error);
      por->partner_status = 1;
    }
  }
}

void grid_port_receive_decode(struct grid_port* por, uint16_t len) {

  uint16_t length = len;
  char message[length + 1];

  // Copy data from cyrcular buffer to temporary linear array;
  grid_port_rxdobulebuffer_receive_to_buffer(por, message, length);

  // close the message string with terminating zero character
  message[length] = '\0';

  // frame validator
  if (message[0] != GRID_CONST_SOH || message[length - 1] != GRID_CONST_LF) {

    grid_port_debug_printf("Frame Error %d ", length);
    return;
  }

  // checksum validator
  uint8_t checksum_received = grid_msg_string_checksum_read(message, length);
  uint8_t checksum_calculated = grid_msg_string_calculate_checksum_of_packet_string(message, length);

  if (checksum_calculated != checksum_received) {

    // INVALID CHECKSUM
    uint8_t error = 0;

    uint16_t packet_length = grid_msg_string_get_parameter(message, GRID_BRC_LEN_offset, GRID_BRC_LEN_length, &error);
    grid_platform_printf("##CHK %d %d\r\n", packet_length, length);
    grid_port_debug_printf("Checksum %02x %02x", checksum_calculated, checksum_received);
    return;
  }

  if (message[1] == GRID_CONST_BRC) { // Broadcast message

    grid_port_receive_broadcast_message(por, message, length);
  } else if (message[1] == GRID_CONST_DCT) { // Direct Message

    grid_port_receive_direct_message(por, message, length);
  } else { // Unknown Message Type

    grid_port_debug_printf("Unknown message type\r\n");
  }

  return;
}

//=============================== PROCESS INBOUND
//==============================//

uint8_t grid_port_process_inbound(struct grid_port* por) {

  uint8_t loopback = por->inbound_loopback;

  uint16_t packet_size = grid_buffer_read_size(&por->rx_buffer);

  if (packet_size == 0) {
    // NO PACKET IN RX BUFFER
    return 0;
  }

  const uint8_t port_count = grid_transport_get_port_array_length(&grid_transport_state);

  struct grid_port* target_port_array[port_count];

  uint8_t target_port_count = 0;

  for (uint8_t i = 0; i < port_count; i++) {

    struct grid_port* next_port = grid_transport_get_port(&grid_transport_state, i);

    if (next_port->partner_status == 0) {
      continue;
    }

    if (next_port == por && loopback == false) {
      continue;
    }

    if (packet_size > grid_buffer_write_size(&next_port->tx_buffer)) {
      // one of the targetports do not have enough space to store the packet
      grid_platform_printf("Buffer Error: %d/%d \r\n", i, target_port_count);
      grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_BLUE, 128);
      return 0;
    }

    target_port_array[target_port_count] = next_port;
    target_port_count++;
  }

  // Copy packet from source buffer to temp array

  char buffer[packet_size];
  grid_buffer_read_to_chunk(&por->rx_buffer, buffer, packet_size);

  // Copy packet from temp array to target port buffer
  for (uint8_t i = 0; i < target_port_count; i++) {

    struct grid_port* target_port = target_port_array[i];
    grid_buffer_write_from_chunk(&target_port->tx_buffer, buffer, packet_size);
  }

  return 1;
}

struct grid_port* grid_port_allocate(void) {

  struct grid_port* por = (struct grid_port*)grid_platform_allocate_volatile(sizeof(struct grid_port));

  por->tx_buffer.buffer_storage = (char*)grid_platform_allocate_volatile(GRID_BUFFER_SIZE);
  memset(por->tx_buffer.buffer_storage, 0, GRID_BUFFER_SIZE);

  por->rx_buffer.buffer_storage = (char*)grid_platform_allocate_volatile(GRID_BUFFER_SIZE);
  memset(por->rx_buffer.buffer_storage, 0, GRID_BUFFER_SIZE);

  return por;
}

void grid_port_init(struct grid_port* por, uint8_t type, uint8_t dir, uint8_t inbound_loopback) {

  grid_transport_register_port(&grid_transport_state, por);

  grid_buffer_init(&por->tx_buffer, GRID_BUFFER_SIZE);
  grid_buffer_init(&por->rx_buffer, GRID_BUFFER_SIZE);

  por->inbound_loopback = inbound_loopback;

  por->direction = dir;

  por->type = type;

  por->tx_double_buffer_status = 0;
  por->rx_double_buffer_status = 0;
  por->rx_double_buffer_read_start_index = 0;
  por->rx_double_buffer_seek_start_index = 0;
  por->rx_double_buffer_write_index = 0;

  for (uint32_t i = 0; i < GRID_DOUBLE_BUFFER_TX_SIZE; i++) {
    por->tx_double_buffer[i] = 0;
  }
  for (uint32_t i = 0; i < GRID_DOUBLE_BUFFER_RX_SIZE; i++) {
    por->rx_double_buffer[i] = 0;
  }

  por->partner_fi = 0;
  por->dx = 0;
  por->dy = 0;

  por->partner_hwcfg = 0;
  por->partner_status = 1;

  por->ping_local_token = 255;
  por->ping_partner_token = 255;

  por->ping_flag = 0;

  if (type == GRID_PORT_TYPE_USART) {

    por->partner_status = 0;
    por->partner_fi = 0;

    sprintf((char*)por->ping_packet, "%c%c%c%c%02lx%02x%02x%c00\n", GRID_CONST_SOH, GRID_CONST_DCT, GRID_CONST_BELL, por->direction, grid_sys_get_hwcfg(&grid_sys_state), 255, 255, GRID_CONST_EOT);

    por->ping_packet_length = strlen((char*)por->ping_packet);

    grid_msg_string_checksum_write(por->ping_packet, por->ping_packet_length, grid_msg_string_calculate_checksum_of_packet_string(por->ping_packet, por->ping_packet_length));

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
  } else {
    por->partner_status = 1; // UI AND USB are considered to be connected by default
  }
}

uint8_t grid_port_process_outbound_usart(struct grid_port* por) {

  if (por->tx_double_buffer_status != 0) {
    // port is busy, a transmission is already in progress!
    return 0;
  }

  uint16_t packet_size = grid_buffer_read_size(&por->tx_buffer);

  if (!packet_size) {

    // NO PACKET IN RX BUFFER
    return 0;
  }

  // Let's transfer the packet to local memory
  grid_buffer_read_init(&por->tx_buffer);

  por->tx_double_buffer_status = packet_size;

  for (uint16_t i = 0; i < packet_size; i++) {

    uint8_t character = grid_buffer_read_character(&por->tx_buffer);
    por->tx_double_buffer[i] = character;
  }

  // Let's acknowledge the transaction
  grid_buffer_read_acknowledge(&por->tx_buffer);

  // grid_platform_printf("%d %d \r\n", por->tx_double_buffer_status,
  // packet_size);

  grid_platform_send_grid_message(por->direction, por->tx_double_buffer, packet_size);

  return 1;
}

uint8_t grid_port_process_outbound_usb(volatile struct grid_port* por) {

  uint16_t length = grid_buffer_read_size(&por->tx_buffer);

  if (!length) {
    // NO PACKET IN RX BUFFER
    return 0;
  }

  // Clear the tx double buffer
  for (uint16_t i = 0; i < GRID_DOUBLE_BUFFER_TX_SIZE; i++) {
    por->tx_double_buffer[i] = 0;
  }

  grid_buffer_read_to_chunk(&por->tx_buffer, por->tx_double_buffer, length);

  // GRID-2-HOST TRANSLATOR

  uint8_t error = 0;

  for (uint16_t i = 0; i < length; i++) {

    if (por->tx_double_buffer[i] != GRID_CONST_STX) {

      continue;
    }

    // chunk is the specific part of the usb tx doublebuffer that we are
    // currently trying to decode
    char* chunk = &por->tx_double_buffer[i];
    char* header = &por->tx_double_buffer[0];

    uint8_t msg_class = grid_msg_string_get_parameter(chunk, GRID_PARAMETER_CLASSCODE_offset, GRID_PARAMETER_CLASSCODE_length, &error);
    uint8_t msg_instr = grid_msg_string_get_parameter(chunk, GRID_INSTR_offset, GRID_INSTR_length, &error);

    if (msg_class == GRID_CLASS_MIDI_code) {

      grid_decode_midi_to_usb(header, chunk);
    }
    if (msg_class == GRID_CLASS_MIDISYSEX_code) {

      grid_decode_sysex_to_usb(header, chunk);
    } else if (msg_class == GRID_CLASS_HIDMOUSEBUTTON_code) {

      grid_decode_mousebutton_to_usb(header, chunk);
    } else if (msg_class == GRID_CLASS_HIDMOUSEMOVE_code) {

      grid_decode_mousemove_to_usb(header, chunk);
    } else if (msg_class == GRID_CLASS_HIDGAMEPADBUTTON_code) {

      grid_decode_gamepadbutton_to_usb(header, chunk);
    } else if (msg_class == GRID_CLASS_HIDGAMEPADMOVE_code) {

      grid_decode_gamepadmove_to_usb(header, chunk);
    } else if (msg_class == GRID_CLASS_HIDKEYBOARD_code) {

      grid_decode_keyboard_to_usb(header, chunk);
    } else {

      // sprintf(&por->tx_double_buffer[output_cursor], "[UNKNOWN] -> Protocol:
      // %d\n", msg_protocol); output_cursor +=
      // strlen(&por->tx_double_buffer[output_cursor]);
    }
  }

  // Let's send the packet through USB
  grid_platform_usb_serial_write(por->tx_double_buffer, length);

  return 0;
}

void grid_port_receiver_softreset(struct grid_port* por) {

  por->partner_status = 0;
  por->partner_last_timestamp = grid_platform_rtc_get_micros();

  por->rx_double_buffer_seek_start_index = 0;
  por->rx_double_buffer_read_start_index = 0;
  por->rx_double_buffer_write_index = 0;

  grid_platform_reset_grid_transmitter(por->direction);

  for (uint16_t i = 0; i < GRID_DOUBLE_BUFFER_RX_SIZE; i++) {
    por->rx_double_buffer[i] = 0;
  }

  for (uint16_t i = 0; i < GRID_DOUBLE_BUFFER_TX_SIZE; i++) {
    por->tx_double_buffer[i] = 0;
  }
}

void grid_port_receiver_hardreset(struct grid_port* por) {

  grid_platform_printf("HARD: ");

  grid_platform_disable_grid_transmitter(por->direction);

  por->partner_status = 0;

  por->ping_partner_token = 255;
  por->ping_local_token = 255;

  grid_msg_string_write_hex_string_value(&por->ping_packet[8], 2, por->ping_partner_token);
  grid_msg_string_write_hex_string_value(&por->ping_packet[6], 2, por->ping_local_token);
  grid_msg_string_checksum_write(por->ping_packet, por->ping_packet_length, grid_msg_string_calculate_checksum_of_packet_string(por->ping_packet, por->ping_packet_length));

  por->partner_last_timestamp = grid_platform_rtc_get_micros();
  grid_platform_reset_grid_transmitter(por->direction);

  por->rx_double_buffer_seek_start_index = 0;
  por->rx_double_buffer_read_start_index = 0;
  por->rx_double_buffer_write_index = 0;

  for (uint16_t i = 0; i < GRID_DOUBLE_BUFFER_RX_SIZE; i++) {
    por->rx_double_buffer[i] = 0;
  }

  for (uint16_t i = 0; i < GRID_DOUBLE_BUFFER_TX_SIZE; i++) {
    por->tx_double_buffer[i] = 0;
  }

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

void grid_port_debug_printf(char const* fmt, ...) {

  va_list ap;

  char temp[100] = {0};

  va_start(ap, fmt);

  vsnprintf(temp, 99, fmt, ap);

  va_end(ap);

  grid_port_debug_print_text(temp);

  return;
}

uint8_t grid_port_packet_send_everywhere(struct grid_msg_packet* msg) {

  uint32_t message_length = grid_msg_packet_get_length(msg);

  struct grid_port* ui_port = grid_transport_get_port_first_of_type(&grid_transport_state, GRID_PORT_TYPE_UI);

  if (grid_buffer_write_size(&ui_port->rx_buffer) >= message_length) {

    grid_buffer_write_from_packet(&ui_port->rx_buffer, msg);

    return 1;
  } else {

    return 0;
  }
}

void grid_port_ping_try_everywhere(void) {

  // NEW PING

  for (uint8_t i = 0; i < 4; i++) {

    struct grid_port* next_port = grid_transport_get_port(&grid_transport_state, i);

    if (next_port->ping_flag == 0) {
      // no need to ping yet!
      continue;
    }

    if (grid_buffer_write_size(&next_port->tx_buffer) < next_port->ping_packet_length) {
      // not enough space in buffer!
      continue;
    }

    grid_buffer_write_from_chunk(&next_port->tx_buffer, next_port->ping_packet, next_port->ping_packet_length);

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

void grid_protocol_send_heartbeat() {

  uint8_t portstate = 0;

  for (uint8_t i = 0; i < 4; i++) {

    portstate |= ((grid_transport_get_port(&grid_transport_state, i)->partner_status) << i);
  }

  struct grid_msg_packet response;

  grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

  grid_msg_packet_body_append_printf(&response, GRID_CLASS_HEARTBEAT_frame);

  grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);

  grid_msg_packet_body_append_parameter(&response, GRID_CLASS_HEARTBEAT_TYPE_offset, GRID_CLASS_HEARTBEAT_TYPE_length, grid_msg_get_heartbeat_type(&grid_msg_state));
  grid_msg_packet_body_append_parameter(&response, GRID_CLASS_HEARTBEAT_HWCFG_offset, GRID_CLASS_HEARTBEAT_HWCFG_length, grid_sys_get_hwcfg(&grid_sys_state));
  grid_msg_packet_body_append_parameter(&response, GRID_CLASS_HEARTBEAT_VMAJOR_offset, GRID_CLASS_HEARTBEAT_VMAJOR_length, GRID_PROTOCOL_VERSION_MAJOR);
  grid_msg_packet_body_append_parameter(&response, GRID_CLASS_HEARTBEAT_VMINOR_offset, GRID_CLASS_HEARTBEAT_VMINOR_length, GRID_PROTOCOL_VERSION_MINOR);
  grid_msg_packet_body_append_parameter(&response, GRID_CLASS_HEARTBEAT_VPATCH_offset, GRID_CLASS_HEARTBEAT_VPATCH_length, GRID_PROTOCOL_VERSION_PATCH);

  grid_msg_packet_body_append_parameter(&response, GRID_CLASS_HEARTBEAT_PORTSTATE_offset, GRID_CLASS_HEARTBEAT_PORTSTATE_length, portstate);

  if (grid_msg_get_heartbeat_type(&grid_msg_state) == 1) { // I am usb connected deevice

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

void grid_port_process_outbound_ui(struct grid_port* por) {

  uint16_t length = grid_buffer_read_size(&por->tx_buffer);

  if (length == 0) {
    // NO PACKET IN RX BUFFER
    return;
  }

  char message[GRID_PARAMETER_PACKET_maxlength] = {0};

  // Let's transfer the packet to local memory
  grid_buffer_read_to_chunk(&por->tx_buffer, message, length);

  // GRID-2-UI TRANSLATOR

  uint8_t error = 0;

  uint8_t id = grid_msg_string_get_parameter(message, GRID_BRC_ID_offset, GRID_BRC_ID_length, &error);

  uint8_t dx = grid_msg_string_get_parameter(message, GRID_BRC_DX_offset, GRID_BRC_DX_length, &error);
  uint8_t dy = grid_msg_string_get_parameter(message, GRID_BRC_DY_offset, GRID_BRC_DY_length, &error);

  uint8_t sx = grid_msg_string_get_parameter(message, GRID_BRC_SX_offset, GRID_BRC_SX_length, &error);
  uint8_t sy = grid_msg_string_get_parameter(message, GRID_BRC_SY_offset, GRID_BRC_SY_length, &error);

  uint8_t rot = grid_msg_string_get_parameter(message, GRID_BRC_ROT_offset, GRID_BRC_ROT_length, &error);
  uint8_t portrot = grid_msg_string_get_parameter(message, GRID_BRC_PORTROT_offset, GRID_BRC_PORTROT_length, &error);

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

    uint8_t msg_class = grid_msg_string_get_parameter(chunk, GRID_PARAMETER_CLASSCODE_offset, GRID_PARAMETER_CLASSCODE_length, &error);

    if (msg_class == GRID_CLASS_PAGEACTIVE_code) { // dont check address!

      grid_decode_pageactive_to_ui(header, chunk);
    }
    if (msg_class == GRID_CLASS_PAGECOUNT_code) {

      // get page count
      grid_decode_pagecount_to_ui(header, chunk);
    }
    if (msg_class == GRID_CLASS_MIDI_code) {

      // midi rx to lua
      grid_decode_midi_to_ui(header, chunk);
    } else if (msg_class == GRID_CLASS_IMEDIATE_code) {

      // run <?lua ... ?> style immediate script
      grid_decode_imediate_to_ui(header, chunk);
    } else if (msg_class == GRID_CLASS_HEARTBEAT_code) {

      grid_decode_heartbeat_to_ui(header, chunk);
    } else if (msg_class == GRID_CLASS_SERIALNUMBER_code) {

      grid_decode_serialmuber_to_ui(header, chunk);
    } else if (msg_class == GRID_CLASS_UPTIME_code) {

      grid_decode_uptime_to_ui(header, chunk);
    } else if (msg_class == GRID_CLASS_RESETCAUSE_code) {

      // Generate RESPONSE
      grid_decode_resetcause_to_ui(header, chunk);
    } else if (msg_class == GRID_CLASS_RESET_code && (position_is_me)) {

      // request immediate system reset
      grid_decode_reset_to_ui(header, chunk);
    }

    else if (msg_class == GRID_CLASS_PAGEDISCARD_code) {

      grid_decode_pagediscard_to_ui(header, chunk);
    } else if (msg_class == GRID_CLASS_PAGESTORE_code) {

      grid_decode_pagestore_to_ui(header, chunk);
    } else if (msg_class == GRID_CLASS_PAGECLEAR_code) {

      grid_decode_pageclear_to_ui(header, chunk);
    }

    else if (msg_class == GRID_CLASS_NVMERASE_code) {

      grid_decode_nvmerase_to_ui(header, chunk);
    } else if (msg_class == GRID_CLASS_NVMDEFRAG_code) {

      grid_decode_nvmdefrag_to_ui(header, chunk);
    } else if (msg_class == GRID_CLASS_CONFIG_code) {

      grid_decode_config_to_ui(header, chunk);
    } else if (msg_class == GRID_CLASS_HIDKEYSTATUS_code) {

      grid_decode_hidkeystatus_to_ui(header, chunk);
    } else {
      // SORRY
    }
  }
}
