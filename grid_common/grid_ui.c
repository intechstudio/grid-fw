/*
 * grid_ui.c
 *
 * Created: 4/12/2019 5:27:13 PM
 * Author : SUKU WC
*/

#include "grid_ui.h"


struct grid_ui_model grid_ui_state;

void grid_ui_model_init(struct grid_ui_model* mod, struct grid_port* port, uint8_t element_list_length){
	

	mod->port = port;

	mod->status = GRID_UI_STATUS_INITIALIZED;

	mod->page_activepage = 0;
	mod->page_count = 4;

	mod->page_change_enabled = 1;

	mod->element_list_length = element_list_length;	

	mod->element_list = malloc(element_list_length*sizeof(struct grid_ui_element));

	mod->page_negotiated = 0;

	mod->ui_interaction_enabled = 0;

	mod->mapmode_state = 0;

	
	mod->read_bulk_status = 0;
	mod->erase_bulk_status = 0;	
	mod->store_bulk_status = 0;
	mod->clear_bulk_status = 0;
	mod->bulk_nvmdefrag_status = 0;

	mod->read_success_callback = NULL;
	mod->erase_success_callback = NULL;
	mod->store_success_callback = NULL;
	mod->clear_success_callback = NULL;
	mod->defrag_success_callback = NULL;
	
}






void grid_ui_element_init(struct grid_ui_model* parent, uint8_t index, enum grid_ui_element_t element_type){
	

	struct grid_ui_element* ele = &parent->element_list[index];

	parent->element_list[index].event_clear_cb = NULL;
	parent->element_list[index].page_change_cb = NULL;


	ele->parent = parent;
	ele->index = index;

	ele->template_parameter_list_length = 0;
	ele->template_parameter_list = NULL;

	ele->template_buffer_list_head = NULL;

	ele->status = GRID_UI_STATUS_INITIALIZED;
	
	ele->type = element_type;

	ele->timer_event_helper = 0;


	if (element_type == GRID_UI_ELEMENT_SYSTEM){
		
		ele->event_list_length = 4;
		
		ele->event_list = malloc(ele->event_list_length*sizeof(struct grid_ui_event));
		grid_ui_event_init(ele, 0, GRID_UI_EVENT_INIT); // Element Initialization Event
		grid_ui_event_init(ele, 1, GRID_UI_EVENT_MAPMODE_CHANGE); // Mapmode change
		grid_ui_event_init(ele, 2, GRID_UI_EVENT_MIDIRX); // Midi Receive
		grid_ui_event_init(ele, 3, GRID_UI_EVENT_TIMER);

		ele->template_initializer = NULL;
		ele->template_parameter_list_length = 0;


		ele->event_clear_cb = NULL;
		ele->page_change_cb = NULL;
		
	}
	else if (element_type == GRID_UI_ELEMENT_POTENTIOMETER){
		
		ele->event_list_length = 3;
		
		ele->event_list = malloc(ele->event_list_length*sizeof(struct grid_ui_event));
		
		grid_ui_event_init(ele, 0, GRID_UI_EVENT_INIT); // Element Initialization Event
		grid_ui_event_init(ele, 1, GRID_UI_EVENT_AC); // Absolute Value Change (7bit)
		grid_ui_event_init(ele, 2, GRID_UI_EVENT_TIMER);

		ele->template_initializer = &grid_ui_element_potmeter_template_parameter_init;
		ele->template_parameter_list_length = GRID_LUA_FNC_P_LIST_length;
		
		ele->event_clear_cb = &grid_ui_element_potmeter_event_clear_cb;
		ele->page_change_cb = &grid_ui_element_potmeter_page_change_cb;

	}
	else if (element_type == GRID_UI_ELEMENT_BUTTON){
		

		ele->event_list_length = 3;
		
		ele->event_list = malloc(ele->event_list_length*sizeof(struct grid_ui_event));

		grid_ui_event_init(ele, 0, GRID_UI_EVENT_INIT); // Element Initialization Event
		grid_ui_event_init(ele, 1, GRID_UI_EVENT_BC);	// Button Change
		grid_ui_event_init(ele, 2, GRID_UI_EVENT_TIMER);



		ele->template_initializer = &grid_ui_element_button_template_parameter_init;
		ele->template_parameter_list_length = GRID_LUA_FNC_B_LIST_length;

		ele->event_clear_cb = &grid_ui_element_button_event_clear_cb;
		ele->page_change_cb = &grid_ui_element_button_page_change_cb;

	}
	else if (element_type == GRID_UI_ELEMENT_ENCODER){
		
		ele->event_list_length = 4;
		
		ele->event_list = malloc(ele->event_list_length*sizeof(struct grid_ui_event));
		
		grid_ui_event_init(ele, 0, GRID_UI_EVENT_INIT); // Element Initialization Event
		grid_ui_event_init(ele, 1, GRID_UI_EVENT_EC);	// Encoder Change
		grid_ui_event_init(ele, 2, GRID_UI_EVENT_BC);	// Button Change
		grid_ui_event_init(ele, 3, GRID_UI_EVENT_TIMER);

		ele->template_initializer = &grid_ui_element_encoder_template_parameter_init;
		ele->template_parameter_list_length = GRID_LUA_FNC_E_LIST_length;
		
		ele->event_clear_cb = &grid_ui_element_encoder_event_clear_cb;
		ele->page_change_cb = &grid_ui_element_encoder_page_change_cb;

	}
	else{
		//UNKNOWN ELEMENT TYPE
		grid_platform_printf("error.unknown_element_type\r\n");
		ele->template_initializer = NULL;
	}


/*
	if (ele->template_initializer != NULL){

		struct grid_ui_template_buffer* buf = grid_ui_template_buffer_create(ele);

		if (buf != NULL){

			ele->template_parameter_list = buf->template_parameter_list;
		}
		else{
			// TRAP
			while(1){}

		}

	}*/

	
}

void grid_ui_event_init(struct grid_ui_element* ele, uint8_t index, enum grid_ui_event_t event_type){
	

	struct grid_ui_event* eve = &ele->event_list[index];

	eve->parent = ele;
	eve->index = index;

	eve->cfg_changed_flag = 0;
	
	eve->status = GRID_UI_STATUS_INITIALIZED;
	
	eve->type   = event_type;	
	eve->status = GRID_UI_STATUS_READY;

	if (event_type == GRID_UI_EVENT_INIT){
		strcpy(eve->function_name, GRID_LUA_FNC_ACTION_INIT_short);
	}
	else if (event_type == GRID_UI_EVENT_AC){
		strcpy(eve->function_name, GRID_LUA_FNC_ACTION_POTMETERCHANGE_short);
	}
	else if (event_type == GRID_UI_EVENT_EC){
		strcpy(eve->function_name, GRID_LUA_FNC_ACTION_ENCODERCHANGE_short);
	}
	else if (event_type == GRID_UI_EVENT_BC){
		strcpy(eve->function_name, GRID_LUA_FNC_ACTION_BUTTONCHANGE_short);
	}
	else if (event_type == GRID_UI_EVENT_MAPMODE_CHANGE){
		strcpy(eve->function_name, GRID_LUA_FNC_ACTION_MAPMODE_short);
	}
	else if (event_type == GRID_UI_EVENT_MIDIRX){
		strcpy(eve->function_name, GRID_LUA_FNC_ACTION_MIDIRX_short);
	}
	else if (event_type == GRID_UI_EVENT_TIMER){
		strcpy(eve->function_name, GRID_LUA_FNC_ACTION_TIMER_short);
	}
	else{
		
		grid_platform_printf("TRAP: Unknown Event "__FILE__"\r\n");	
		while(1){

		}
	}
	
	eve->action_string = NULL;

	eve->cfg_changed_flag = 0;
	eve->cfg_default_flag = 1;
	eve->cfg_flashempty_flag = 1;

}



