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




#define GRID_UI_EVENT_STRING_maxlength		30
#define GRID_UI_EVENT_PARAMETER_maxcount	4


#define GRID_UI_ACTION_STRING_maxlength		120
#define GRID_UI_ACTION_PARAMETER_maxcount	14


struct grid_ui_template_parameter{
	
	enum grid_ui_status status;
	
	uint8_t group;
	uint8_t address;
	uint8_t length;
	uint8_t offset;
		
};


struct grid_ui_event
{

	enum grid_ui_status status;	
	
	struct grid_ui_element* parent;
	uint8_t index;
	
	enum grid_ui_event_t trigger;
	
	enum grid_ui_event_t type;
	
	uint32_t							event_string_length;
	uint8_t								event_string[GRID_UI_EVENT_STRING_maxlength];
	
	uint8_t								event_parameter_count;
	struct grid_ui_template_parameter	event_parameter_list[GRID_UI_EVENT_PARAMETER_maxcount];
	
	uint32_t							action_string_length;
	uint8_t								action_string[GRID_UI_ACTION_STRING_maxlength];
	
	uint8_t								action_parameter_count;
	struct grid_ui_template_parameter	action_parameter_list[GRID_UI_ACTION_PARAMETER_maxcount];
	
	uint8_t cfg_changed_flag;
	uint8_t cfg_default_flag;
	uint8_t cfg_flashempty_flag;
	
};


struct grid_ui_element
{
	enum grid_ui_status status;
	
	struct grid_ui_bank* parent;
	uint8_t index;
	
	enum grid_ui_element_t type;
	
	uint32_t template_parameter_list[GRID_TEMPLATE_UI_PARAMETER_LIST_LENGTH];
	
	uint8_t						event_list_length;
	struct grid_ui_event*		event_list;
	
};

struct grid_ui_bank
{
	enum grid_ui_status status;
	
	struct grid_ui_model* parent;
	uint8_t index;
	
	uint8_t						element_list_length;
	struct	grid_ui_element*	element_list;
	
};


struct grid_ui_model
{
	enum grid_ui_status status;
	
	uint8_t						bank_list_length;
	struct	grid_ui_bank*		bank_list;
	
};




volatile struct grid_ui_model grid_ui_state;
volatile struct grid_ui_model grid_core_state;


void grid_ui_model_init(struct grid_ui_model* mod, uint8_t bank_list_length);
void grid_ui_bank_init(struct grid_ui_model* parent, uint8_t index, uint8_t element_list_length);
void grid_ui_element_init(struct grid_ui_bank* parent, uint8_t index, enum grid_ui_element_t element_type);
void grid_ui_event_init(struct grid_ui_element* parent, uint8_t index, enum grid_ui_event_t event_type);


void grid_ui_nvm_store_all_configuration(struct grid_ui_model* ui, struct grid_nvm_model* nvm);
void grid_ui_nvm_load_all_configuration(struct grid_ui_model* ui, struct grid_nvm_model* nvm);
void grid_ui_nvm_clear_all_configuration(struct grid_ui_model* ui, struct grid_nvm_model* nvm);

uint8_t grid_ui_recall_event_configuration(struct grid_ui_model* ui, uint8_t bank, uint8_t element, enum grid_ui_event_t event_type);

uint8_t grid_ui_nvm_store_event_configuration(struct grid_ui_model* ui, struct grid_nvm_model* nvm, struct grid_ui_event* eve);
uint8_t grid_ui_nvm_load_event_configuration(struct grid_ui_model* ui, struct grid_nvm_model* nvm, struct grid_ui_event* eve);
uint8_t grid_ui_nvm_clear_event_configuration(struct grid_ui_model* ui, struct grid_nvm_model* nvm, struct grid_ui_event* eve);

void grid_ui_reinit(struct grid_ui_model* ui);

void grid_ui_reinit_local(struct grid_ui_model* ui);

void grid_ui_event_register_eventstring(struct grid_ui_element* ele, enum grid_ui_event_t event_type, uint8_t* event_string, uint32_t event_string_length);
void grid_ui_event_generate_eventstring(struct grid_ui_element* ele, enum grid_ui_event_t event_type);

void grid_ui_event_register_actionstring(struct grid_ui_element* ele, enum grid_ui_event_t event_type, uint8_t* action_string, uint32_t action_string_length);
void grid_ui_event_generate_actionstring(struct grid_ui_element* ele, enum grid_ui_event_t event_type);


uint8_t grid_ui_event_find(struct grid_ui_element* ele, enum grid_ui_event_t event_type);
uint8_t grid_ui_event_template_action(struct grid_ui_element* ele, uint8_t event_index);
void grid_ui_event_trigger(struct grid_ui_element* ele, uint8_t event_index);
void grid_ui_event_trigger_local(struct grid_ui_element* ele, uint8_t event_index);

void grid_ui_smart_trigger(struct grid_ui_model* mod, uint8_t bank, uint8_t element, enum grid_ui_event_t event);
void grid_ui_smart_trigger_local(struct grid_ui_model* mod, uint8_t bank, uint8_t element, enum grid_ui_event_t event);

void grid_ui_event_reset(struct grid_ui_event* eve);

uint8_t grid_ui_event_istriggered(struct grid_ui_event* eve);
uint8_t grid_ui_event_istriggered_local(struct grid_ui_event* eve);

uint32_t grid_ui_event_render_action(struct grid_ui_event* eve, uint8_t* target_string);



void grid_port_process_ui(struct grid_port* por);



#endif