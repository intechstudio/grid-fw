#ifndef GRID_MODULE_H_INCLUDED
#define GRID_MODULE_H_INCLUDED

#include "../atmel_start.h"
#include "../samd51a/include/samd51n20a.h"
//#include "../thirdparty/RTOS/hal_rtos.h"


#include <stdio.h>
#include <stdlib.h>
 
 
 #define RTC1SEC 16384

 #define RTC1MS (RTC1SEC/1000)
 
 
#include "grid_d51.h"
 
#include "grid_module_bu16_revb.h"
#include "grid_module_po16_revb.h"
#include "grid_module_pbf4_reva.h"
#include "grid_module_en16_reva.h"

#include "atmel_start_pins.h"


#include "grid_unittest.h"
#include "grid_debug.h"


#include "grid_usb.h"


#include "grid_expr.h"

#include "grid_ain.h"
#include "grid_buf.h"
#include "grid_led.h"

#include "grid_protocol.h"
#include "grid_sys.h"
#include "grid_ui.h"

#include "grid_nvm.h"
#include "grid_msg.h"








//====================== GRID SYNC ===================================//
enum grid_sync_selector { GRID_SYNC_UNDEFINED, GRID_SYNC_1, GRID_SYNC_2};
enum grid_sync_mode { GRID_SYNC_INITIAL, GRID_SYNC_MASTER, GRID_SYNC_SLAVE};



void grid_module_common_init(void);

void grid_module_init_animation(struct grid_led_model* mod);

#endif
