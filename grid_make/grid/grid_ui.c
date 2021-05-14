#include "grid_ui.h"



void grid_port_process_ui(struct grid_ui_model* ui, struct grid_port* por){
	
	// Priorities: Always process local, try to process direct, broadcast messages are last. 
	
	
	uint8_t ui_available = 0;
	uint8_t message_local_action_available = 0;

	// UI STATE
		
	for (uint8_t j=0; j<grid_ui_state.element_list_length; j++){
		
		for (uint8_t k=0; k<grid_ui_state.element_list[j].event_list_length; k++){
			
			if (grid_ui_event_istriggered(&grid_ui_state.element_list[j].event_list[k])){

				ui_available++;

				
			}
			
			if (grid_ui_event_istriggered_local(&grid_ui_state.element_list[j].event_list[k])){
				
				message_local_action_available++;

				
			}
			
		}
		
	}
	
	
	//NEW PING
	struct grid_port* port[4] = {&GRID_PORT_N, &GRID_PORT_E, &GRID_PORT_S, &GRID_PORT_W};
	
	for (uint8_t k = 0; k<4; k++){
		
		if (port[k]->ping_flag == 1){
		
			if (grid_buffer_write_init(&port[k]->tx_buffer, port[k]->ping_packet_length)){
				//Success
				for(uint32_t i = 0; i<port[k]->ping_packet_length; i++){
					grid_buffer_write_character(&port[k]->tx_buffer, port[k]->ping_packet[i]);
				}
				grid_buffer_write_acknowledge(&port[k]->tx_buffer);
			}
			port[k]->ping_flag = 0;
		}		
			
	}			

	grid_d51_task_next(ui->task);
		

	
	//LOCAL MESSAGES
	if (message_local_action_available){
		
	
		struct grid_msg message;
		grid_msg_init(&message);
		grid_msg_init_header(&message, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_ROTATION);
			
			
		// Prepare packet header
		uint8_t payload[GRID_PARAMETER_PACKET_maxlength] = {0};				
		uint32_t offset=0;
		
		
		
		// UI STATE

		for (uint8_t j=0; j<grid_ui_state.element_list_length; j++){
			
			for (uint8_t k=0; k<grid_ui_state.element_list[j].event_list_length; k++){
			
				if (offset>GRID_PARAMETER_PACKET_marign){
					continue;
				}		
				else{
					
					CRITICAL_SECTION_ENTER()
					if (grid_ui_event_istriggered_local(&grid_ui_state.element_list[j].event_list[k])){
						
						offset += grid_ui_event_render_action(&grid_ui_state.element_list[j].event_list[k], &payload[offset]);
						grid_ui_event_reset(&grid_ui_state.element_list[j].event_list[k]);
					
					}
					CRITICAL_SECTION_LEAVE()
					
				}
			
			}
			
			

			
		}
		
		
		
		grid_msg_body_append_text(&message, payload);
		grid_msg_packet_close(&message);
			
		uint32_t message_length = grid_msg_packet_get_length(&message);
			
		// Put the packet into the UI_TX buffer
		if (grid_buffer_write_init(&GRID_PORT_U.tx_buffer, message_length)){
				
			for(uint32_t i = 0; i<message_length; i++){
					
				grid_buffer_write_character(&GRID_PORT_U.tx_buffer, grid_msg_packet_send_char(&message, i));
			}
				
			grid_buffer_write_acknowledge(&GRID_PORT_U.tx_buffer);
			
// 			uint8_t debug_string[200] = {0};
// 			sprintf(debug_string, "Space: RX: %d/%d  TX: %d/%d", grid_buffer_get_space(&GRID_PORT_U.rx_buffer), GRID_BUFFER_SIZE, grid_buffer_get_space(&GRID_PORT_U.tx_buffer), GRID_BUFFER_SIZE);
// 			grid_debug_print_text(debug_string);


		}
		else{
			// LOG UNABLE TO WRITE EVENT
		}
			
	}
	
	
	
	
	// Bandwidth Limiter for Broadcast messages
	
	if (por->cooldown > 0){
		por->cooldown--;
	}
	
	
	if (por->cooldown > 10){
		// dummy calls to make sure subtask after return are counted properly
		grid_d51_task_next(ui->task);		
		grid_d51_task_next(ui->task);		
		grid_d51_task_next(ui->task);	
		return;
	}

	
	
	grid_d51_task_next(ui->task);	

	struct grid_msg message;
	grid_msg_init(&message);
	grid_msg_init_header(&message, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_ROTATION);
	
	
	grid_d51_task_next(ui->task);
	// BROADCAST MESSAGES : UI STATE
	if (ui_available){
		
		for (uint8_t j=0; j<grid_ui_state.element_list_length; j++){
		
			for (uint8_t k=0; k<grid_ui_state.element_list[j].event_list_length; k++){ //j=1 because init is local
			
				if (grid_msg_packet_get_length(&message)>GRID_PARAMETER_PACKET_marign){
					continue;
				}		
				else{
								
					if (grid_ui_event_istriggered(&grid_ui_state.element_list[j].event_list[k])){

						uint32_t offset = grid_msg_body_get_length(&message); 

						message.body_length += grid_ui_event_render_event(&grid_ui_state.element_list[j].event_list[k], &message.body[offset]);
					
						offset = grid_msg_body_get_length(&message); 

						CRITICAL_SECTION_ENTER()
						message.body_length += grid_ui_event_render_action(&grid_ui_state.element_list[j].event_list[k], &message.body[offset]);
						grid_ui_event_reset(&grid_ui_state.element_list[j].event_list[k]);
						CRITICAL_SECTION_LEAVE()
						
					}
					
				}
			
			}
		
		}
		
	}

	grid_d51_task_next(ui->task);	
	if (ui_available){

		
		//por->cooldown += (2+por->cooldown/2);
		por->cooldown += 10;
		//por->cooldown = 3;
		
		

		grid_msg_packet_close(&message);
		uint32_t length = grid_msg_packet_get_length(&message);
		

		// Put the packet into the UI_RX buffer
		if (grid_buffer_write_init(&GRID_PORT_U.rx_buffer, length)){
	
			for(uint16_t i = 0; i<length; i++){
				
				grid_buffer_write_character(&GRID_PORT_U.rx_buffer, grid_msg_packet_send_char(&message, i));
			}
			
			grid_buffer_write_acknowledge(&GRID_PORT_U.rx_buffer);

			
		}
		else{
			// LOG UNABLE TO WRITE EVENT
		}
		
		
		
	
	}
	
}


