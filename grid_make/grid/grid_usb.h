/*
 * grid_usb.h
 *
 * Created: 7/6/2020 12:07:42 PM
 *  Author: suku
 */ 


#ifndef GRID_USB_H_
#define GRID_USB_H_

#include "grid_module.h"

// only for uint definitions
#include  <stdint.h>
// only for malloc
#include  <stdlib.h>


extern int32_t grid_platform_usb_midi_write(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3);
extern int32_t grid_platform_usb_midi_write_status(void);
extern int32_t grid_platform_usb_mouse_button_change(uint8_t b_state, uint8_t type);
extern int32_t grid_platform_usb_mouse_move(int8_t position, uint8_t axis);
extern int32_t grid_platform_usb_keyboard_keys_state_change(void* keys_desc, uint8_t keys_count);


void grid_usb_midi_init();


/** Describes the USB HID Keyboard Key descriptors. */
struct grid_kb_key_descriptors {
	/* HID Key Value, defined in usb_protocol_hid.h */
	uint8_t key_id;
	/* Flag whether it is a modifier key */
	bool b_modifier;
	/* Key State */
	enum hiddf_kb_key_state state;
};


struct grid_midi_event_desc {
	
	uint8_t byte0;
	uint8_t byte1;
	uint8_t byte2;
	uint8_t byte3;

};



void grid_midi_buffer_init(struct grid_midi_event_desc* buf, uint16_t length);

// Midi tx buffer
#define GRID_MIDI_TX_BUFFER_length 100
struct grid_midi_event_desc grid_midi_tx_buffer[GRID_MIDI_TX_BUFFER_length];

uint16_t grid_midi_tx_write_index;
uint16_t grid_midi_tx_read_index;

uint8_t grid_midi_tx_push(struct grid_midi_event_desc midi_event);
uint8_t grid_midi_tx_pop();

// Midi rx buffer
#define GRID_MIDI_RX_BUFFER_length 100
struct grid_midi_event_desc grid_midi_rx_buffer[GRID_MIDI_RX_BUFFER_length];

uint16_t grid_midi_rx_write_index;
uint16_t grid_midi_rx_read_index;

uint8_t grid_midi_rx_push(struct grid_midi_event_desc midi_event);
uint8_t grid_midi_rx_pop();



struct grid_keyboard_event_desc {
	
	uint8_t keycode;
	uint8_t ismodifier;
	uint8_t ispressed;
	uint32_t delay;

};

uint16_t grid_keyboard_tx_write_index;
uint16_t grid_keyboard_tx_read_index;

uint32_t grid_keyboard_tx_rtc_lasttimestamp;







#define GRID_KEYBOARD_TX_BUFFER_length 101

struct grid_keyboard_event_desc grid_keyboard_tx_buffer[GRID_KEYBOARD_TX_BUFFER_length];

void grid_keyboard_buffer_init(struct grid_keyboard_event_desc* buf, uint16_t length);

uint8_t grid_keyboard_tx_push(struct grid_keyboard_event_desc keyboard_event);
uint8_t grid_keyboard_tx_pop();





#define GRID_KEYBOARD_KEY_maxcount 6

struct grid_keyboard_model{
	
	struct grid_kb_key_descriptors hid_key_array[GRID_KEYBOARD_KEY_maxcount]; 
	struct  grid_keyboard_event_desc key_list[GRID_KEYBOARD_KEY_maxcount];
	uint8_t key_active_count;
    
    uint8_t isenabled;
	
};

struct grid_keyboard_model grid_keyboard_state;

void grid_keyboard_init(struct grid_keyboard_model* kb);

uint8_t grid_keyboard_cleanup(struct grid_keyboard_model* kb);

uint8_t grid_keyboard_keychange(struct grid_keyboard_model* kb, struct grid_keyboard_event_desc* key);






#endif /* GRID_USB_H_ */