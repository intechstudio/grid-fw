#ifndef GRID_DECODE_H
#define GRID_DECODE_H

#include <stdint.h>

struct grid_decoder_collection {

  uint8_t class;
  uint8_t (*process)(char* header, char* chunk);
};

extern struct grid_decoder_collection* grid_decoder_to_ui_reference;
extern struct grid_decoder_collection* grid_decoder_to_usb_reference;

int grid_port_decode_class(struct grid_decoder_collection* decoder_collection, uint16_t class, char* header, char* chunk);

// =================== USB OUTBOUND ================= //

uint8_t grid_decode_midi_to_usb(char* header, char* chunk);
uint8_t grid_decode_sysex_to_usb(char* header, char* chunk);

uint8_t grid_decode_mousebutton_to_usb(char* header, char* chunk);
uint8_t grid_decode_mousemove_to_usb(char* header, char* chunk);

uint8_t grid_decode_gamepadmove_to_usb(char* header, char* chunk);
uint8_t grid_decode_gamepadbutton_to_usb(char* header, char* chunk);

uint8_t grid_decode_keyboard_to_usb(char* header, char* chunk);

// =================== UI OUTBOUND ================= //

uint8_t grid_decode_pageactive_to_ui(char* header, char* chunk);
uint8_t grid_decode_pagecount_to_ui(char* header, char* chunk);
uint8_t grid_decode_midi_to_ui(char* header, char* chunk);
uint8_t grid_decode_sysex_to_ui(char* header, char* chunk);
uint8_t grid_decode_imediate_to_ui(char* header, char* chunk);
uint8_t grid_decode_heartbeat_to_ui(char* header, char* chunk);

uint8_t grid_decode_serialmuber_to_ui(char* header, char* chunk);
uint8_t grid_decode_uptime_to_ui(char* header, char* chunk);
uint8_t grid_decode_resetcause_to_ui(char* header, char* chunk);
uint8_t grid_decode_reset_to_ui(char* header, char* chunk);

uint8_t grid_decode_pagediscard_to_ui(char* header, char* chunk);
uint8_t grid_decode_pagestore_to_ui(char* header, char* chunk);
uint8_t grid_decode_pageclear_to_ui(char* header, char* chunk);

uint8_t grid_decode_nvmerase_to_ui(char* header, char* chunk);
uint8_t grid_decode_nvmdefrag_to_ui(char* header, char* chunk);

uint8_t grid_decode_config_to_ui(char* header, char* chunk);

uint8_t grid_decode_hidkeystatus_to_ui(char* header, char* chunk);

#endif /* GRID_DECODE_H */
