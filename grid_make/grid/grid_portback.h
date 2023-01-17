#ifndef GRID_PORTBACK_H_INCLUDED
#define GRID_PORTBACK_H_INCLUDED

#include "grid_module.h"

#include "grid_sys.h"
#include "grid_msg.h"
#include "grid_led.h"
#include "grid_protocol.h"

#include "grid_buf.h"

#include <stdint.h>
#include <stdarg.h>
#include <string.h>

#include "grid_port.h"

extern uint8_t grid_platform_send_grid_message(uint8_t direction, uint8_t* buffer, uint16_t length);
extern uint8_t grid_platform_disable_grid_transmitter(uint8_t direction);
extern uint8_t grid_platform_reset_grid_transmitter(uint8_t direction);
extern uint8_t grid_platform_enable_grid_transmitter(uint8_t direction);





void grid_port_receive_decode(struct grid_port* por, uint16_t startcommand, uint16_t len);

void grid_port_receive_task(struct grid_port* por);




uint8_t grid_port_process_outbound_usart(struct grid_port* por);
uint8_t grid_port_process_outbound_usb(struct grid_port* por);
uint8_t grid_port_process_outbound_ui(struct grid_port* por);



void grid_protocol_nvm_erase_succcess_callback();
void grid_protocol_nvm_clear_succcess_callback();
void grid_protocol_nvm_read_succcess_callback();
void grid_protocol_nvm_store_succcess_callback();
void grid_protocol_nvm_defrag_succcess_callback();

#endif