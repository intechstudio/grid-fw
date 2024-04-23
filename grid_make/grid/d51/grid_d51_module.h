#ifndef GRID_D51_MODULE_H_INCLUDED
#define GRID_D51_MODULE_H_INCLUDED

#include "../atmel_start.h"
#include "../samd51a/include/samd51n20a.h"
// #include "../thirdparty/RTOS/hal_rtos.h"

#include <stdio.h>
#include <stdlib.h>

#include "d51/grid_d51.h"
#include "d51/grid_d51_uart.h"
#include "d51/grid_d51_usb.h"

#include "grid_protocol.h"

#include "grid_d51_module_bu16.h"
#include "grid_d51_module_ef44.h"
#include "grid_d51_module_en16.h"
#include "grid_d51_module_pbf4.h"
#include "grid_d51_module_po16.h"

#include "atmel_start_pins.h"

#include "grid_usb.h"

#include "grid_ain.h"
#include "grid_led.h"
#include "grid_port.h"

#include "grid_sys.h"

#include "grid_d51_nvm.h"
#include "grid_msg.h"

#include "grid_d51_lua.h"

//====================== GRID SYNC ===================================//
enum grid_sync_selector { GRID_SYNC_UNDEFINED, GRID_SYNC_1, GRID_SYNC_2 };
enum grid_sync_mode { GRID_SYNC_INITIAL, GRID_SYNC_MASTER, GRID_SYNC_SLAVE };

void grid_module_common_init(void);

#endif