void grid_ui_rtc_ms_tick_time(struct grid_ui_model* ui){
	
	for (uint8_t i = 0; i<ui->element_list_length; i++){

		struct grid_ui_element* ele = &ui->element_list[i];

		if (ele->timer_event_helper > 0){

			ele->timer_event_helper--;
			
			if (ele->timer_event_helper == 0){

				printf("tick\r\n");

				struct grid_ui_event* eve = grid_ui_event_find(ele, GRID_UI_EVENT_TIMER);
				
				if (eve != NULL){
				
					printf("bumm\r\n");
					grid_ui_event_trigger(eve);

				}


			}

			
		}
		
	}
}

void grid_ui_rtc_ms_mapmode_handler(struct grid_ui_model* ui, uint8_t new_mapmode_value){

		

	if (new_mapmode_value != ui->mapmode_state){
		
		ui->mapmode_state = new_mapmode_value;
			
		if (ui->mapmode_state == 0){ // RELEASE
			
				
		}
		else{ // PRESS

			

			struct grid_ui_element* sys_ele = &grid_ui_state.element_list[grid_ui_state.element_list_length-1]; 

			struct grid_ui_event* eve = grid_ui_event_find(sys_ele, GRID_UI_EVENT_MAPMODE_CHANGE);
			
			if (eve == NULL){
			}
			else{

			}
				
			grid_ui_event_trigger(eve);		


		}

	}


}

struct grid_ui_template_buffer* grid_ui_template_buffer_create(struct grid_ui_element* ele){
		
	struct grid_ui_template_buffer* this = NULL;
	struct grid_ui_template_buffer* prev = ele->template_buffer_list_head;

	this = malloc(sizeof(struct grid_ui_template_buffer));

	//grid_platform_printf("Template Buffer Create %x \r\n", this);

	if (this == NULL){
		grid_platform_printf("error.ui.MallocFailed1\r\n");
	}
	else{

		this->status = 0;
		this->next = NULL;
		this->page_number = 0;
		this->parent = ele;

		uint8_t allocation_length = ele->template_parameter_list_length;

		if (allocation_length == 0){
			// always allocate at least one element (ESP malloc returns NULL for zero length allocation on system element)
			allocation_length=1;
		}

		this->template_parameter_list = malloc(allocation_length*sizeof(int32_t));

		grid_platform_printf("malloc %d %lx\r\n", ele->template_parameter_list_length, this->template_parameter_list);

		if (this->template_parameter_list == NULL){
			grid_platform_printf("error.ui.MallocFailed2\r\n");
			grid_platform_delay_ms(100);
		}
		else{

			if (ele->template_initializer!=NULL){
				ele->template_initializer(this);
			}

			//grid_platform_printf("LIST\r\n");

			if (prev != NULL){


				while(prev->next != NULL){

					this->page_number++;
					prev = prev->next;

				}

				prev->next = this;
				return this;

			}
			else{
				
				this->parent->template_buffer_list_head = this;
				return this;
				//this is the first item in the list
			}
		}
	}
	
	// FAILED
	return NULL;
}


uint8_t grid_ui_template_buffer_list_length(struct grid_ui_element* ele){

	uint8_t count = 0;

	struct grid_ui_template_buffer* this = ele->template_buffer_list_head;

	while (this != NULL)
	{
		count++;
		this = this->next;
	}
	
	return count;

}

struct grid_ui_template_buffer* grid_ui_template_buffer_find(struct grid_ui_element* ele, uint8_t page){

	uint8_t count = 0;

	struct grid_ui_template_buffer* this = ele->template_buffer_list_head;

	while (this != NULL)
	{
		if (count == page){
			return this;
		}

		count++;
		this = this->next;
	}
	
	return NULL;

}



uint8_t grid_ui_page_get_activepage(struct grid_ui_model* ui){

	return ui->page_activepage;

}


uint8_t grid_ui_page_get_next(struct grid_ui_model* ui){


	return (ui->page_activepage + 1) % ui->page_count;

}

uint8_t grid_ui_page_get_prev(struct grid_ui_model* ui){

	return (ui->page_activepage + ui->page_count - 1) % ui->page_count;

}



void grid_ui_page_load(struct grid_ui_model* ui, uint8_t page){

	/*
	
		Reset encoder mode
		Reset button mode
		Reset potmeter mode/reso
		Reset led animations

	*/

	// reset all of the state parameters of all leds
	grid_led_reset(&grid_led_state);

	uint8_t oldpage = ui->page_activepage;
	ui->page_activepage = page;
	// Call the page_change callback

	//grid_platform_printf("LOAD PAGE: %d\r\n", page);

	for (uint8_t i = 0; i < ui->element_list_length; i++)
	{	

		struct grid_ui_element* ele = grid_ui_element_find(ui, i); 

		if (ele == NULL){
			grid_platform_printf("NULL ELEMENT\r\n");
		} 

		ele->timer_event_helper = 0; // stop the event's timer

		// clear all of the pending events for the element
		for (uint8_t j = 0; j < ele->event_list_length; j++){
			struct grid_ui_event* eve = &ele->event_list[j];

			grid_ui_event_reset(eve);

		}	

		// if (ele->template_initializer!=NULL){
		// 	ele->template_initializer(ele->template_buffer_list_head);
		// }


		uint8_t template_buffer_length = grid_ui_template_buffer_list_length(ele);

		
		grid_platform_printf("Allocating i=%d len=%d\r\n", i, template_buffer_length);

		//if (i==0) grid_platform_printf("TB LEN: %d\r\n", template_buffer_length);
		while (template_buffer_length < page+1){

			grid_platform_printf("$"); // emergency allocation
			grid_ui_template_buffer_create(ele);

			template_buffer_length = grid_ui_template_buffer_list_length(ele);

			//if (i==0) grid_platform_printf("CREATE NEW, LEN: %d\r\n", template_buffer_length);
		}



		//struct grid_ui_template_buffer* buf =  grid_ui_template_buffer_find(ele, page==0?1:page);
		struct grid_ui_template_buffer* buf =  grid_ui_template_buffer_find(ele, page);
		
		if (buf == NULL){

			grid_platform_printf("error.template buffer is invalid\r\n");
			grid_port_debug_print_text("error.template buffer is invalid");

		}
		else{

			// load the template parameter list
			ele->template_parameter_list = buf->template_parameter_list;

			if (buf->template_parameter_list == NULL){
				grid_port_debug_print_text("NULL");
			}

		}


		if (ele->page_change_cb != NULL){

			ele->page_change_cb(ele, oldpage, page);
		}

	}


	grid_platform_printf("STOP\r\n");
	grid_lua_stop_vm(&grid_lua_state);
	grid_platform_printf("START\r\n");
	grid_lua_start_vm(&grid_lua_state);

	grid_ui_bulk_pageread_init(ui, &grid_ui_page_load_success_callback);

	
}

