#pragma once

#ifndef GRID_UI_H_INCLUDED
#define GRID_UI_H_INCLUDED

// only for uint definitions
#include  <stdint.h>
// only for malloc
#include  <stdlib.h>

#include  <string.h>

#include "grid_protocol.h"
#include "grid_ain.h"
#include "grid_lua_api.h"

extern void grid_platform_printf(char const *fmt, ...);

extern void* grid_platform_find_actionstring_file(uint8_t page, uint8_t element, uint8_t event_type);
extern uint16_t grid_platform_get_actionstring_file_size(void* file_pointer);
extern uint32_t grid_platform_read_actionstring_file_contents(void* file_pointer, char* targetstring);
extern void grid_platform_delete_actionstring_file(void* file_pointer);
extern void grid_platform_write_actionstring_file(uint8_t page, uint8_t element, uint8_t event_type, char* buffer, uint16_t length);

extern void	grid_platform_clear_actionstring_files_from_page(uint8_t page);
extern void grid_platform_delete_actionstring_files_all();

extern uint8_t grid_platform_get_nvm_state();
extern uint8_t grid_platform_erase_nvm_next();

enum grid_ui_status_t {
	GRID_UI_STATUS_UNDEFINED,
	GRID_UI_STATUS_INITIALIZED,
	GRID_UI_STATUS_TRAP,
	
	GRID_UI_STATUS_OK,
	
	GRID_UI_STATUS_READY,
	GRID_UI_STATUS_TRIGGERED,
            
	GRID_UI_STATUS_TRIGGERED_LOCAL
	
};



struct grid_ui_event
{

	enum grid_ui_status_t status;	
	
	struct grid_ui_element* parent;
	uint8_t index;
	
	enum grid_ui_status_t trigger;
	
	enum grid_ui_event_t type;
	
	char* action_string;

	char function_name[10];

	uint8_t cfg_changed_flag;
	uint8_t cfg_default_flag;
	uint8_t cfg_flashempty_flag;
	
};


struct grid_ui_template_buffer{

	struct grid_ui_element* parent;
	uint8_t page_number;
	uint8_t status;
	int32_t* template_parameter_list;
	struct grid_ui_template_buffer* next;

};


struct grid_ui_element
{
	enum grid_ui_status_t status;

 	uint32_t timer_event_helper;
	
	struct grid_ui_model* parent;
	uint8_t index;
	
	enum grid_ui_element_t type;

	void (*template_initializer)(struct grid_ui_template_buffer*);

	struct grid_ui_template_buffer* template_buffer_list_head;

	uint8_t template_parameter_list_length;
	int32_t* template_parameter_list;

	void (*page_change_cb)(struct grid_ui_element*, uint8_t, uint8_t);
	void (*event_clear_cb)(struct grid_ui_event*);

	uint8_t						event_list_length;
	struct grid_ui_event*		event_list;
	
};



struct grid_ui_model
{

	struct grid_port* port;

	enum grid_ui_status_t status;
	
	uint8_t page_activepage;
	uint8_t page_count;

	uint8_t page_change_enabled;
	uint8_t page_negotiated;
	uint8_t ui_interaction_enabled;

	uint8_t mapmode_state;

	uint8_t						element_list_length;
	struct	grid_ui_element*	element_list;
	

	/// BULK OPERATIONS ///
	uint8_t read_bulk_status;
	uint8_t read_bulk_last_element;
	uint8_t read_bulk_last_event;
	
	uint8_t erase_bulk_status;

    
	uint8_t store_bulk_status;

	uint8_t clear_bulk_status;


	uint8_t bulk_nvmdefrag_status;
	uint8_t bulk_nvmdefrag_stage; // 0: move, 1: erase



	void ( *read_success_callback)();
	void ( *erase_success_callback)();
	void ( *store_success_callback)();
	void ( *clear_success_callback)();
	void ( *defrag_success_callback)();

};


extern struct grid_ui_model grid_ui_state;


void grid_ui_model_init(struct grid_ui_model* mod, struct grid_port* port, uint8_t element_list_length);




void grid_ui_element_init(struct grid_ui_model* parent, uint8_t index, enum grid_ui_element_t element_type);
void grid_ui_event_init(struct grid_ui_element* ele, uint8_t index, enum grid_ui_event_t event_type);

void grid_ui_rtc_ms_tick_time(struct grid_ui_model* ui);
void grid_ui_rtc_ms_mapmode_handler(struct grid_ui_model* ui, uint8_t new_mapmode_value);

struct grid_ui_template_buffer* grid_ui_template_buffer_create(struct grid_ui_element* ele);
uint8_t grid_ui_template_buffer_list_length(struct grid_ui_element* ele);
struct grid_ui_template_buffer* grid_ui_template_buffer_find(struct grid_ui_element* ele, uint8_t page);

