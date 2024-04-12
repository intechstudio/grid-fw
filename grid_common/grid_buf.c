/*
 * grid_buf.c
 *
 * Created: 4/12/2019 5:27:13 PM
 * Author : SUKU WC
 */

#include "grid_buf.h"

uint8_t grid_buffer_init(struct grid_buffer* buf, uint16_t length) {

  buf->buffer_length = length;

  buf->read_length = 0;

  buf->read_start = 0;
  buf->read_stop = 0;
  buf->read_active = 0;

  buf->write_start = 0;
  buf->write_stop = 0;
  buf->write_active = 0;

  buf->write_init_error_count = 0;
  buf->write_acknowledge_error_count = 0;

  for (uint16_t i = 0; i < buf->buffer_length; i++) {
    buf->buffer_storage[i] = 0;
  }

  return 1;
}

uint16_t grid_buffer_write_size(struct grid_buffer* buf) {

  uint16_t space = 0;

  if (buf->read_start > buf->write_start) {
    space = buf->read_start - buf->write_start;
  } else {
    space = buf->buffer_length - buf->write_start + buf->read_start;
  }

  return space;
}

uint16_t grid_buffer_get_space(struct grid_buffer* buf) {

  uint16_t space = 0;

  if (buf->read_start > buf->write_start) {
    space = buf->read_start - buf->write_start;
  } else {
    space = buf->buffer_length - buf->write_start + buf->read_start;
  }

  return space;
}

uint16_t grid_buffer_write_init(struct grid_buffer* buf, uint16_t length) {

  uint16_t space = grid_buffer_get_space(buf);

  if (space > length) {

    buf->write_stop = (buf->write_start + length) % buf->buffer_length;

    return length;
  } else {

    buf->write_init_error_count++;
    return 0; // failed
  }
}

uint8_t grid_buffer_write_character(struct grid_buffer* buf, uint8_t character) {

  buf->buffer_storage[buf->write_active] = character;

  buf->write_active++;
  buf->write_active %= buf->buffer_length;

  return 1;
}

uint8_t grid_buffer_write_acknowledge(struct grid_buffer* buf) {

  if (buf->write_active == buf->write_stop) {

    buf->write_start = buf->write_active;
    return 1;
  } else {

    buf->write_acknowledge_error_count++;
    return 0;
  }
}

uint8_t grid_buffer_write_cancel(struct grid_buffer* buf) {

  buf->write_active = buf->write_start;
  buf->write_stop = buf->write_start;

  return 1;
}

uint16_t grid_buffer_read_size(struct grid_buffer* buf) {

  // Seek message end character
  for (uint16_t i = 0; i < buf->buffer_length; i++) {

    uint16_t index = (buf->read_start + i) % buf->buffer_length;

    // Hit the write pointer, no message
    if (index == buf->write_start)
      return 0;

    if (buf->buffer_storage[index] == '\n') {

      return i + 1; // packet length
    }
  }

  return 0;
}

uint8_t grid_buffer_write_from_chunk(struct grid_buffer* buf, char* chunk, uint16_t length) {

  if (grid_buffer_write_init(buf, length)) {

    for (uint16_t i = 0; i < length; i++) {

      grid_buffer_write_character(buf, chunk[i]);
    }

    grid_buffer_write_acknowledge(buf);

    return 1;
  } else {

    return 0;
  }
}

uint8_t grid_buffer_write_from_packet(struct grid_buffer* buf, struct grid_msg_packet* packet) {

  uint32_t message_length = grid_msg_packet_get_length(packet);

  // Put the packet into the UI_TX buffer
  if (grid_buffer_write_init(buf, message_length)) {

    for (uint32_t i = 0; i < message_length; i++) {

      grid_buffer_write_character(buf, grid_msg_packet_send_char_by_char(packet, i));
    }

    grid_buffer_write_acknowledge(buf);

    return 1;
  } else {
    return 0;
  }
}

uint16_t grid_buffer_read_init(struct grid_buffer* buf) {

  // Seek message end character
  for (uint16_t i = 0; i < buf->buffer_length; i++) {

    uint16_t index = (buf->read_start + i) % buf->buffer_length;

    // Hit the write pointer, no message
    if (index == buf->write_start)
      return 0;

    if (buf->buffer_storage[index] == '\n') {

      buf->read_stop = (index + 1) % buf->buffer_length;

      buf->read_length = i + 1;

      return buf->read_length; // packet length
    }
  }

  return 0;
}

uint8_t grid_buffer_read_character(struct grid_buffer* buf) {

  // Check if packet is not over
  if (buf->read_active != buf->read_stop) {

    uint8_t character = buf->buffer_storage[buf->read_active];

    buf->read_active++;
    buf->read_active %= buf->buffer_length;

    return character;
  } else {

    while (1) {
      // TRAP: TRANSMISSION WAS OVER ALREADY
    }
  }
}

// TRANSMISSION WAS ACKNOWLEDGED, PACKET CAN BE DELETED
uint8_t grid_buffer_read_acknowledge(struct grid_buffer* buf) {

  // Check if packet is really over
  if (buf->read_active == buf->read_stop) {
    buf->read_start = buf->read_stop;
    return 1;
  } else {

    while (1) {
      // TRAP: TRANSMISSION WAS NOT OVER YET
    }
  }
}

// JUMP BACK TO THE BEGINNING OF THE PACKET
uint8_t grid_buffer_read_nacknowledge(struct grid_buffer* buf) {

  buf->read_active = buf->read_start;

  return 1;
}

// DISCARD PACKET
uint8_t grid_buffer_read_cancel(struct grid_buffer* buf) {

  buf->read_active = buf->read_stop;
  buf->read_start = buf->read_stop;

  return 1;
}

uint8_t grid_buffer_read_to_chunk(struct grid_buffer* buf, char* chunk, uint16_t length) {

  grid_buffer_read_init(buf);

  for (uint16_t i = 0; i < length; i++) {

    chunk[i] = grid_buffer_read_character(buf);
  }
  grid_buffer_read_acknowledge(buf);

  return 1;
}

uint8_t grid_buffer_read_to_packet(struct grid_buffer* buf, struct grid_msg_packet* packet, uint16_t length) {

  grid_buffer_read_init(buf);

  for (uint16_t i = 0; i < length; i++) {

    uint8_t nextchar = grid_buffer_read_character(buf);

    grid_msg_packet_receive_char_by_char(packet, nextchar);
  }

  // Let's acknowledge the transfer	(should wait for partner to send ack)
  grid_buffer_read_acknowledge(buf);

  return 1;
}