void grid_ui_page_load_success_callback(void){


	grid_platform_printf("LOAD SUCCESS\r\n");
	grid_keyboard_enable(&grid_keyboard_state);


	// phase out the animation
	grid_led_set_alert(&grid_led_state, GRID_LED_COLOR_WHITE_DIM, 100);
	grid_led_set_alert_timeout_automatic(&grid_led_state);
	
}




void grid_ui_page_clear_template_parameters(struct grid_ui_model* ui, uint8_t page){




	for (uint8_t i = 0; i < ui->element_list_length; i++)
	{	
		struct grid_ui_element* ele = &ui->element_list[i];

		struct grid_ui_template_buffer* buf = grid_ui_template_buffer_find(ele, page);

		if (ele->template_initializer!=NULL){
			ele->template_initializer(buf);
		}

		
	}

}



uint8_t grid_ui_page_change_is_enabled(struct grid_ui_model* ui)
{
	return ui->page_change_enabled;

}


struct grid_ui_event* grid_ui_event_find(struct grid_ui_element* ele, enum grid_ui_event_t event_type){

	if (ele == NULL) {
		return NULL;
	}

	uint8_t event_index = 255;
		
	for(uint8_t i=0; i<ele->event_list_length; i++){
		if (ele->event_list[i].type == event_type){
			return &ele->event_list[i];
		}
	}

		
		
	return NULL;
	
}


void* grid_ui_event_allocate_actionstring(struct grid_ui_event* eve, uint32_t length){

	// +1 for termination zero
	eve->action_string = (char*) malloc((length+1) * sizeof(uint8_t));
	return eve->action_string;

}

void grid_ui_event_free_actionstring(struct grid_ui_event* eve){

	if (eve->action_string == NULL){
		// not allocated
		return;
	}

	free(eve->action_string);
	eve->action_string = NULL;

}



uint8_t grid_ui_event_isdefault_actionstring(struct grid_ui_event* eve, char* action_string){

	struct grid_ui_element* ele = eve->parent;

	// ENCODER - INIT
	if (ele->type == GRID_UI_ELEMENT_ENCODER && eve->type == GRID_UI_EVENT_INIT) {
		return (strcmp(action_string, GRID_ACTIONSTRING_INIT_ENC) == 0);
	}
	
	// ENCODER - BUTTON CHANGE
	if (ele->type == GRID_UI_ELEMENT_ENCODER && eve->type == GRID_UI_EVENT_BC) {
		return (strcmp(action_string, GRID_ACTIONSTRING_BC) == 0);
	}	

	// ENCODER - ENCODER CHANGE
	if (ele->type == GRID_UI_ELEMENT_ENCODER && eve->type == GRID_UI_EVENT_EC) {
		return (strcmp(action_string, GRID_ACTIONSTRING_EC) == 0);
	}

	// ENCODER - TIMER
	if (ele->type == GRID_UI_ELEMENT_ENCODER && eve->type == GRID_UI_EVENT_TIMER) {
		return (strcmp(action_string, GRID_ACTIONSTRING_TIMER) == 0);
	}

	// POTMETER - INIT
	if (ele->type == GRID_UI_ELEMENT_POTENTIOMETER && eve->type == GRID_UI_EVENT_INIT) {
		return (strcmp(action_string, GRID_ACTIONSTRING_INIT_POT) == 0);
	}

	// POTMETER - ANALOG CHANGE
	if (ele->type == GRID_UI_ELEMENT_POTENTIOMETER && eve->type == GRID_UI_EVENT_AC) {
		return (strcmp(action_string, GRID_ACTIONSTRING_AC) == 0);
	}

	// POTMETER - TIMER
	if (ele->type == GRID_UI_ELEMENT_POTENTIOMETER && eve->type == GRID_UI_EVENT_TIMER) {
		return (strcmp(action_string, GRID_ACTIONSTRING_TIMER) == 0);
	}

	// BUTTON - INIT
	if (ele->type == GRID_UI_ELEMENT_BUTTON && eve->type == GRID_UI_EVENT_INIT) {
		return (strcmp(action_string, GRID_ACTIONSTRING_INIT_BUT) == 0);
	}

	// BUTTON - ANALOG CHANGE
	if (ele->type == GRID_UI_ELEMENT_BUTTON && eve->type == GRID_UI_EVENT_BC) {
		return (strcmp(action_string, GRID_ACTIONSTRING_BC) == 0);
	}

	// BUTTON - TIMER
	if (ele->type == GRID_UI_ELEMENT_BUTTON && eve->type == GRID_UI_EVENT_TIMER) {
		return (strcmp(action_string, GRID_ACTIONSTRING_TIMER) == 0);
	}

	// SYSTEM - INIT
	if (ele->type == GRID_UI_ELEMENT_SYSTEM && eve->type == GRID_UI_EVENT_INIT) {
		return (strcmp(action_string, GRID_ACTIONSTRING_PAGE_INIT) == 0);
	}

	// SYSTEM - MAPMODE
	if (ele->type == GRID_UI_ELEMENT_SYSTEM && eve->type == GRID_UI_EVENT_MAPMODE_CHANGE) {
		return (strcmp(action_string, GRID_ACTIONSTRING_MAPMODE_CHANGE) == 0);
	}

	// SYSTEM - MIDIRX
	if (ele->type == GRID_UI_ELEMENT_SYSTEM && eve->type == GRID_UI_EVENT_MIDIRX) {
		return (strcmp(action_string, GRID_ACTIONSTRING_MIDIRX) == 0);
	}

	// SYSTEM - TIMER
	if (ele->type == GRID_UI_ELEMENT_SYSTEM && eve->type == GRID_UI_EVENT_TIMER) {
		return (strcmp(action_string, GRID_ACTIONSTRING_TIMER) == 0);
	}


	grid_platform_printf("TRAP: Unknown Element or Event "__FILE__"\r\n");
	while(1){

	}

}


void grid_ui_event_register_actionstring(struct grid_ui_event* eve, char* action_string){

	struct grid_ui_element* ele = eve->parent;

	if (strlen(action_string) == 0){
		//grid_platform_printf("NULLSTRING el:%d, elv:%d\r\n", ele->index, eve->type);
		return;
	}
	
	// by default assume that incoming config is not defaultconfig
	eve->cfg_default_flag = 0;



	char temp[GRID_PARAMETER_ACTIONSTRING_maxlength+100] = {0};
	uint32_t len = strlen(action_string);

	action_string[len-3] = '\0';
	sprintf(temp, "ele[%d].%s = function (self) %s end", ele->index, eve->function_name, &action_string[6]);
	action_string[len-3] = ' ';

	if (grid_ui_event_isdefault_actionstring(eve, action_string)){
		eve->cfg_default_flag = 1;
	}

	if (0 == grid_lua_dostring(&grid_lua_state, temp)){
		grid_port_debug_printf("LUA not OK, Failed to register action! EL: %d EV: %d", ele->index, eve->type);
	};

	if (eve->cfg_default_flag == 0){ // NOT DEFAULT

		// free old action string
		grid_ui_event_free_actionstring(eve);

		// allocate space for the new action string
		grid_ui_event_allocate_actionstring(eve, strlen(action_string));
		strcpy(eve->action_string, action_string);
	}
	else{
		//free
	}

	eve->cfg_changed_flag = 1;

	
}



