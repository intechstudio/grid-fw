#ifndef GRID_SYS_H_INCLUDED
#define GRID_SYS_H_INCLUDED

#include "sam.h"

/* ==================== Reading MCU Unique Serial Nuber ====================== */
// Word 0: 0x008061FC	Word 1: 0x00806010	Word 2: 0x00806014	Word 3: 0x00806018

#define GRID_SYS_UNIQUE_ID_ADDRESS_0 0x008061FC
#define GRID_SYS_UNIQUE_ID_ADDRESS_1 0x00806010
#define GRID_SYS_UNIQUE_ID_ADDRESS_2 0x00806014
#define GRID_SYS_UNIQUE_ID_ADDRESS_3 0x00806018

uint32_t grid_sys_hwfcg = -1;



#define  GRID_MODULE_P16_RevB		0
#define  GRID_MODULE_B16_RevB		128
#define  GRID_MODULE_PBF4_RevA		64








typedef struct grid_buffer{
	
	uint16_t buffer_length;
	uint8_t* buffer_storage;
	
	uint16_t read_start;
	uint16_t read_stop;
	uint16_t read_active;
	
	uint16_t read_length;
	
	uint16_t write_start;
	uint16_t write_stop;
	uint16_t write_active;
	
} GRID_BUFFER_t;


// Recent messages buffer allows detection and termination of duplicate messages
// Store: dX, dY, ID, ID

uint32_t grid_com_recent_messages[250];




uint8_t grid_buffer_init(struct grid_buffer* buf, uint16_t length);


uint16_t grid_buffer_read_next_length();
uint8_t grid_buffer_read_character(struct grid_buffer* buf);
uint8_t grid_buffer_read_acknowledge();		// OK, delete
uint8_t grid_buffer_read_nacknowledge();	// Restart packet
uint8_t grid_buffer_read_cancel();			// Discard packet



uint16_t grid_buffer_write_init(struct grid_buffer* buf, uint16_t length);
uint8_t  grid_buffer_write_character(struct grid_buffer* buf, uint8_t character);

uint8_t grid_buffer_write_acknowledge(struct grid_buffer* buf);
uint8_t grid_buffer_write_cancel(struct grid_buffer* buf);	



#endif /* GRID_TEL_H_INCLUDED */