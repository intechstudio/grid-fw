/*
 * usb_protocol_midi.c
 *
 * Created: 9/30/2019 2:31:56 PM
 *  Author: WPC-User
 */ 
#ifndef _USB_PROTOCOL_AUDIO_H_
#define _USB_PROTOCOL_AUDIO_H_

#include "usb_includes.h"



typedef struct
{
	uint8_t header;
	uint8_t byte1;
	uint8_t byte2;
	uint8_t byte3;
} midiEventPacket_t;




#endif