uint32_t grid_ui_event_render_event(struct grid_ui_event* eve, char* target_string){

	uint8_t page = eve->parent->parent->page_activepage;
	uint8_t element = eve->parent->index;
	uint8_t event = eve->type;
	uint8_t param = 0;

	if (eve->parent->type == GRID_UI_ELEMENT_POTENTIOMETER){
		param = eve->parent->template_parameter_list[GRID_LUA_FNC_P_POTMETER_STATE_index];
	}
	else if (eve->parent->type == GRID_UI_ELEMENT_BUTTON){
		param = eve->parent->template_parameter_list[GRID_LUA_FNC_B_BUTTON_STATE_index];
	}
	else if (eve->parent->type == GRID_UI_ELEMENT_ENCODER && eve->type == GRID_UI_EVENT_EC){
		param = eve->parent->template_parameter_list[GRID_LUA_FNC_E_ENCODER_STATE_index];
	}
	else if (eve->parent->type == GRID_UI_ELEMENT_ENCODER && eve->type == GRID_UI_EVENT_BC){
		param = eve->parent->template_parameter_list[GRID_LUA_FNC_E_BUTTON_STATE_index];
	}

	// map mapmode to element 255
	if (eve->parent->type == GRID_UI_ELEMENT_SYSTEM){
		element = 255;
	}


	sprintf(target_string, GRID_CLASS_EVENT_frame);

	grid_msg_string_set_parameter(target_string, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code, NULL);

	grid_msg_string_set_parameter(target_string, GRID_CLASS_EVENT_PAGENUMBER_offset, GRID_CLASS_EVENT_PAGENUMBER_length, page, NULL);
	grid_msg_string_set_parameter(target_string, GRID_CLASS_EVENT_ELEMENTNUMBER_offset, GRID_CLASS_EVENT_ELEMENTNUMBER_length, element, NULL);
	grid_msg_string_set_parameter(target_string, GRID_CLASS_EVENT_EVENTTYPE_offset, GRID_CLASS_EVENT_EVENTTYPE_length, event, NULL);
	grid_msg_string_set_parameter(target_string, GRID_CLASS_EVENT_EVENTPARAM_offset, GRID_CLASS_EVENT_EVENTPARAM_length, param, NULL);

	return strlen(target_string);

}


void grid_ui_event_generate_actionstring(struct grid_ui_event* eve, char* targetstring){
	
	if (eve->parent->type == GRID_UI_ELEMENT_SYSTEM){
				
		switch(eve->type){
			case GRID_UI_EVENT_INIT:				strcpy(targetstring, GRID_ACTIONSTRING_PAGE_INIT);			break;
			case GRID_UI_EVENT_MAPMODE_CHANGE:		strcpy(targetstring, GRID_ACTIONSTRING_MAPMODE_CHANGE);		break;
			case GRID_UI_EVENT_MIDIRX:				strcpy(targetstring, GRID_ACTIONSTRING_MIDIRX);				break;
			case GRID_UI_EVENT_TIMER:				strcpy(targetstring, GRID_ACTIONSTRING_TIMER);				break;
			default: break;
		}
		
	}
	else if (eve->parent->type == GRID_UI_ELEMENT_BUTTON){
				

		switch(eve->type){
			case GRID_UI_EVENT_INIT:	strcpy(targetstring, GRID_ACTIONSTRING_INIT_BUT);		break;
			case GRID_UI_EVENT_BC:		strcpy(targetstring, GRID_ACTIONSTRING_BC);				break;
			case GRID_UI_EVENT_TIMER:	strcpy(targetstring, GRID_ACTIONSTRING_TIMER);			break;
			default: break;
		}
		
	}
	else if (eve->parent->type == GRID_UI_ELEMENT_POTENTIOMETER){
		
		switch(eve->type){
			case GRID_UI_EVENT_INIT:	strcpy(targetstring, GRID_ACTIONSTRING_INIT_POT);		break;
			case GRID_UI_EVENT_AC:		strcpy(targetstring, GRID_ACTIONSTRING_AC);				break;
			case GRID_UI_EVENT_TIMER:	strcpy(targetstring, GRID_ACTIONSTRING_TIMER);			break;
			default: break;
		}
		
	}
	else if (eve->parent->type == GRID_UI_ELEMENT_ENCODER){
		
		switch(eve->type){
			case GRID_UI_EVENT_INIT:        strcpy(targetstring, GRID_ACTIONSTRING_INIT_ENC);	break;
			case GRID_UI_EVENT_EC:        	strcpy(targetstring, GRID_ACTIONSTRING_EC);			break;
			case GRID_UI_EVENT_BC:			strcpy(targetstring, GRID_ACTIONSTRING_BC);			break;
			case GRID_UI_EVENT_TIMER:		strcpy(targetstring, GRID_ACTIONSTRING_TIMER);		break;
			default: break;
		}
			
	}
	
	
}




uint32_t grid_ui_event_render_action(struct grid_ui_event* eve, char* target_string){

	char temp[GRID_PARAMETER_ACTIONSTRING_maxlength + 100] = {0};

	uint32_t i=0;

	sprintf(temp, "<?lua ele[%d]:%s(self) ?>", eve->parent->index, eve->function_name);

	// new php style implementation
	uint32_t code_start = 0;
	uint32_t code_end = 0;
	uint32_t code_length = 0;
	uint32_t code_type = 0;  // 0: nocode, 1: expr, 2: lua

	uint32_t total_substituted_length = 0;


	uint32_t length = strlen(temp);

	for(i=0; i<length ; i++){

		target_string[i-total_substituted_length] = temp[i];

		if (0 == strncmp(&temp[i], "<?lua ", 6)){

			//grid_platform_printf("<?lua \r\n");
			code_start = i;
			code_end = i;
			code_type = 2; // 2=lua


		}
		else if (0 == strncmp(&temp[i], " ?>", 3)){

			code_end = i + 3; // +3 because  ?>
			//grid_platform_printf(" ?>\r\n");

			if (code_type == 2){ //LUA
				
				temp[i] = 0; // terminating zero for lua dostring

				if (0 == grid_lua_dostring(&grid_lua_state, &temp[code_start+6])){
					grid_port_debug_printf("LUA not OK! EL: %d EV: %d", eve->parent->index, eve->index);
				};

				uint32_t code_stdo_length = strlen(grid_lua_get_output_string(&grid_lua_state));

				temp[i] = ' '; // reverting terminating zero to space

				i+= 3-1; // +3 because  ?> -1 because i++
				code_length = code_end - code_start;

				strcpy(&target_string[code_start-total_substituted_length], grid_lua_get_output_string(&grid_lua_state));
				
				uint8_t errorlen = 0;

				if (strlen(grid_lua_get_error_string(&grid_lua_state))){


					char errorbuffer[100] = {0};

					sprintf(errorbuffer, GRID_CLASS_DEBUGTEXT_frame_start);
					strcat(errorbuffer, grid_lua_get_error_string(&grid_lua_state));
					sprintf(&errorbuffer[strlen(errorbuffer)], GRID_CLASS_DEBUGTEXT_frame_end);
					
					errorlen = strlen(errorbuffer);

					strcpy(&target_string[code_start-total_substituted_length+code_stdo_length], errorbuffer);

					grid_lua_clear_stde(&grid_lua_state);

				}


				total_substituted_length += code_length - code_stdo_length - errorlen;


				//grid_lua_debug_memory_stats(&grid_lua_state, "Ui");
				grid_lua_clear_stdo(&grid_lua_state);

			}
			code_type = 0;
			
		}

	}


	// Call the event clear callback

	if (eve->parent->event_clear_cb != NULL){

		eve->parent->event_clear_cb(eve);
	}
	
	return length - total_substituted_length;
		
}