uint8_t grid_ui_page_get_activepage(struct grid_ui_model* ui);
uint8_t grid_ui_page_get_next(struct grid_ui_model* ui);
uint8_t grid_ui_page_get_prev(struct grid_ui_model* ui);


void grid_ui_page_load(struct grid_ui_model* ui, uint8_t page);
void grid_ui_page_load_success_callback(void);

void grid_ui_page_clear_template_parameters(struct grid_ui_model* ui, uint8_t page);
uint8_t grid_ui_page_change_is_enabled(struct grid_ui_model* ui);


struct grid_ui_event* grid_ui_event_find(struct grid_ui_element* ele, enum grid_ui_event_t event_type);

void* 		grid_ui_event_allocate_actionstring(struct grid_ui_event* eve, uint32_t length);
void 		grid_ui_event_free_actionstring(struct grid_ui_event* eve);
uint8_t 	grid_ui_event_isdefault_actionstring(struct grid_ui_event* eve, char* action_string);
void 		grid_ui_event_register_actionstring(struct grid_ui_event* eve, char* targetstring);
uint32_t 	grid_ui_event_render_event(struct grid_ui_event* eve, char* target_string);
void 		grid_ui_event_generate_actionstring(struct grid_ui_event* eve, char* targetstring);
uint32_t 	grid_ui_event_render_action(struct grid_ui_event* eve, char* target_string);
void 		grid_ui_event_recall_configuration(struct grid_ui_model* ui, uint8_t page, uint8_t element, enum grid_ui_event_t event_type, char* targetstring);

// grid ui event trigger 
void 		grid_ui_event_trigger(struct grid_ui_event* eve);
void 		grid_ui_event_trigger_local(struct grid_ui_event* eve);
void 		grid_ui_event_reset(struct grid_ui_event* eve);
uint8_t 	grid_ui_event_istriggered(struct grid_ui_event* eve);
uint8_t 	grid_ui_event_istriggered_local(struct grid_ui_event* eve);
uint16_t 	grid_ui_event_count_istriggered(struct grid_ui_model* ui);
uint16_t 	grid_ui_event_count_istriggered_local(struct grid_ui_model* ui);



struct grid_ui_element* grid_ui_element_find(struct grid_ui_model* ui, uint8_t element_number);

void 		grid_ui_element_timer_set(struct grid_ui_element* ele, uint32_t duration);
void 		grid_ui_element_set_template_parameter(struct grid_ui_element* ele, uint8_t template_index, int32_t value);
int32_t 	grid_ui_element_get_template_parameter(struct grid_ui_element* ele, uint8_t template_index);

void 		grid_ui_element_potmeter_template_parameter_init(struct grid_ui_template_buffer* buf);
void 		grid_ui_element_button_template_parameter_init(struct grid_ui_template_buffer* buf);
void 		grid_ui_element_encoder_template_parameter_init(struct grid_ui_template_buffer* buf);

void		grid_ui_element_button_event_clear_cb(struct grid_ui_event* eve);
void 		grid_ui_element_button_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new);

void 		grid_ui_element_encoder_event_clear_cb(struct grid_ui_event* eve);
void 		grid_ui_element_encoder_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new);

void 		grid_ui_element_potmeter_event_clear_cb(struct grid_ui_event* eve);
void 		grid_ui_element_potmeter_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new);


void	grid_ui_bulk_pageread_init(struct grid_ui_model* ui, void (*success_cb)());
uint8_t grid_ui_bulk_pageread_is_in_progress(struct grid_ui_model* ui);
void	grid_ui_bulk_pageread_next(struct grid_ui_model* ui);

void	grid_ui_bulk_pagestore_init(struct grid_ui_model* ui, void (*success_cb)());
uint8_t grid_ui_bulk_pagestore_is_in_progress(struct grid_ui_model* ui);
void	grid_ui_bulk_pagestore_next(struct grid_ui_model* ui);

void	grid_ui_bulk_pageclear_init(struct grid_ui_model* ui, void (*success_cb)());
uint8_t grid_ui_bulk_pageclear_is_in_progress(struct grid_ui_model* ui);
void	grid_ui_bulk_pageclear_next(struct grid_ui_model* ui);

void	grid_ui_bulk_nvmerase_init(struct grid_ui_model* ui, void (*success_cb)());
uint8_t grid_ui_bulk_nvmerase_is_in_progress(struct grid_ui_model* ui);
void	grid_ui_bulk_nvmerase_next(struct grid_ui_model* ui);


uint8_t grid_ui_bluk_anything_is_in_progress(struct grid_ui_model* ui);

void grid_port_process_ui_local_UNSAFE(struct grid_ui_model* ui);

void grid_port_process_ui_UNSAFE(struct grid_ui_model* ui);


#endif /* GRID_UI_H_INCLUDED */