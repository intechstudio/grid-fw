/*
 * grid_usb.h
 *
 * Created: 7/6/2020 12:07:42 PM
 *  Author: suku
 */ 


#ifndef GRID_USB_H_
#define GRID_USB_H_

#include "grid_module.h"

static uint8_t *cdcdf_demo_buf;
static bool grid_usb_serial_bulkout_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count);
static bool grid_usb_serial_bulkin_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count);
static bool grid_usb_serial_statechange_cb(usb_cdc_control_signal_t state);
void grid_usb_serial_init();


static bool grid_usb_midi_bulkout_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count);
static bool grid_usb_midi_bulkin_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count);

void grid_usb_midi_init();

volatile grid_usb_serial_bulkin_flag;

struct grid_midi_event_desc {
	
	uint8_t byte0;
	uint8_t byte1;
	uint8_t byte2;
	uint8_t byte3;

};


uint16_t grid_midi_tx_write_index;
uint16_t grid_midi_tx_read_index;


#define GRID_MIDI_TX_BUFFER_length 300

struct grid_midi_event_desc grid_midi_tx_buffer[GRID_MIDI_TX_BUFFER_length];

void grid_midi_buffer_init(struct grid_midi_event_desc* buf, uint16_t length);

uint8_t grid_midi_tx_push(struct grid_midi_event_desc midi_event);
uint8_t grid_midi_tx_pop();


struct grid_keyboard_event_desc {
	
	uint8_t keycode;
	uint8_t ismodifier;
	uint8_t ispressed;
	uint32_t delay;

};

uint16_t grid_keyboard_tx_write_index;
uint16_t grid_keyboard_tx_read_index;

uint32_t grid_keyboard_tx_rtc_lasttimestamp;







#define GRID_KEYBOARD_TX_BUFFER_length 300

struct grid_keyboard_event_desc grid_keyboard_tx_buffer[GRID_MIDI_TX_BUFFER_length];

void grid_keyboard_buffer_init(struct grid_keyboard_event_desc* buf, uint16_t length);

uint8_t grid_keyboard_tx_push(struct grid_keyboard_event_desc keyboard_event);
uint8_t grid_keyboard_tx_pop();





#define GRID_KEYBOARD_KEY_maxcount 6

struct grid_keyboard_model{
	
	struct hiddf_kb_key_descriptors hid_key_array[GRID_KEYBOARD_KEY_maxcount]; 
	struct  grid_keyboard_event_desc key_list[GRID_KEYBOARD_KEY_maxcount];
	uint8_t key_active_count;
    
    uint8_t isenabled;
	
};

struct grid_keyboard_model grid_keyboard_state;

void grid_keyboard_init(struct grid_keyboard_model* kb);

uint8_t grid_keyboard_cleanup(struct grid_keyboard_model* kb);

uint8_t grid_keyboard_keychange(struct grid_keyboard_model* kb, struct grid_keyboard_event_desc* key);






#endif /* GRID_USB_H_ */