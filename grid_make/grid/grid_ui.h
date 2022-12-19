#ifndef GRID_UI_H_INCLUDED
#define GRID_UI_H_INCLUDED

#include "grid_module.h"

#include <stdint.h>
#include <stdlib.h>


enum grid_ui_status {
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

	enum grid_ui_status status;	
	
	struct grid_ui_element* parent;
	uint8_t index;
	
	enum grid_ui_event_t trigger;
	
	enum grid_ui_event_t type;
	
	uint8_t* action_string;

	uint8_t function_name[10];

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
	enum grid_ui_status status;

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


	enum grid_ui_status status;
	
	uint8_t page_activepage;
	uint8_t page_count;

	uint8_t page_change_enabled;
	uint8_t page_negotiated;
	uint8_t ui_interaction_enabled;

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


struct grid_ui_template_buffer* grid_ui_template_buffer_create(struct grid_ui_element* ele);
uint8_t grid_ui_template_buffer_list_length(struct grid_ui_element* ele);
struct grid_ui_template_buffer* grid_ui_template_buffer_find(struct grid_ui_element* ele, uint8_t page);


volatile struct grid_ui_model grid_ui_state;

void grid_ui_model_init(struct grid_ui_model* mod, uint8_t element_list_length);
void grid_ui_element_init(struct grid_ui_model* parent, uint8_t index, enum grid_ui_element_t element_type);
void grid_ui_event_init(struct grid_ui_element* ele, uint8_t index, enum grid_ui_event_t event_type);



uint8_t grid_ui_event_isdefault_actionstring(struct grid_ui_event* eve, uint8_t* action_string);
void grid_ui_event_register_actionstring(struct grid_ui_event* eve, uint8_t* targetstring);
void grid_ui_event_generate_actionstring(struct grid_ui_event* eve, uint8_t* targetstring);

uint8_t grid_ui_page_clear_template_parameters(struct grid_ui_model* ui, uint8_t page);


struct grid_ui_event* grid_ui_event_find(struct grid_ui_element* ele, enum grid_ui_event_t event_type);

void grid_ui_event_trigger(struct grid_ui_event* eve);
void grid_ui_event_trigger_local(struct grid_ui_event* eve);


void grid_ui_event_reset(struct grid_ui_event* eve);

uint8_t grid_ui_event_istriggered(struct grid_ui_event* eve);
uint8_t grid_ui_event_istriggered_local(struct grid_ui_event* eve);

uint32_t grid_ui_event_render_event(struct grid_ui_event* eve, uint8_t* target_string);
uint32_t grid_ui_event_render_action(struct grid_ui_event* eve, uint8_t* target_string);

void* grid_ui_event_allocate_actionstring(struct grid_ui_event* eve, uint32_t length);
void grid_ui_event_free_actionstring(struct grid_ui_event* eve);


void grid_port_process_ui(struct grid_ui_model* ui, struct grid_port* por);


// requires NVM
uint8_t grid_ui_page_load(struct grid_ui_model* ui, uint8_t page);
uint8_t grid_ui_recall_event_configuration(struct grid_ui_model* ui, uint8_t page, uint8_t element, enum grid_ui_event_t event_type, uint8_t* targetstring);




uint8_t grid_ui_bluk_anything_is_in_progress(struct grid_ui_model* ui);

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


void	grid_ui_bulk_nvmdefrag_init(struct grid_nvm_model* nvm, void (*success_cb)());
uint8_t grid_ui_bulk_nvmdefrag_is_in_progress(struct grid_nvm_model* nvm);
void	grid_ui_bulk_nvmdefrag_next(struct grid_nvm_model* nvm);



#endif