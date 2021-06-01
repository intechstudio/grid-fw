#ifndef GRID_SYS_H_INCLUDED
#define GRID_SYS_H_INCLUDED

#include "grid_module.h"
#include "grid_buf.h"

// Recent messages buffer allows detection and termination of duplicate messages
// Store: dX, dY, ID, ID

int32_t grid_utility_map(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max);


#define GRID_SYS_RECENT_MESSAGES_LENGTH			32
#define GRID_SYS_RECENT_MESSAGES_INDEX_T		uint8_t

#define GRID_SYS_BANK_MAXNUMBER					4

struct grid_lastheader{

	uint8_t status;
	uint8_t id;

};



struct grid_sys_model
{
	
	uint32_t uptime;
	uint8_t reset_cause;
	
	uint8_t editor_connected;
	uint32_t editor_heartbeat_lastrealtime;


	struct grid_lastheader lastheader_config;
	struct grid_lastheader lastheader_configstore;
	struct grid_lastheader lastheader_configerase;
	struct grid_lastheader lastheader_configdiscard;


    uint8_t sessionid;
	
	uint8_t alert_color_red;
	uint8_t alert_color_green;
	uint8_t alert_color_blue;
	
	uint16_t alert_state;
	uint8_t alert_style;
	uint8_t alert_code;
	
	uint8_t alert_color_changed;
	
	
	uint8_t bank_activebank_number;

	uint8_t mapmodestate;
	
	
	
	uint8_t bank_active_changed;
	
	uint8_t bank_setting_changed_flag;
	
	uint8_t bank_enabled[GRID_SYS_BANK_MAXNUMBER];
	
	uint8_t bank_color_r[GRID_SYS_BANK_MAXNUMBER];
	uint8_t bank_color_g[GRID_SYS_BANK_MAXNUMBER];
	uint8_t bank_color_b[GRID_SYS_BANK_MAXNUMBER];
	
	uint8_t bank_activebank_valid;
	
	uint8_t bank_activebank_color_r;
	uint8_t bank_activebank_color_g;
	uint8_t bank_activebank_color_b;
	
	uint8_t bank_init_flag;
	
	uint32_t realtime;


	uint32_t hwfcg;
	uint8_t heartbeat_type;
	
	int8_t module_x;
	int8_t module_y;
	uint8_t module_rot;
	
	
	uint32_t recent_messages[GRID_SYS_RECENT_MESSAGES_LENGTH];
	GRID_SYS_RECENT_MESSAGES_INDEX_T recent_messages_index;
	
	uint8_t next_broadcast_message_id;
	
};

volatile struct grid_sys_model grid_sys_state;


/// TASK SWITCHER

#define GRID_TASK_NUMBER 8

enum grid_task{
	
	GRID_TASK_IDLE,
	GRID_TASK_UNDEFINED,
	GRID_TASK_RECEIVE,
	GRID_TASK_REPORT,
	GRID_TASK_INBOUND,
	GRID_TASK_OUTBOUND,
	GRID_TASK_LED,
	GRID_TASK_ALERT,
	
};

struct grid_task_model{
	
	uint8_t status;
	enum grid_task current_task;
	
	uint32_t timer[GRID_TASK_NUMBER];
	
};

struct grid_task_model grid_task_state;

enum grid_task grid_task_enter_task(struct grid_task_model* mod, enum grid_task next_task);

void grid_task_leave_task(struct grid_task_model* mod, enum grid_task previous_task);

void grid_task_timer_tick(struct grid_task_model* mod);

void grid_task_timer_reset(struct grid_task_model* mod);

uint32_t grid_task_timer_read(struct grid_task_model* mod, enum grid_task task);







/* ==================== Reading MCU Unique Serial Nuber ====================== */
// Word 0: 0x008061FC	Word 1: 0x00806010	Word 2: 0x00806014	Word 3: 0x00806018

#define GRID_D51_UNIQUE_ID_ADDRESS_0 0x008061FC
#define GRID_D51_UNIQUE_ID_ADDRESS_1 0x00806010
#define GRID_D51_UNIQUE_ID_ADDRESS_2 0x00806014
#define GRID_D51_UNIQUE_ID_ADDRESS_3 0x00806018



#define GRID_SYS_DEFAULT_POSITION 127
#define GRID_SYS_LOCAL_POSITION 255
#define GRID_SYS_GLOBAL_POSITION 0
#define GRID_SYS_DEFAULT_ROTATION 0
#define GRID_SYS_DEFAULT_AGE 0


uint32_t grid_sys_unittest(void);

void grid_sys_recall_configuration(struct grid_sys_model* sys, uint8_t bank);

void grid_debug_print_text(uint8_t* str);

void grid_debug_printf(char const *fmt, ...);


struct io_descriptor *grid_sys_north_io;
struct io_descriptor *grid_sys_east_io;
struct io_descriptor *grid_sys_south_io;
struct io_descriptor *grid_sys_west_io;


uint32_t grid_sys_rtc_get_time(struct grid_sys_model* mod);


void grid_sys_rtc_set_time(struct grid_sys_model* mod, uint32_t tvalue);

uint32_t grid_sys_rtc_get_elapsed_time(struct grid_sys_model* mod, uint32_t told);

void grid_sys_rtc_tick_time(struct grid_sys_model* mod);

uint32_t grid_sys_get_hwcfg(struct grid_sys_model* mod);



uint8_t grid_sys_bank_enable(struct grid_sys_model* mod, uint8_t banknumber);
uint8_t grid_sys_bank_disable(struct grid_sys_model* mod, uint8_t banknumber);

uint8_t grid_sys_bank_set_color(struct grid_sys_model* mod, uint8_t banknumber, uint32_t rgb);
uint32_t grid_sys_bank_get_color(struct grid_sys_model* mod, uint8_t banknumber);



uint8_t grid_sys_get_bank_num(struct grid_sys_model* mod);
uint8_t grid_sys_get_bank_next(struct grid_sys_model* mod);

uint8_t grid_sys_get_bank_number_of_first_valid(struct grid_sys_model* mod);

uint8_t grid_sys_get_bank_valid(struct grid_sys_model* mod);

uint8_t grid_sys_get_bank_red(struct grid_sys_model* mod);
uint8_t grid_sys_get_bank_gre(struct grid_sys_model* mod);
uint8_t grid_sys_get_bank_blu(struct grid_sys_model* mod);

uint8_t grid_sys_get_map_state(struct grid_sys_model* mod);


void grid_sys_set_bank(struct grid_sys_model* mod, uint8_t value);

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


uint32_t grid_msg_get_parameter(uint8_t* message, uint8_t offset, uint8_t length, uint8_t* error);
uint32_t grid_msg_set_parameter(uint8_t* message, uint8_t offset, uint8_t length, uint32_t value, uint8_t* error);


uint8_t grid_msg_calculate_checksum_of_packet_string(uint8_t* str, uint32_t length);
uint8_t grid_msg_calculate_checksum_of_string(uint8_t* str, uint32_t length);

uint8_t grid_msg_checksum_read(uint8_t* str, uint32_t length);
void grid_msg_checksum_write(uint8_t* message, uint32_t length, uint8_t checksum);


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