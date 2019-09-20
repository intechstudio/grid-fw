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




// Recent messages buffer allows detection and termination of duplicate messages
// Store: dX, dY, ID, ID

uint32_t grid_com_recent_messages[250];



struct io_descriptor *grid_sys_north_io;
struct io_descriptor *grid_sys_east_io;
struct io_descriptor *grid_sys_south_io;
struct io_descriptor *grid_sys_west_io;





#endif