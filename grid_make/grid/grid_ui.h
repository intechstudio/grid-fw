#ifndef GRID_UI_H_INCLUDED
#define GRID_UI_H_INCLUDED

#include "grid_module.h"



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
	

	uint8_t	action_string[GRID_PARAMETER_ACTIONSTRING_maxlength];

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

	struct grid_d51_task* task;
	

	enum grid_ui_status status;
	
	uint8_t page_activepage;
	uint8_t page_count;

	uint8_t page_change_enabled;
	uint8_t page_negotiated;

	uint8_t						element_list_length;
	struct	grid_ui_element*	element_list;
	

};


struct grid_ui_template_buffer* grid_ui_template_buffer_create(struct grid_ui_element* ele);

uint8_t grid_ui_template_buffer_list_length(struct grid_ui_element* ele);

struct grid_ui_template_buffer* grid_ui_template_buffer_find(struct grid_ui_element* ele, uint8_t page);


volatile struct grid_ui_model grid_ui_state;

void grid_ui_model_init(struct grid_ui_model* mod, uint8_t element_list_length);
void grid_ui_element_init(struct grid_ui_model* parent, uint8_t index, enum grid_ui_element_t element_type);
void grid_ui_event_init(struct grid_ui_element* ele, uint8_t index, enum grid_ui_event_t event_type);


uint8_t grid_ui_recall_event_configuration(struct grid_ui_model* ui, struct grid_nvm_model* nvm, uint8_t page, uint8_t element, enum grid_ui_event_t event_type);

void grid_ui_event_register_actionstring(struct grid_ui_event* eve, uint8_t* targetstring);
void grid_ui_event_generate_actionstring(struct grid_ui_event* eve, uint8_t* targetstring);

uint8_t grid_ui_page_load(struct grid_ui_model* ui, struct grid_nvm_model* nvm, uint8_t page);


struct grid_ui_event* grid_ui_event_find(struct grid_ui_element* ele, enum grid_ui_event_t event_type);

void grid_ui_event_trigger(struct grid_ui_event* eve);
void grid_ui_event_trigger_local(struct grid_ui_event* eve);


void grid_ui_event_reset(struct grid_ui_event* eve);

uint8_t grid_ui_event_istriggered(struct grid_ui_event* eve);
uint8_t grid_ui_event_istriggered_local(struct grid_ui_event* eve);

uint32_t grid_ui_event_render_event(struct grid_ui_event* eve, uint8_t* target_string);
uint32_t grid_ui_event_render_action(struct grid_ui_event* eve, uint8_t* target_string);


void grid_port_process_ui(struct grid_ui_model* ui, struct grid_port* por);



#endif