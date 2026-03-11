#ifndef GRID_DECODE_H
#define GRID_DECODE_H

#include <stdint.h>

#include "grid_msg.h"

struct grid_decoder_collection {

  uint8_t class;
  uint8_t (*process)(char* header, char* chunk);
};

extern struct grid_decoder_collection* grid_decoder_to_ui_reference;
extern struct grid_decoder_collection* grid_decoder_to_usb_reference;

int grid_port_decode_class(struct grid_decoder_collection* decoder_collection, uint16_t class, char* header, char* chunk);
void grid_port_decode_msg(struct grid_decoder_collection* coll, struct grid_msg* msg);

#endif /* GRID_DECODE_H */