void grid_ui_event_recall_configuration(struct grid_ui_model* ui, uint8_t page, uint8_t element, enum grid_ui_event_t event_type, char* targetstring){
	

	struct grid_ui_element* ele = &ui->element_list[element];

	struct grid_ui_event* eve = grid_ui_event_find(ele, event_type);


	if (eve == NULL){

		grid_platform_printf("warning."__FILE__".event does not exist!\r\n");
		return;
	}



	// Event actually exists


	if (ui->page_activepage == page){
		// currently active page needs to be sent

		// file pointer
		void* entry = NULL;
		entry = grid_platform_find_actionstring_file(page, element, event_type);

		if (eve->cfg_default_flag){ // SEND BACK THE DEFAULT


			grid_ui_event_generate_actionstring(eve, targetstring);
			

			grid_platform_printf("DEFAULT: %s\r\n", targetstring);

		}
		else if (eve->action_string != NULL){

			grid_platform_printf("FOUND eve->action_string: %s\r\n", eve->action_string);

			strcpy(targetstring, eve->action_string);

		}
		else if (entry != NULL){


			uint32_t len = grid_platform_read_actionstring_file_contents(entry, targetstring);
			grid_platform_close_actionstring_file(entry);

		}
		else{
			grid_platform_printf("BIG PROBLEM, SENDING DEFAULT\r\n");

			grid_ui_event_generate_actionstring(eve, targetstring);
			
		}

	}		
	else{

		grid_platform_printf("!!!!! PAGE IS NOT ACTIVE\r\n");
		// use nvm_toc to find the configuration to be sent

		// file pointer
		void* entry = NULL;
		entry = grid_platform_find_actionstring_file(page, element, event_type);

		if (entry != NULL){
			
			uint32_t len = grid_platform_read_actionstring_file_contents(entry, targetstring);
			grid_platform_close_actionstring_file(entry);

		}
		else{
			//grid_platform_printf("NOT FOUND, Send default!\r\n");
			grid_ui_event_generate_actionstring(eve, targetstring);	
			//grid_ui_event_register_actionstring(eve, actionstring);	

		}

		// if no toc entry is found but page exists then send efault configuration

	}
				

	
}



void grid_ui_event_trigger(struct grid_ui_event* eve){

	if (eve != NULL){

		eve->trigger = GRID_UI_STATUS_TRIGGERED;
	}

}

void grid_ui_event_trigger_local(struct grid_ui_event* eve){

	if (eve != NULL){

		eve->trigger = GRID_UI_STATUS_TRIGGERED_LOCAL;
	}

}
void grid_ui_event_reset(struct grid_ui_event* eve){

	if (eve != NULL){
		eve->trigger = GRID_UI_STATUS_READY;
	}
}

uint8_t grid_ui_event_istriggered(struct grid_ui_event* eve){
		
	
	if (eve != NULL && eve->trigger == GRID_UI_STATUS_TRIGGERED){
					
		return 1;
				
	}
	else{
		
		return 0;
	}
				
}


uint8_t grid_ui_event_istriggered_local(struct grid_ui_event* eve){
		
		
	if (eve != NULL && eve->trigger == GRID_UI_STATUS_TRIGGERED_LOCAL){
					
		return 1;
				
	}
	else{
		
		return 0;
	}
			
}

uint16_t grid_ui_event_count_istriggered(struct grid_ui_model* ui){

	
	uint16_t count = 0;

	// UI STATE
		
	for (uint8_t j=0; j<ui->element_list_length; j++){
		
		for (uint8_t k=0; k<ui->element_list[j].event_list_length; k++){
			
			if (grid_ui_event_istriggered(&ui->element_list[j].event_list[k])){

				count++;

				
			}
		
			
		}
		
	}

	return count;

}
uint16_t grid_ui_event_count_istriggered_local(struct grid_ui_model* ui){

	uint16_t count = 0;

	// UI STATE
		
	for (uint8_t j=0; j<ui->element_list_length; j++){
		
		for (uint8_t k=0; k<ui->element_list[j].event_list_length; k++){
			
			if (grid_ui_event_istriggered_local(&ui->element_list[j].event_list[k])){

				count++;

				
			}
		
			
		}
		
	}
	return count;


}


struct grid_ui_element* grid_ui_element_find(struct grid_ui_model* ui, uint8_t element_number){

	if (element_number < ui->element_list_length){

		return &ui->element_list[element_number];
	}
	else{
		return NULL;
	}

}


void grid_ui_element_timer_set(struct grid_ui_element* ele, uint32_t duration){


	ele->timer_event_helper = duration;

}


void grid_ui_element_set_template_parameter(struct grid_ui_element* ele, uint8_t template_index, int32_t value){

	if (template_index < ele->template_parameter_list_length){

		ele->template_parameter_list[template_index] = value;
	}

}

int32_t grid_ui_element_get_template_parameter(struct grid_ui_element* ele, uint8_t template_index){

	if (template_index < ele->template_parameter_list_length){

		return ele->template_parameter_list[template_index];
	}	
	else{
		return 0;
	}

}



void grid_ui_element_potmeter_template_parameter_init(struct grid_ui_template_buffer* buf){

	uint8_t element_index = buf->parent->index;
	int32_t* template_parameter_list = buf->template_parameter_list;


	template_parameter_list[GRID_LUA_FNC_P_ELEMENT_INDEX_index]		= element_index;
	template_parameter_list[GRID_LUA_FNC_P_POTMETER_NUMBER_index] 	= element_index;
	template_parameter_list[GRID_LUA_FNC_P_POTMETER_VALUE_index] 	= 0;
	template_parameter_list[GRID_LUA_FNC_P_POTMETER_MIN_index] 		= 0;
	template_parameter_list[GRID_LUA_FNC_P_POTMETER_MAX_index] 		= 127;
	template_parameter_list[GRID_LUA_FNC_P_POTMETER_MODE_index] 	= 7;
	template_parameter_list[GRID_LUA_FNC_P_POTMETER_ELAPSED_index] 	= 0;
	template_parameter_list[GRID_LUA_FNC_P_POTMETER_STATE_index] 	= 0;

	// Load AIN value to VALUE register

	int32_t resolution = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MODE_index];

	if (resolution < 1){
		resolution = 1;
	}
	else if (resolution > 12){
		resolution = 12;
	}

	int32_t min = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MIN_index];
	int32_t max = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MAX_index];

	int32_t next = grid_ain_get_average_scaled(&grid_ain_state, element_index, 16, resolution, min, max);
	template_parameter_list[GRID_LUA_FNC_P_POTMETER_VALUE_index] = next;


	

}

