#ifndef GRID_MODULE_H_INCLUDED
#define GRID_MODULE_H_INCLUDED

#include "../atmel_start.h"
#include "../samd51a/include/samd51n20a.h"
//#include "../thirdparty/RTOS/hal_rtos.h"


#include <stdio.h>
#include <stdlib.h>


 
 
 
#include "d51/grid_d51.h"
#include "d51/grid_d51_uart.h"
#include "d51/grid_d51_usb.h"
 
#include "grid_protocol.h"

#include "grid_module_bu16.h"
#include "grid_module_po16.h"
#include "grid_module_pbf4.h"
#include "grid_module_en16.h"
#include "grid_module_ef44.h"

#include "atmel_start_pins.h"


#include "grid_usb.h"


#include "grid_ain.h"
#include "grid_portback.h"
#include "grid_port.h"
#include "grid_led.h"




#include "grid_sys.h"
#include "grid_ui.h"

#include "grid_d51_nvm.h"
#include "grid_msg.h"

#include "grid_lua_api_back.h"




//====================== GRID SYNC ===================================//
enum grid_sync_selector { GRID_SYNC_UNDEFINED, GRID_SYNC_1, GRID_SYNC_2};
enum grid_sync_mode { GRID_SYNC_INITIAL, GRID_SYNC_MASTER, GRID_SYNC_SLAVE};



void grid_module_common_init(void);

void grid_module_init_animation(struct grid_led_model* mod);


void grid_element_potmeter_template_parameter_init(struct grid_ui_template_buffer* buf);
void grid_element_button_template_parameter_init(struct grid_ui_template_buffer* buf);
void grid_element_encoder_template_parameter_init(struct grid_ui_template_buffer* buf);



void grid_element_button_event_clear_cb(struct grid_ui_event* eve);
void grid_element_button_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new);

void grid_element_encoder_event_clear_cb(struct grid_ui_event* eve);
void grid_element_encoder_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new);

void grid_element_potmeter_event_clear_cb(struct grid_ui_event* eve);
void grid_element_potmeter_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new);


#endif
