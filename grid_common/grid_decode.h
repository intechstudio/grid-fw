#ifndef GRID_DECODE_H_INCLUDED
#define GRID_DECODE_H_INCLUDED

#include "grid_buf.h"
#include "grid_led.h"
#include "grid_lua_api.h"
#include "grid_msg.h"
#include "grid_protocol.h"
#include "grid_sys.h"
#include "grid_ui.h"
#include "grid_usb.h"

#include <stdarg.h>
#include <stdint.h>
#include <string.h>

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

#endif
