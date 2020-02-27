#ifndef GRID_UI_H_INCLUDED
#define GRID_UI_H_INCLUDED

#include "grid_module.h"

enum grid_report_type_t {
	GRID_REPORT_TYPE_UNDEFINED,
	GRID_REPORT_TYPE_LOCAL,
	GRID_REPORT_TYPE_BROADCAST,
	GRID_REPORT_TYPE_DIRECT_ALL,
	GRID_REPORT_TYPE_DIRECT_NORTH,
	GRID_REPORT_TYPE_DIRECT_EAST,
	GRID_REPORT_TYPE_DIRECT_SOUTH,
	GRID_REPORT_TYPE_DIRECT_WEST
};


#define  GRID_REPORT_OFFSET 7

enum grid_report_index_t {
	
		
	
	GRID_REPORT_INDEX_HEARTBEAT,
	
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

struct grid_ui_model
{
	uint8_t report_length;
	uint8_t report_offset;
	struct grid_ui_report* report_array;	
	
	struct grid_ui_report* report_ui_array;
};



volatile struct grid_ui_model grid_ui_state;


void grid_port_process_ui(struct grid_port* por);


uint8_t grid_ui_model_init(struct grid_ui_model* mod, uint8_t len);


uint8_t grid_report_init(struct grid_ui_model* mod, uint8_t index, enum grid_report_type_t type, uint8_t* p, uint32_t p_len, uint8_t* h, uint32_t h_len);

uint8_t grid_report_ui_init(struct grid_ui_model* mod, uint8_t index, enum grid_report_type_t type, uint8_t* p, uint32_t p_len, uint8_t* h, uint32_t h_len);

uint8_t grid_report_sys_init(struct grid_ui_model* mod);





uint8_t grid_report_render(struct grid_ui_model* mod, uint8_t index, uint8_t* target);




enum grid_report_type_t grid_report_get_type(struct grid_ui_model* mod, uint8_t index);


uint8_t grid_report_ui_get_changed_flag(struct grid_ui_model* mod, uint8_t index);

void grid_report_ui_set_changed_flag(struct grid_ui_model* mod, uint8_t index);

void grid_report_ui_clear_changed_flag(struct grid_ui_model* mod, uint8_t index);



uint8_t grid_report_sys_get_changed_flag(struct grid_ui_model* mod, uint8_t index);

void grid_report_sys_set_changed_flag(struct grid_ui_model* mod, uint8_t index);

void grid_report_sys_clear_changed_flag(struct grid_ui_model* mod, uint8_t index);


#endif