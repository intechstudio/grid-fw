#ifndef GRID_UI_H_INCLUDED
#define GRID_UI_H_INCLUDED

#include "grid_module.h"

enum grid_report_type_t {
	GRID_REPORT_TYPE_UNDEFINED,
	GRID_REPORT_TYPE_LOCAL,
	GRID_REPORT_TYPE_BROADCAST,
	GRID_REPORT_TYPE_DIRECT_NORTH,
	GRID_REPORT_TYPE_DIRECT_EAST,
	GRID_REPORT_TYPE_DIRECT_SOUTH,
	GRID_REPORT_TYPE_DIRECT_WEST
};



enum grid_ui_status {
	GRID_UI_STATUS_UNDEFINED,
	GRID_UI_STATUS_INITIALIZED,
	GRID_UI_STATUS_TRAP
};


#define  GRID_REPORT_OFFSET 6

enum grid_report_index_t {
		
	GRID_REPORT_INDEX_PING_NORTH,
	GRID_REPORT_INDEX_PING_EAST,
	GRID_REPORT_INDEX_PING_SOUTH,
	GRID_REPORT_INDEX_PING_WEST,	
	GRID_REPORT_INDEX_MAPMODE,
	GRID_REPORT_INDEX_CFG_REQUEST
};


struct grid_ui_report
{
	uint8_t changed;
	
	enum grid_report_type_t type;
	
	uint8_t payload_length;
	uint8_t* payload;
	
	uint8_t helper_length;
	uint8_t* helper;
};

struct grid_report_model
{
	uint16_t report_length;
	uint16_t report_offset;
	struct grid_ui_report* report_array;		
	struct grid_ui_report* report_ui_array;
	
};


enum grid_ui_event_t {
	
	GRID_UI_EVENT_INIT,
	
	GRID_UI_EVENT_HEARTBEAT,
		
	GRID_UI_EVENT_AVC7,
	GRID_UI_EVENT_RVC,
	GRID_UI_EVENT_DVC,
	GRID_UI_EVENT_DP,
	GRID_UI_EVENT_DR,
	GRID_UI_EVENT_DD,
		
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


enum grid_ui_element_t {
	
	GRID_UI_ELEMENT_SYSTEM,
	GRID_UI_ELEMENT_POTENTIOMETER,
	GRID_UI_ELEMENT_BUTTON,
	GRID_UI_ELEMENT_ENCODER,
	
};

#define GRID_TEMPLATE_PARAMETER_LIST_LENGTH     8

enum grid_template_parameter_index_t {
	
	GRID_TEMPLATE_PARAMETER_CONTROLLER_NUMBER,
	GRID_TEMPLATE_PARAMETER_CONTROLLER_NUMBER_REVERSED,
	GRID_TEMPLATE_PARAMETER_CONTROLLER_AV7,
	GRID_TEMPLATE_PARAMETER_CONTROLLER_AV8,
	GRID_TEMPLATE_PARAMETER_CONTROLLER_AV14U,
	GRID_TEMPLATE_PARAMETER_CONTROLLER_AV14L,
	GRID_TEMPLATE_PARAMETER_CONTROLLER_DV7,
	GRID_TEMPLATE_PARAMETER_CONTROLLER_DV8
	
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








void grid_ui_event_register_action(struct grid_ui_element* ele, enum grid_ui_event_t event_type, uint8_t* event_string, uint32_t event_string_length, struct grid_ui_action_parameter* parameter_list, uint8_t parameter_list_length);


void grid_ui_event_register_action_smart(struct grid_ui_element* ele, enum grid_ui_event_t event_type, uint8_t* event_string, uint32_t event_string_length);




uint8_t grid_ui_event_find(struct grid_ui_element* ele, enum grid_ui_event_t event_type);

void grid_ui_event_trigger(struct grid_ui_event* eve);
void grid_ui_event_reset(struct grid_ui_event* eve);

uint8_t grid_ui_event_istriggered(struct grid_ui_event* eve);


uint8_t grid_ui_event_render_action(struct grid_ui_event* eve, uint8_t* target_string);

uint8_t grid_ui_event_template_action(struct grid_ui_element* ele, uint8_t event_index);




volatile struct grid_report_model grid_report_state; //GET RID OF THIS



volatile struct grid_ui_model grid_ui_state;
volatile struct grid_ui_model grid_core_state;




void grid_ui_model_init(struct grid_ui_model* mod, uint8_t element_list_length);
void grid_ui_element_init(struct grid_ui_element* ele, enum grid_ui_element_t element_type);

void grid_ui_event_init(struct grid_ui_event* eve, enum grid_ui_event_t event_type);


void grid_port_process_ui(struct grid_port* por);


uint8_t grid_report_model_init(struct grid_report_model* mod, uint8_t len);


uint8_t grid_report_init(struct grid_report_model* mod, uint8_t index, enum grid_report_type_t type, uint8_t* p, uint32_t p_len, uint8_t* h, uint32_t h_len);

uint8_t grid_report_ui_init(struct grid_report_model* mod, uint8_t index, enum grid_report_type_t type, uint8_t* p, uint32_t p_len, uint8_t* h, uint32_t h_len);

uint8_t grid_report_sys_init(struct grid_report_model* mod);





uint8_t grid_report_render(struct grid_report_model* mod, uint8_t index, uint8_t* target);




enum grid_report_type_t grid_report_get_type(struct grid_report_model* mod, uint8_t index);


uint8_t grid_report_ui_get_changed_flag(struct grid_report_model* mod, uint8_t index);

void grid_report_ui_set_changed_flag(struct grid_report_model* mod, uint8_t index);

void grid_report_ui_clear_changed_flag(struct grid_report_model* mod, uint8_t index);



uint8_t grid_report_sys_get_changed_flag(struct grid_report_model* mod, uint8_t index);

void grid_report_sys_set_changed_flag(struct grid_report_model* mod, uint8_t index);


void grid_report_sys_set_payload_parameter(struct grid_report_model* mod, uint8_t index, uint8_t offset, uint8_t length, uint8_t value);

void grid_report_sys_clear_changed_flag(struct grid_report_model* mod, uint8_t index);


#endif