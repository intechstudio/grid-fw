#ifndef GRID_BUF_H
#define GRID_BUF_H

#include <stdint.h>
#include <stdlib.h>

#include "grid_msg.h"
#include "grid_protocol.h"

extern void grid_platform_printf(char const* fmt, ...);

extern uint8_t grid_platform_send_grid_message(uint8_t direction, char* buffer, uint16_t length);
extern void* grid_platform_allocate_volatile(size_t size);

#define GRID_BUFFER_SIZE 2000 // 1000 this is the buffer for internal routing

// +4 for status bytes on esp32 to rp2040 transmission
#define GRID_DOUBLE_BUFFER_TX_SIZE (GRID_PARAMETER_DOUBLE_BUFFER_MINIMUM_SIZE)
#define GRID_DOUBLE_BUFFER_RX_SIZE 3000 // 600

struct grid_buffer {

  uint8_t index;
  struct grid_transport* parent;

  uint16_t buffer_length;
  char* buffer_storage;

  uint16_t read_start;
  uint16_t read_stop;
  uint16_t read_active;

  uint16_t read_length;

  uint16_t write_start;
  uint16_t write_stop;
  uint16_t write_active;

  uint32_t write_init_error_count;
  uint32_t write_acknowledge_error_count;
};

uint8_t grid_buffer_init(struct grid_buffer* buf, uint16_t length);

uint16_t grid_buffer_write_size(struct grid_buffer* buf);

uint16_t grid_buffer_read_size(struct grid_buffer* buf);
uint16_t grid_buffer_read_init(struct grid_buffer* buf);

uint16_t grid_buffer_read_next_length();
uint8_t grid_buffer_read_character(struct grid_buffer* buf);
uint8_t grid_buffer_read_acknowledge();  // OK, delete
uint8_t grid_buffer_read_nacknowledge(); // Restart packet
uint8_t grid_buffer_read_cancel();       // Discard packet

uint8_t grid_buffer_read_to_chunk(struct grid_buffer* buf, char* chunk, uint16_t length);
uint8_t grid_buffer_read_to_packet(struct grid_buffer* buf, struct grid_msg_packet* packet, uint16_t length);

uint16_t grid_buffer_get_space(struct grid_buffer* buf);

uint16_t grid_buffer_write_init(struct grid_buffer* buf, uint16_t length);
uint8_t grid_buffer_write_character(struct grid_buffer* buf, uint8_t character);

uint8_t grid_buffer_write_acknowledge(struct grid_buffer* buf);
uint8_t grid_buffer_write_cancel(struct grid_buffer* buf);

uint8_t grid_buffer_write_from_chunk(struct grid_buffer* buf, char* chunk, uint16_t length);
uint8_t grid_buffer_write_from_packet(struct grid_buffer* buf, struct grid_msg_packet* packet);

#endif /* GRID_BUF_H */