void grid_ui_element_button_template_parameter_init(struct grid_ui_template_buffer* buf){
	
	uint8_t element_index = buf->parent->index;
	int32_t* template_parameter_list = buf->template_parameter_list;

	template_parameter_list[GRID_LUA_FNC_B_ELEMENT_INDEX_index] 	= element_index;
	template_parameter_list[GRID_LUA_FNC_B_BUTTON_NUMBER_index] 	= element_index;
	template_parameter_list[GRID_LUA_FNC_B_BUTTON_VALUE_index] 		= 0;
	template_parameter_list[GRID_LUA_FNC_B_BUTTON_MIN_index] 		= 0;
	template_parameter_list[GRID_LUA_FNC_B_BUTTON_MAX_index] 		= 127;
	template_parameter_list[GRID_LUA_FNC_B_BUTTON_MODE_index] 		= 0;
	template_parameter_list[GRID_LUA_FNC_B_BUTTON_ELAPSED_index] 	= 0;
	template_parameter_list[GRID_LUA_FNC_B_BUTTON_STATE_index] 		= 0;
}

void grid_ui_element_encoder_template_parameter_init(struct grid_ui_template_buffer* buf){

	//printf("template parameter init\r\n");

	uint8_t element_index = buf->parent->index;
	int32_t* template_parameter_list = buf->template_parameter_list;

	template_parameter_list[GRID_LUA_FNC_E_ELEMENT_INDEX_index] 	= element_index;
	template_parameter_list[GRID_LUA_FNC_E_BUTTON_NUMBER_index] 	= element_index;
	template_parameter_list[GRID_LUA_FNC_E_BUTTON_VALUE_index] 		= 0;
	template_parameter_list[GRID_LUA_FNC_E_BUTTON_MIN_index] 		= 0;
	template_parameter_list[GRID_LUA_FNC_E_BUTTON_MAX_index] 		= 127;
	template_parameter_list[GRID_LUA_FNC_E_BUTTON_MODE_index] 		= 0;
	template_parameter_list[GRID_LUA_FNC_E_BUTTON_ELAPSED_index] 	= 0;
	template_parameter_list[GRID_LUA_FNC_E_BUTTON_STATE_index] 		= 0;

	template_parameter_list[GRID_LUA_FNC_E_ENCODER_NUMBER_index] 	= element_index;
	template_parameter_list[GRID_LUA_FNC_E_ENCODER_VALUE_index] 	= 0;
	template_parameter_list[GRID_LUA_FNC_E_ENCODER_MIN_index] 		= 0;
	template_parameter_list[GRID_LUA_FNC_E_ENCODER_MAX_index] 		= 128 - 1;
	template_parameter_list[GRID_LUA_FNC_E_ENCODER_MODE_index] 		= 0;
	template_parameter_list[GRID_LUA_FNC_E_ENCODER_ELAPSED_index] 	= 0;
	template_parameter_list[GRID_LUA_FNC_E_ENCODER_STATE_index] 	= 64;
	template_parameter_list[GRID_LUA_FNC_E_ENCODER_VELOCITY_index] 	= 50;

}



void grid_ui_element_button_event_clear_cb(struct grid_ui_event* eve){


}

void grid_ui_element_button_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new){

	
	// for (uint8_t i=0; i<grid_ui_state.element_list_length; i++){

	// 	struct grid_ui_event* eve = NULL;

	// 	eve = grid_ui_event_find(&grid_ui_state.element_list[i], GRID_UI_EVENT_INIT);
	// 	grid_ui_event_trigger_local(eve);	

	// 	eve = grid_ui_event_find(&grid_ui_state.element_list[i], GRID_UI_EVENT_BC);
	// 	grid_ui_event_trigger_local(eve);	
	// }
}



void grid_ui_element_encoder_event_clear_cb(struct grid_ui_event* eve){


	int32_t* template_parameter_list = eve->parent->template_parameter_list;



	template_parameter_list[GRID_LUA_FNC_E_ENCODER_STATE_index] = 64;

	if (template_parameter_list[GRID_LUA_FNC_E_ENCODER_MODE_index] == 1){ // relative

		int32_t min = template_parameter_list[GRID_LUA_FNC_E_ENCODER_MIN_index];
		int32_t max = template_parameter_list[GRID_LUA_FNC_E_ENCODER_MAX_index];

		template_parameter_list[GRID_LUA_FNC_E_ENCODER_VALUE_index] = ((max+1)-min)/2;

	}	
	else if (template_parameter_list[GRID_LUA_FNC_E_ENCODER_MODE_index] == 2){

		template_parameter_list[GRID_LUA_FNC_E_ENCODER_VALUE_index] = 0;

	}
 
}

void grid_ui_element_encoder_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new){

			
	// for (uint8_t i = 0; i<16; i++)
	// {
		
	// 	struct grid_ui_event* eve = NULL;

	// 	eve = grid_ui_event_find(&grid_ui_state.element_list[i], GRID_UI_EVENT_INIT);
	// 	grid_ui_event_trigger_local(eve);	

	// 	eve = grid_ui_event_find(&grid_ui_state.element_list[i], GRID_UI_EVENT_EC);
	// 	grid_ui_event_trigger_local(eve);	
	// }

}



void grid_ui_element_potmeter_event_clear_cb(struct grid_ui_event* eve){

}

void grid_ui_element_potmeter_page_change_cb(struct grid_ui_element* ele, uint8_t page_old, uint8_t page_new){


	uint8_t element_index = ele->index;
	int32_t* template_parameter_list = ele->template_parameter_list;


	int32_t resolution = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MODE_index];

	if (resolution < 1){
		resolution = 1;
	}
	else if (resolution > 12){
		resolution = 12;
	}

	int32_t min = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MIN_index];
	int32_t max = template_parameter_list[GRID_LUA_FNC_P_POTMETER_MAX_index];

	int32_t next = grid_ain_get_average_scaled(&grid_ain_state, element_index, grid_platform_get_adc_bit_depth(), resolution, min, max); 
	template_parameter_list[GRID_LUA_FNC_P_POTMETER_VALUE_index] = next;
}



void grid_ui_bulk_pageread_init(struct grid_ui_model* ui, void (*success_cb)()){


	ui->read_success_callback = success_cb;


	ui->read_bulk_last_element = 0;
	ui->read_bulk_last_event = -1;

	ui->read_bulk_status = 1;
			
}



uint8_t grid_ui_bulk_pageread_is_in_progress(struct grid_ui_model* ui){

	return ui->read_bulk_status;

}