void grid_ui_model_init(struct grid_ui_model* mod, uint8_t element_list_length){
	
	mod->status = GRID_UI_STATUS_INITIALIZED;

	mod->page_activepage = 0;

	mod->element_list_length = element_list_length;	

	printf("UI MODEL INIT: %d\r\n", element_list_length);
	mod->element_list = malloc(element_list_length*sizeof(struct grid_ui_element));
	
}

struct grid_ui_template_buffer* grid_ui_template_buffer_create(struct grid_ui_element* ele){
		
	struct grid_ui_template_buffer* this = NULL;
	struct grid_ui_template_buffer* prev = ele->template_buffer_list_head;

	this = malloc(sizeof(struct grid_ui_template_buffer));

	if (this == NULL){
		printf("error.ui.MallocFailed\r\n");
	}
	else{

		this->status = 0;
		this->next = NULL;
		this->page_number = 0;
		this->parent = ele;

		
		this->template_parameter_list = malloc(ele->template_parameter_list_length*sizeof(int32_t));

		if (this->template_parameter_list == NULL){
			printf("error.ui.MallocFailed\r\n");
		}
		else{

			if (ele->template_initializer!=NULL){
				ele->template_initializer(this);
			}

			//printf("LIST\r\n");

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

	
	if (element_type == GRID_UI_ELEMENT_SYSTEM){
		
		ele->event_list_length = 2;
		
		ele->event_list = malloc(ele->event_list_length*sizeof(struct grid_ui_event));
		grid_ui_event_init(ele, 0, GRID_UI_EVENT_INIT); // Element Initialization Event
		grid_ui_event_init(ele, 1, GRID_UI_EVENT_MAPMODE_CHANGE); // Mapmode change

		ele->template_initializer = NULL;
		ele->template_parameter_list_length = 0;
		
	}
	else if (element_type == GRID_UI_ELEMENT_POTENTIOMETER){
		
		ele->event_list_length = 2;
		
		ele->event_list = malloc(ele->event_list_length*sizeof(struct grid_ui_event));
		
		grid_ui_event_init(ele, 0, GRID_UI_EVENT_INIT); // Element Initialization Event
		grid_ui_event_init(ele, 1, GRID_UI_EVENT_AC); // Absolute Value Change (7bit)

		ele->template_initializer = &grid_element_potmeter_template_parameter_init;
		ele->template_parameter_list_length = GRID_LUA_FNC_P_LIST_length;
		
		ele->event_clear_cb = &grid_element_potmeter_event_clear_cb;
		ele->page_change_cb = &grid_element_potmeter_page_change_cb;

	}
	else if (element_type == GRID_UI_ELEMENT_BUTTON){
		
		ele->event_list_length = 2;
		
		ele->event_list = malloc(ele->event_list_length*sizeof(struct grid_ui_event));
		
		grid_ui_event_init(ele, 0, GRID_UI_EVENT_INIT); // Element Initialization Event
		grid_ui_event_init(ele, 1, GRID_UI_EVENT_BC);	// Button Change

		ele->template_initializer = &grid_element_button_template_parameter_init;
		ele->template_parameter_list_length = GRID_LUA_FNC_B_LIST_length;

		ele->event_clear_cb = &grid_element_button_event_clear_cb;
		ele->page_change_cb = &grid_element_button_page_change_cb;

	}
	else if (element_type == GRID_UI_ELEMENT_ENCODER){
		
		ele->event_list_length = 3;
		
		ele->event_list = malloc(ele->event_list_length*sizeof(struct grid_ui_event));
		
		grid_ui_event_init(ele, 0, GRID_UI_EVENT_INIT); // Element Initialization Event
		grid_ui_event_init(ele, 1, GRID_UI_EVENT_EC);	// Encoder Change
		grid_ui_event_init(ele, 2, GRID_UI_EVENT_BC);	// Button Change

		ele->template_initializer = &grid_element_encoder_template_parameter_init;
		ele->template_parameter_list_length = GRID_LUA_FNC_E_LIST_length;
		
		ele->event_clear_cb = &grid_element_encoder_event_clear_cb;
		ele->page_change_cb = &grid_element_encoder_page_change_cb;

	}
	else{
		//UNKNOWN ELEMENT TYPE
		printf("error.unknown_element_type\r\n");
		ele->template_initializer = NULL;
	}

	if (ele->template_initializer != NULL){

		struct grid_ui_template_buffer* buf = grid_ui_template_buffer_create(ele);

		if (buf != NULL){

			ele->template_parameter_list = buf->template_parameter_list;
		}
		else{

		}

	}
	
}

void grid_ui_event_init(struct grid_ui_element* parent, uint8_t index, enum grid_ui_event_t event_type){
	

	struct grid_ui_event* eve = &parent->event_list[index];
	eve->parent = parent;
	eve->index = index;

	eve->cfg_changed_flag = 0;
	
	eve->status = GRID_UI_STATUS_INITIALIZED;
	
	eve->type   = event_type;	
	eve->status = GRID_UI_STATUS_READY;

	

	// Initializing Action String
	for (uint32_t i=0; i<GRID_UI_ACTION_STRING_maxlength; i++){
		eve->action_string[i] = 0;
	}		

	uint8_t actionstring[GRID_UI_ACTION_STRING_maxlength] = {0};
	grid_ui_event_generate_actionstring(eve, actionstring);	
	grid_ui_event_register_actionstring(eve, actionstring);


	eve->cfg_changed_flag = 0; // clear changed flag
	
	eve->cfg_changed_flag = 0;
	eve->cfg_default_flag = 1;
	eve->cfg_flashempty_flag = 1;

}


uint8_t grid_ui_recall_event_configuration(struct grid_ui_model* ui, struct grid_nvm_model* nvm, uint8_t page, uint8_t element, enum grid_ui_event_t event_type){
	
	printf("RECALL!!! \r\n");

	
	struct grid_msg message;

	grid_msg_init(&message);
	grid_msg_init_header(&message, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_ROTATION);


	uint8_t payload[GRID_PARAMETER_PACKET_maxlength] = {0};
	uint8_t payload_length = 0;
	uint32_t offset = 0;

	struct grid_ui_element* ele = &ui->element_list[element];

	struct grid_ui_event* eve = grid_ui_event_find(ele, event_type);

	if (eve != NULL){

		// Event actually exists

		sprintf(payload, GRID_CLASS_CONFIG_frame_start);
		grid_msg_body_append_text(&message, payload);

		grid_msg_text_set_parameter(&message, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);
		
		
		grid_msg_text_set_parameter(&message, 0, GRID_CLASS_CONFIG_VERSIONMAJOR_offset, GRID_CLASS_CONFIG_VERSIONMAJOR_length, GRID_PROTOCOL_VERSION_MAJOR);
		grid_msg_text_set_parameter(&message, 0, GRID_CLASS_CONFIG_VERSIONMINOR_offset, GRID_CLASS_CONFIG_VERSIONMINOR_length, GRID_PROTOCOL_VERSION_MINOR);
		grid_msg_text_set_parameter(&message, 0, GRID_CLASS_CONFIG_VERSIONPATCH_offset, GRID_CLASS_CONFIG_VERSIONPATCH_length, GRID_PROTOCOL_VERSION_PATCH);
				
		grid_msg_text_set_parameter(&message, 0, GRID_CLASS_CONFIG_PAGENUMBER_offset, GRID_CLASS_CONFIG_PAGENUMBER_length, page);
		grid_msg_text_set_parameter(&message, 0, GRID_CLASS_CONFIG_ELEMENTNUMBER_offset, GRID_CLASS_CONFIG_EVENTTYPE_length, element);
		grid_msg_text_set_parameter(&message, 0, GRID_CLASS_CONFIG_EVENTTYPE_offset, GRID_CLASS_CONFIG_EVENTTYPE_length, event_type);
		grid_msg_text_set_parameter(&message, 0, GRID_CLASS_CONFIG_ACTIONLENGTH_offset, GRID_CLASS_CONFIG_ACTIONLENGTH_length, 0);

		if (ui->page_activepage == page){
			// currently active page needs to be sent
			grid_msg_body_append_text(&message, eve->action_string);
			grid_msg_text_set_parameter(&message, 0, GRID_CLASS_CONFIG_ACTIONLENGTH_offset, GRID_CLASS_CONFIG_ACTIONLENGTH_length, strlen(eve->action_string));		
		
		}		
		else{

			// use nvm_toc to find the configuration to be sent

			struct grid_nvm_toc_entry* entry = NULL;
			entry = grid_nvm_toc_entry_find(&grid_nvm_state, page, element, event_type);

			if (entry != NULL){
				
				printf("FOUND %d %d %d 0x%x (+%d)!\r\n", entry->page_id, entry->element_id, entry->event_type, entry->config_string_offset, entry->config_string_length);

				uint8_t buffer[entry->config_string_length+10];

				uint32_t len = grid_nvm_toc_generate_actionstring(nvm, entry, buffer);

				// reset body pointer because cfg in nvm already has the config header
				message.body_length = 0;
				grid_msg_body_append_text(&message, buffer);

				grid_msg_text_set_parameter(&message, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);
			
			}
			else{
				printf("NOT FOUND, Send default!\r\n");
				uint8_t actionstring[GRID_UI_ACTION_STRING_maxlength] = {0};
				grid_ui_event_generate_actionstring(eve, actionstring);	
				//grid_ui_event_register_actionstring(eve, actionstring);	

				grid_msg_body_append_text(&message, actionstring);
				grid_msg_text_set_parameter(&message, 0, GRID_CLASS_CONFIG_ACTIONLENGTH_offset, GRID_CLASS_CONFIG_ACTIONLENGTH_length, strlen(eve->action_string));

			}

			// if no toc entry is found but page exists then send efault configuration

		}
	
		sprintf(payload, GRID_CLASS_CONFIG_frame_end);
		payload_length = strlen(payload);

		grid_msg_body_append_text(&message, payload);

		printf("CFG: %s\r\n", message.body);
		grid_msg_packet_close(&message);
		grid_msg_packet_send_everywhere(&message);
	}
	else{

		printf("warning."__FILE__".event does not exist!\r\n");
	}
	
}

uint8_t grid_ui_page_load(struct grid_ui_model* ui, struct grid_nvm_model* nvm, uint8_t page){

	uint8_t oldpage = ui->page_activepage;
	ui->page_activepage = page;
	// Call the page_change callback

	printf("LOAD PAGE: %d\r\n", page);

	for (uint8_t i = 0; i < grid_ui_state.element_list_length; i++)
	{	

		struct grid_ui_element* ele = &grid_ui_state.element_list[i];

		uint8_t template_buffer_length = grid_ui_template_buffer_list_length(ele);

		if (i==0) printf("TB LEN: %d\r\n", template_buffer_length);
		if (template_buffer_length < page+1){

			grid_ui_template_buffer_create(ele);

			if (i==0) printf("CREATE NEW, LEN: %d\r\n", grid_ui_template_buffer_list_length(ele));
			
		}


		struct grid_ui_template_buffer* buf =  grid_ui_template_buffer_find(ele, page);
		
		if (buf == NULL){

			printf("error.template buffer is invalid\r\n");

		}
		else{

			// load the template parameter list
			ele->template_parameter_list = buf->template_parameter_list;

		}


		if (ele->page_change_cb != NULL){

			ele->page_change_cb(oldpage, page);
		}

	}

	grid_nvm_ui_bulk_read_init(nvm, ui);
	
}


void grid_ui_event_register_actionstring(struct grid_ui_event* eve, uint8_t* action_string){
		

	if (strlen(action_string) == 0){

		printf("NULLSTRING\r\n");
		return;

	}


	if (eve->type == GRID_UI_EVENT_EC){

		uint8_t temp[GRID_UI_ACTION_STRING_maxlength] = {0};
		uint32_t len = strlen(action_string);
		action_string[len-3] = '\0';
		sprintf(temp, "ele[%d]."GRID_LUA_FNC_E_ACTION_ENCODERCHANGE_short" = function (a) local this = ele[%d] %s end", eve->parent->index, eve->parent->index, &action_string[6]);
		action_string[len-3] = ' ';

		grid_lua_dostring(&grid_lua_state, temp);

		sprintf(eve->action_string, action_string);
	}
	else if (eve->type == GRID_UI_EVENT_BC){

		uint8_t temp[GRID_UI_ACTION_STRING_maxlength] = {0};
		uint32_t len = strlen(action_string);
		action_string[len-3] = '\0';
		sprintf(temp, "ele[%d]."GRID_LUA_FNC_E_ACTION_BUTTONCHANGE_short" = function (a) local this = ele[%d] %s end", eve->parent->index, eve->parent->index, &action_string[6]);
		action_string[len-3] = ' ';

		grid_lua_dostring(&grid_lua_state, temp);

		sprintf(eve->action_string, action_string);
	}
	else if (eve->type == GRID_UI_EVENT_INIT){

		uint8_t temp[GRID_UI_ACTION_STRING_maxlength] = {0};

		sprintf(eve->action_string, action_string);
	}
	else{

		sprintf(eve->action_string, action_string);
		
		

	}


	eve->cfg_changed_flag = 1;
	//printf("action: %s\r\n", eve->action_string);

	//grid_lua_debug_memory_stats(&grid_lua_state, "R.A.S.");
	lua_gc(grid_lua_state.L, LUA_GCCOLLECT);
	
}


void grid_ui_event_generate_actionstring(struct grid_ui_event* eve, uint8_t* targetstring){
	
	if (eve->parent->type == GRID_UI_ELEMENT_SYSTEM){
				
		switch(eve->type){
			case GRID_UI_EVENT_INIT:	break;
			case GRID_UI_EVENT_MAPMODE_CHANGE:		sprintf(targetstring, GRID_ACTIONSTRING_MAPMODE_CHANGE);			break;
		}
		
	}
	else if (eve->parent->type == GRID_UI_ELEMENT_BUTTON){
				
		switch(eve->type){
			case GRID_UI_EVENT_INIT:	sprintf(targetstring, GRID_ACTIONSTRING_INIT_BUT);		break;
			case GRID_UI_EVENT_BC:		sprintf(targetstring, GRID_ACTIONSTRING_BC);			break;
		}
		
	}
	else if (eve->parent->type == GRID_UI_ELEMENT_POTENTIOMETER){
		
		switch(eve->type){
			case GRID_UI_EVENT_INIT:	sprintf(targetstring, GRID_ACTIONSTRING_INIT_BUT);		break;
			case GRID_UI_EVENT_AC:		sprintf(targetstring, GRID_ACTIONSTRING_AC);			break;
		}
		
	}
	else if (eve->parent->type == GRID_UI_ELEMENT_ENCODER){
		
		switch(eve->type){
			case GRID_UI_EVENT_INIT:        sprintf(targetstring, GRID_ACTIONSTRING_INIT_ENC);	break;
			case GRID_UI_EVENT_EC:        	sprintf(targetstring, GRID_ACTIONSTRING_EC);		break;
			case GRID_UI_EVENT_BC:			sprintf(targetstring, GRID_ACTIONSTRING_BC);		break;
		}
			
	}
	
	
}



struct grid_ui_event* grid_ui_event_find(struct grid_ui_element* ele, enum grid_ui_event_t event_type){

	uint8_t event_index = 255;
		
	for(uint8_t i=0; i<ele->event_list_length; i++){
		if (ele->event_list[i].type == event_type){
			return &ele->event_list[i];
		}
	}

		
		
	return NULL;
	
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

uint32_t grid_ui_event_render_event(struct grid_ui_event* eve, uint8_t* target_string){

	uint8_t page = eve->parent->parent->page_activepage;
	uint8_t element = eve->parent->index;
	uint8_t event = eve->type;
	uint8_t param = 7;

	sprintf(target_string, GRID_CLASS_EVENT_frame);

	grid_msg_set_parameter(target_string, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code, NULL);

	grid_msg_set_parameter(target_string, GRID_CLASS_EVENT_PAGENUMBER_offset, GRID_CLASS_EVENT_PAGENUMBER_length, page, NULL);
	grid_msg_set_parameter(target_string, GRID_CLASS_EVENT_ELEMENTNUMBER_offset, GRID_CLASS_EVENT_ELEMENTNUMBER_length, element, NULL);
	grid_msg_set_parameter(target_string, GRID_CLASS_EVENT_EVENTTYPE_offset, GRID_CLASS_EVENT_EVENTTYPE_length, event, NULL);
	grid_msg_set_parameter(target_string, GRID_CLASS_EVENT_EVENTPARAM_offset, GRID_CLASS_EVENT_EVENTPARAM_length, param, NULL);

	return strlen(target_string);

}

uint32_t grid_ui_event_render_action(struct grid_ui_event* eve, uint8_t* target_string){

	
	uint8_t temp[500] = {0};

	uint32_t i=0;
	

	if (eve->type == GRID_UI_EVENT_EC){

		sprintf(temp, "<?lua ele[%d]."GRID_LUA_FNC_E_ACTION_ENCODERCHANGE_short"() ?>", eve->parent->index);

	}
	else if (eve->type == GRID_UI_EVENT_BC){

		sprintf(temp, "<?lua ele[%d]."GRID_LUA_FNC_E_ACTION_BUTTONCHANGE_short"() ?>", eve->parent->index);

	}
	else if (eve->type == GRID_UI_EVENT_INIT){

		uint32_t len = strlen(eve->action_string);	
		eve->action_string[len-3] = '\0';	
		sprintf(temp, "<?lua local this = ele[%d] %s ?>", eve->parent->index, &eve->action_string[6]);
		eve->action_string[len-3] = ' ';

	}
	else{

		strcpy(temp, eve->action_string);

	}



	// new php style implementation
	uint32_t code_start = 0;
	uint32_t code_end = 0;
	uint32_t code_length = 0;
	uint32_t code_type = 0;  // 0: nocode, 1: expr, 2: lua



	uint32_t total_substituted_length = 0;


	// printf("ACTION: %s\r\n", eve->action_string);

	uint32_t length = strlen(temp);

	for(i=0; i<length ; i++){

		target_string[i-total_substituted_length] = temp[i];



		if (0 == strncmp(&temp[i], "<?expr ", 7)){

			//printf("<?expr \r\n");
			code_start = i;
			code_end = i;
			code_type = 1; // 1=expr


		}
		else if (0 == strncmp(&temp[i], "<?lua ", 6)){

			//printf("<?lua \r\n");
			code_start = i;
			code_end = i;
			code_type = 2; // 2=lua


		}
		else if (0 == strncmp(&temp[i], " ?>", 3)){

			code_end = i + 3; // +3 because  ?>
			//printf(" ?>\r\n");

			if (code_type == 2){ //LUA
				
				temp[i] = 0; // terminating zero for lua dostring

				uint32_t cycles[5] = {0};

				cycles[0] = grid_d51_dwt_cycles_read();

							
				cycles[1] = grid_d51_dwt_cycles_read();

				// printf("Render Actionstring: %s\r\n", &temp[code_start+6]);
				grid_lua_dostring(&grid_lua_state, &temp[code_start+6]); // +6 is length of "<?lua "
					
				cycles[2] = grid_d51_dwt_cycles_read();

				uint32_t code_stdo_length = strlen(grid_lua_state.stdo);

				temp[i] = ' '; // reverting terminating zero to space

				i+= 3-1; // +3 because  ?> -1 because i++
				code_length = code_end - code_start;

				strcpy(&target_string[code_start-total_substituted_length], grid_lua_state.stdo);

				
				uint8_t errorlen = 0;

				if (strlen(grid_lua_state.stde)){

					
					printf(grid_lua_state.stde);

					uint8_t errorbuffer[100] = {0};

					sprintf(errorbuffer, GRID_CLASS_DEBUGTEXT_frame_start);
					strcat(errorbuffer, grid_lua_state.stde);
					sprintf(&errorbuffer[strlen(errorbuffer)], GRID_CLASS_DEBUGTEXT_frame_end);
					
					errorlen = strlen(errorbuffer);

					strcpy(&target_string[code_start-total_substituted_length+code_stdo_length], errorbuffer);

					grid_lua_clear_stde(&grid_lua_state);

				}


				total_substituted_length += code_length - code_stdo_length - errorlen;


				cycles[3] = grid_d51_dwt_cycles_read();



				//printf("Lua: %s \r\nTime [us]: %d %d %d\r\n", grid_lua_state.stdo, (cycles[1]-cycles[0])/120, (cycles[2]-cycles[1])/120, (cycles[3]-cycles[2])/120);
				//grid_lua_debug_memory_stats(&grid_lua_state, "Ui");
				grid_lua_clear_stdo(&grid_lua_state);

			}
			else if(code_type == 1){ // expr

				temp[i] = 0; // terminating zero for strlen
				grid_expr_set_current_event(&grid_expr_state, eve);


				uint32_t cycles[5] = {0};

				cycles[0] = grid_d51_dwt_cycles_read();

				grid_expr_evaluate(&grid_expr_state, &temp[code_start+7], strlen( &temp[code_start+7])); // -2 to not include {

				cycles[1] = grid_d51_dwt_cycles_read();

				uint32_t code_stdo_length = grid_expr_state.output_string_length;
				
				temp[i] = ' '; // reverting terminating zero to space

				i+= 3-1; // +3 because  ?> -1 because i++
				code_length = code_end - code_start;

				char* stdo = &grid_expr_state.output_string[GRID_EXPR_OUTPUT_STRING_MAXLENGTH-grid_expr_state.output_string_length];

				//printf("Expr: %s \r\nTime [us]: %d\r\n", stdo, (cycles[1]-cycles[0])/120);

				strcpy(&target_string[code_start-total_substituted_length], stdo);


				total_substituted_length += code_length - code_stdo_length;
		

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


