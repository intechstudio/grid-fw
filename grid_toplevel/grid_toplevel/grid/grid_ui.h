#ifndef GRID_UI_H_INCLUDED
#define GRID_UI_H_INCLUDED

#include "grid_module.h"



enum grid_ui_status {
	GRID_UI_STATUS_UNDEFINED,
	GRID_UI_STATUS_INITIALIZED,
	GRID_UI_STATUS_TRAP
};



enum grid_ui_trigger{
	
	GRID_UI_EVENT_STATUS_UNDEFINED,
	GRID_UI_EVENT_STATUS_READY,
	GRID_UI_EVENT_STATUS_TRIGGERED
	
};

enum grid_ui_action_status{
	
	GRID_UI_ACTION_STATUS_UNDEFINED,
	GRID_UI_ACTION_STATUS_OK
	
};



#define GRID_UI_ACTION_STRING_LENGTH	100

#define GRID_UI_ACTION_PARAMETER_COUNT	 10


struct grid_ui_action_parameter{
	
	enum grid_ui_status status;
	
	uint8_t group;
	uint8_t address;
	uint8_t length;
	uint8_t offset;
		
};


struct grid_ui_event
{

	enum grid_ui_status status;	
	enum grid_ui_event_t trigger;
	
	enum grid_ui_event_t type;
	
	uint32_t action_length;
	enum grid_ui_action_status action_status;
	uint8_t* action_string;
	
	uint8_t action_parameter_count;
	struct grid_ui_action_parameter* action_parameter_list;
};


struct grid_ui_element
{
	enum grid_ui_status status;
	
	enum grid_ui_element_t type;
	
	uint32_t* template_parameter_list;
	
	uint8_t						event_list_length;
	struct grid_ui_event*		event_list;
	
};


struct grid_ui_model
{
	enum grid_ui_status status;
	
	uint8_t   element_list_length;
	struct grid_ui_element*		element;
	
};




volatile struct grid_ui_model grid_ui_state;
volatile struct grid_ui_model grid_core_state;


void grid_ui_model_init(struct grid_ui_model* mod, uint8_t element_list_length);
void grid_ui_element_init(struct grid_ui_element* ele, enum grid_ui_element_t element_type);
void grid_ui_event_init(struct grid_ui_event* eve, enum grid_ui_event_t event_type);

void grid_ui_event_register_action(struct grid_ui_element* ele, enum grid_ui_event_t event_type, uint8_t* event_string, uint32_t event_string_length);

uint8_t grid_ui_event_find(struct grid_ui_element* ele, enum grid_ui_event_t event_type);
uint8_t grid_ui_event_template_action(struct grid_ui_element* ele, uint8_t event_index);
void grid_ui_event_trigger(struct grid_ui_event* eve);
void grid_ui_event_reset(struct grid_ui_event* eve);

uint8_t grid_ui_event_istriggered(struct grid_ui_event* eve);

uint8_t grid_ui_event_render_action(struct grid_ui_event* eve, uint8_t* target_string);



void grid_port_process_ui(struct grid_port* por);



#endif