void grid_ui_bulk_pageread_next(struct grid_ui_model* ui){
	
	if (!grid_ui_bulk_pageread_is_in_progress(ui)){
		return;
	}

	if (!grid_platform_get_nvm_state()){
		return;
	}

	grid_lua_gc_collect(&grid_lua_state);


	uint8_t last_element_helper = ui->read_bulk_last_element;
	uint8_t last_event_helper = ui->read_bulk_last_event + 1;

	uint8_t firstrun = 1;
	


	//grid_platform_printf("BULK READ PAGE LOAD %d %d\r\n", last_element_helper, last_event_helper);

	for (uint8_t i=last_element_helper; i<ui->element_list_length; i++){

		struct grid_ui_element* ele = &ui->element_list[i];



		for (uint8_t j=0; j<ele->event_list_length; j++){

			if (firstrun){

				j = last_event_helper;
				firstrun = 0;
							
				if (j>=ele->event_list_length){ // to check last_event_helper in the first iteration
					break;
				}
			}			

			struct grid_ui_event* eve = &ele->event_list[j];

			// file pointer
			void* entry = NULL;
			
			entry = grid_platform_find_actionstring_file(ui->page_activepage, ele->index, eve->type);

			if (entry != NULL){
				
				//grid_platform_printf("Page Load: FOUND %d %d %d 0x%x (+%d)!\r\n", entry->page_id, entry->element_id, entry->event_type, entry->config_string_offset, entry->config_string_length);

				uint16_t length = grid_platform_get_actionstring_file_size(entry);

				if (length){
					char temp[GRID_PARAMETER_ACTIONSTRING_maxlength + 100] = {0};

					grid_platform_read_actionstring_file_contents(entry, temp);
					grid_platform_close_actionstring_file(entry);
					grid_ui_event_register_actionstring(eve, temp);
					
					// came from default so no need to store it in eve->action_string
					grid_ui_event_free_actionstring(eve);
					eve->cfg_changed_flag = 0; // clear changed flag
				}
				else{
					//grid_platform_printf("Page Load: NULL length\r\n");
					grid_platform_close_actionstring_file(entry);
				}
				
			}
			else{
				
				//grid_platform_printf("Page Load: NOT FOUND, Set default!\r\n");


				char temp[GRID_PARAMETER_ACTIONSTRING_maxlength + 100] = {0};



				grid_ui_event_generate_actionstring(eve, temp);
				grid_ui_event_register_actionstring(eve, temp);


				// came from TOC so no need to store it in eve->action_string
				grid_ui_event_free_actionstring(eve);
				eve->cfg_changed_flag = 0; // clear changed flag

			}


			if (eve->type == GRID_UI_EVENT_INIT){
				grid_ui_event_trigger_local(eve);
			}

			ui->read_bulk_last_element = i;
			ui->read_bulk_last_event = j;

			return;

		}

	}
	


	if (ui->read_success_callback != NULL){
		ui->read_success_callback();
		ui->read_success_callback = NULL;
	}

	ui->read_bulk_status = 0;


}





void grid_ui_bulk_pagestore_init(struct grid_ui_model* ui, void (*success_cb)()){

	ui->store_success_callback = success_cb;

	ui->store_bulk_status = 1;



}

uint8_t grid_ui_bulk_pagestore_is_in_progress(struct grid_ui_model* ui){

	return ui->store_bulk_status;
	
}

// DO THIS!!
void grid_ui_bulk_pagestore_next(struct grid_ui_model* ui){

 	if (!grid_ui_bulk_pagestore_is_in_progress(ui)){
		return;
	}

	if (!grid_platform_get_nvm_state()){
		return;
	}



	for (uint8_t i=0; i<ui->element_list_length; i++){

		struct grid_ui_element* ele = &ui->element_list[i];

		for (uint8_t j=0; j<ele->event_list_length; j++){

			struct grid_ui_event* eve = &ele->event_list[j];

			if (eve->cfg_changed_flag){
				
				if (eve->cfg_default_flag){

					void* entry = grid_platform_find_actionstring_file(ele->parent->page_activepage, ele->index, eve->type);
					
					if (entry != NULL){
						//grid_platform_printf("DEFAULT, FOUND - SO DESTROY! %d %d\r\n", ele->index, eve->index);

						
						grid_platform_delete_actionstring_file(entry);

					}
					else{

						//grid_platform_printf("DEFAULT BUT NOT FOUND! %d %d\r\n", ele->index, eve->index);
					}

					// its reverted to default, so no need to keep it in eve->action_string
					grid_ui_event_free_actionstring(eve);
	
				}
				else{
					
					//grid_platform_printf("NOT DEFAULT! %d %d\r\n", ele->index, eve->index);

					grid_platform_write_actionstring_file(ele->parent->page_activepage, ele->index, eve->type, eve->action_string, strlen(eve->action_string));

					// now its stored in TOC so no need to keep it in eve->action_string
					grid_ui_event_free_actionstring(eve);

				}

				eve->cfg_changed_flag = 0; // clear changed flag

				// after the first successful store quit from this function
				return;
			}

		}

	}

	// if every change was previously stored then execution will continue from here

	if (ui->store_success_callback != NULL){
		ui->store_success_callback();
		ui->store_success_callback = NULL;
	}

	ui->store_bulk_status = 0;
}





void grid_ui_bulk_pageclear_init(struct grid_ui_model* ui, void (*success_cb)()){

	ui->clear_success_callback = success_cb;


	ui->clear_bulk_status = 1;

}

uint8_t grid_ui_bulk_pageclear_is_in_progress(struct grid_ui_model* ui){

	return ui->clear_bulk_status;
	
}

// DO THIS!!
void grid_ui_bulk_pageclear_next(struct grid_ui_model* ui){

 	if (!grid_ui_bulk_pageclear_is_in_progress(ui)){
		return;
	}

	if (!grid_platform_get_nvm_state()){
		return;
	}

	grid_platform_clear_actionstring_files_from_page(ui->page_activepage);

	
	if (ui->clear_success_callback != NULL){
		ui->clear_success_callback();
		ui->clear_success_callback = NULL;
	}

	ui->clear_bulk_status = 0;



		
}





void grid_ui_bulk_nvmerase_init(struct grid_ui_model* ui, void (*success_cb)()){


	ui->erase_success_callback = success_cb;



	grid_platform_delete_actionstring_files_all();



	ui->erase_bulk_status = 1;

}

uint8_t grid_ui_bulk_nvmerase_is_in_progress(struct grid_ui_model* ui){

	return ui->erase_bulk_status;
	
}


void grid_ui_bulk_nvmerase_next(struct grid_ui_model* ui){

	if (!grid_ui_bulk_nvmerase_is_in_progress(ui)){
		return;
	}

	if (!grid_platform_get_nvm_state()){
		return;
	}

	// flash_erase takes 32 ms so no need to implement timeout here, only erase one block at a time!

	if(grid_platform_erase_nvm_next()){

		// there is still stuff to be erased
	}
	else{ // done with the erase

		// call success callback
		if (ui->erase_success_callback != NULL){

			ui->erase_success_callback();
			ui->erase_success_callback = NULL;
		}



		ui->erase_bulk_status = 0;


	}

	
}


