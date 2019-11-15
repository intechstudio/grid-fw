#ifndef GRID_SYS_H_INCLUDED
#define GRID_SYS_H_INCLUDED

#include "grid_module.h"

/* ==================== Reading MCU Unique Serial Nuber ====================== */
// Word 0: 0x008061FC	Word 1: 0x00806010	Word 2: 0x00806014	Word 3: 0x00806018

#define GRID_SYS_UNIQUE_ID_ADDRESS_0 0x008061FC
#define GRID_SYS_UNIQUE_ID_ADDRESS_1 0x00806010
#define GRID_SYS_UNIQUE_ID_ADDRESS_2 0x00806014
#define GRID_SYS_UNIQUE_ID_ADDRESS_3 0x00806018


uint32_t grid_sys_hwfcg;

#define GRID_SYS_DEFAULT_POSITION 127





// Recent messages buffer allows detection and termination of duplicate messages
// Store: dX, dY, ID, ID



#define GRID_SYS_RECENT_MESSAGES_LENGTH			32
#define GRID_SYS_RECENT_MESSAGES_INDEX_T		uint8_t

struct grid_sys_model 
{
	
	uint8_t age;
	
	uint8_t alert_color_red;
	uint8_t alert_color_green;
	uint8_t alert_color_blue;
	
	uint16_t alert_state;
	uint8_t alert_style;
	uint8_t alert_code;
	
	uint8_t alert_color_changed;
	
	
	uint8_t bank_select;
	
	uint8_t bank_color_r[4];
	uint8_t bank_color_g[4];
	uint8_t bank_color_b[4];
	
		
	
	uint32_t realtime;
		
	uint32_t recent_messages[GRID_SYS_RECENT_MESSAGES_LENGTH];
	GRID_SYS_RECENT_MESSAGES_INDEX_T recent_messages_index;	
	
	uint8_t next_broadcast_message_id;
	
};

volatile struct grid_sys_model grid_sys_state;





struct io_descriptor *grid_sys_north_io;
struct io_descriptor *grid_sys_east_io;
struct io_descriptor *grid_sys_south_io;
struct io_descriptor *grid_sys_west_io;


uint32_t grid_sys_rtc_get_time(struct grid_sys_model* mod);


void grid_sys_rtc_set_time(struct grid_sys_model* mod, uint32_t tvalue);

uint32_t grid_sys_rtc_get_elapsed_time(struct grid_sys_model* mod, uint32_t told);

void grid_sys_rtc_tick_time(struct grid_sys_model* mod);

uint32_t grid_sys_get_hwcfg();

//====================== SYS ALERT ==========================//
uint8_t grid_sys_alert_read_color_changed_flag(struct grid_sys_model* mod);

void grid_sys_alert_set_color_changed_flag(struct grid_sys_model* mod);
void grid_sys_alert_clear_color_changed_flag(struct grid_sys_model* mod);
uint8_t grid_sys_alert_get_color_intensity(struct grid_sys_model* mod);

void grid_sys_alert_set_color(struct grid_sys_model* mod, uint8_t red, uint8_t green, uint8_t blue);

void grid_sys_alert_set_alert(struct grid_sys_model* mod, uint8_t red, uint8_t green, uint8_t blue, uint8_t style, uint16_t duration);

uint8_t grid_sys_alert_get_color_r(struct grid_sys_model* mod);
uint8_t grid_sys_alert_get_color_g(struct grid_sys_model* mod);

uint8_t grid_sys_alert_get_color_b(struct grid_sys_model* mod);

//=========================== SYS MSG ============================//

uint8_t grid_msg_get_id(uint8_t* message);
uint8_t grid_msg_get_dx(uint8_t* message);
uint8_t grid_msg_get_dy(uint8_t* message);
uint8_t grid_msg_get_age(uint8_t* message);

void grid_msg_set_id(uint8_t* message, uint8_t param);
void grid_msg_set_dx(uint8_t* message, uint8_t param);
void grid_msg_set_dy(uint8_t* message, uint8_t param);
void grid_msg_set_age(uint8_t* message, uint8_t param);

//=========================== SYS CB ============================//

static void tx_cb_USART_GRID_N(const struct usart_async_descriptor *const descr);

static void tx_cb_USART_GRID_E(const struct usart_async_descriptor *const descr);

static void tx_cb_USART_GRID_S(const struct usart_async_descriptor *const descr);

static void tx_cb_USART_GRID_W(const struct usart_async_descriptor *const descr);

void tx_cb_USART_GRID(struct grid_port* const por);


static void rx_cb_USART_GRID_N(const struct usart_async_descriptor *const descr);

static void rx_cb_USART_GRID_E(const struct usart_async_descriptor *const descr);

static void rx_cb_USART_GRID_S(const struct usart_async_descriptor *const descr);

static void rx_cb_USART_GRID_W(const struct usart_async_descriptor *const descr);

void rx_cb_USART_GRID(struct grid_port* const por);



void dma_transfer_complete_n_cb(struct _dma_resource *resource);
void dma_transfer_complete_e_cb(struct _dma_resource *resource);
void dma_transfer_complete_s_cb(struct _dma_resource *resource);
void dma_transfer_complete_w_cb(struct _dma_resource *resource);
void dma_transfer_complete(struct grid_port* por);





#endif