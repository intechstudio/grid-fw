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

#define GRID_SYS_DEFAULT_POSITION 127

#define  GRID_MODULE_P16_RevB		0
#define  GRID_MODULE_B16_RevB		128
#define  GRID_MODULE_PBF4_RevA		64

#define  GRID_MODULE_EN16_RevA		0xC0




// Recent messages buffer allows detection and termination of duplicate messages
// Store: dX, dY, ID, ID



#define GRID_SYS_RECENT_MESSAGES_LENGTH			128
#define GRID_SYS_RECENT_MESSAGES_INDEX_T		uint8_t

struct grid_sys_model 
{
	
	
	uint8_t color_red;
	uint8_t color_green;
	uint8_t color_blue;
		
	uint32_t recent_messages[GRID_SYS_RECENT_MESSAGES_LENGTH];
	GRID_SYS_RECENT_MESSAGES_INDEX_T recent_messages_index;	
	
	uint8_t next_broadcast_message_id;
	
	uint16_t error_state;
	uint8_t error_style;
	uint8_t error_code;	
	
};


struct grid_sys_model grid_sys_state;



struct io_descriptor *grid_sys_north_io;
struct io_descriptor *grid_sys_east_io;
struct io_descriptor *grid_sys_south_io;
struct io_descriptor *grid_sys_west_io;





#endif