uint8_t grid_ui_bluk_anything_is_in_progress(struct grid_ui_model* ui){

	if (grid_ui_bulk_nvmerase_is_in_progress(ui)){
		return 1;
	}
	
	if (grid_ui_bulk_pageclear_is_in_progress(ui)){
		return 1;
	}

	if (grid_ui_bulk_pageread_is_in_progress(ui)){
		return 1;
	}
	
	if (grid_ui_bulk_pagestore_is_in_progress(ui)){
		return 1;
	}

	// if (grid_d51_nvm_ui_bulk_nvmdefrag_is_in_progress(nvm, ui)){
	// 	return 1;
	// }

	// Return 0 if nothing is in progress
	return 0;


}






void grid_port_process_ui_local_UNSAFE(struct grid_ui_model* ui){

		
	// Prepare packet header LOCAL
	struct grid_msg_packet message_local;
	grid_msg_packet_init(&grid_msg_state, &message_local, GRID_PARAMETER_DEFAULT_POSITION, GRID_PARAMETER_DEFAULT_POSITION);
	char payload_local[GRID_PARAMETER_PACKET_maxlength] = {0};				
	uint32_t offset_local=0;		

	// Prepare packet header GLOBAL
	struct grid_msg_packet message_global;
	grid_msg_packet_init(&grid_msg_state, &message_global, GRID_PARAMETER_DEFAULT_POSITION, GRID_PARAMETER_DEFAULT_POSITION);
	char payload_global[GRID_PARAMETER_PACKET_maxlength] = {0};				
	uint32_t offset_global=0;
	
	
	
	// UI STATE

	for (uint8_t j=0; j<ui->element_list_length; j++){
		
		for (uint8_t k=0; k<ui->element_list[j].event_list_length; k++){
		
			if (offset_local>GRID_PARAMETER_PACKET_marign || offset_global>GRID_PARAMETER_PACKET_marign){
				continue;
			}		
			else{
				
				
				if (grid_ui_event_istriggered_local(&ui->element_list[j].event_list[k])){
					
					offset_local += grid_ui_event_render_action(&ui->element_list[j].event_list[k], &payload_local[offset_local]);
					grid_ui_event_reset(&ui->element_list[j].event_list[k]);


					// automatically report elementname after config
					if (j<ui->element_list_length-1){
						

							uint8_t number = j;
							char command[26] = {0};

							sprintf(command, "gens(%d,ele[%d]:gen())", j, j);

							// lua get element name
							grid_lua_clear_stdo(&grid_lua_state);
							grid_lua_dostring(&grid_lua_state, command);
							strcat(payload_global, grid_lua_get_output_string(&grid_lua_state));
							grid_lua_clear_stdo(&grid_lua_state);

					}
					

				
				}
				
			}
		
		}
		
		

		
	}

	if (strlen(payload_global)>0){

		grid_msg_packet_body_append_text(&message_global, payload_global);
		grid_msg_packet_close(&grid_msg_state, &message_global);
		grid_port_packet_send_everywhere(&message_global);
	}

	
	grid_msg_packet_body_append_text(&message_local, payload_local);


	grid_msg_packet_close(&grid_msg_state, &message_local);
		
	uint32_t message_length = grid_msg_packet_get_length(&message_local);
		
	// Put the packet into the UI_TX buffer
	if (grid_buffer_write_init(&ui->port->tx_buffer, message_length)){
			
		for(uint32_t i = 0; i<message_length; i++){
				
			grid_buffer_write_character(&ui->port->tx_buffer, grid_msg_packet_send_char_by_char(&message_local, i));
		}
			
		grid_buffer_write_acknowledge(&ui->port->tx_buffer);
		
// 			uint8_t debug_string[200] = {0};
// 			sprintf(debug_string, "Space: RX: %d/%d  TX: %d/%d", grid_buffer_get_space(&ui->port->rx_buffer), GRID_BUFFER_SIZE, grid_buffer_get_space(&ui->port->tx_buffer), GRID_BUFFER_SIZE);
// 			grid_port_debug_print_text(debug_string);


	}
	else{
		// LOG UNABLE TO WRITE EVENT
	}
		





}


void grid_port_process_ui_UNSAFE(struct grid_ui_model* ui){
	

	struct grid_msg_packet message;
	grid_msg_packet_init(&grid_msg_state, &message, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);
	

	for (uint8_t j=0; j<ui->element_list_length; j++){
	
		for (uint8_t k=0; k<ui->element_list[j].event_list_length; k++){ //j=1 because init is local
		
			if (grid_msg_packet_get_length(&message)>GRID_PARAMETER_PACKET_marign){
				continue;
			}		
			else{
							
				if (grid_ui_event_istriggered(&ui->element_list[j].event_list[k])){

					uint32_t offset = grid_msg_packet_body_get_length(&message); 

					message.body_length += grid_ui_event_render_event(&ui->element_list[j].event_list[k], &message.body[offset]);
				
					offset = grid_msg_packet_body_get_length(&message); 

					
					message.body_length += grid_ui_event_render_action(&ui->element_list[j].event_list[k], &message.body[offset]);
					grid_ui_event_reset(&ui->element_list[j].event_list[k]);
					
					

				}
				
			}
		
		}
	}
	

	

	grid_msg_packet_close(&grid_msg_state, &message);
	uint32_t length = grid_msg_packet_get_length(&message);
	

	// Put the packet into the UI_RX buffer
	if (grid_buffer_write_init(&ui->port->rx_buffer, length)){

		for(uint16_t i = 0; i<length; i++){
			
			grid_buffer_write_character(&ui->port->rx_buffer, grid_msg_packet_send_char_by_char(&message, i));
		}
		
		grid_buffer_write_acknowledge(&ui->port->rx_buffer);

		
	}
	else{
		// LOG UNABLE TO WRITE EVENT
	}

	

	// LEDREPORT
	if (grid_protocol_led_change_report_length(&grid_led_state) && grid_sys_get_editor_connected_state(&grid_sys_state)){

		struct grid_msg_packet response;
								
		grid_msg_packet_init(&grid_msg_state, &response, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

		char response_payload[300] = {0};
		uint16_t len = 0;
		snprintf(response_payload, 299, GRID_CLASS_LEDPREVIEW_frame_start);
		len += strlen(&response_payload[len]);

		uint16_t report_length = grid_protocol_led_change_report_generate(&grid_led_state, -1, &response_payload[len]);

		len += strlen(&response_payload[len]);

		grid_msg_packet_body_append_text(&response, response_payload);


		grid_msg_packet_body_append_printf(&response, GRID_CLASS_LEDPREVIEW_frame_end);

		grid_msg_packet_body_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);													
		grid_msg_packet_body_set_parameter(&response, 0, GRID_CLASS_LEDPREVIEW_LENGTH_offset, GRID_CLASS_LEDPREVIEW_LENGTH_length, report_length);
		
		grid_msg_packet_close(&grid_msg_state, &response);
		grid_port_packet_send_everywhere(&response);

	}

	
}


