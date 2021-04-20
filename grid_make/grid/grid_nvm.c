/*
 * grid_nvm.c
 *
 * Created: 9/15/2020 3:41:44 PM
 *  Author: suku
 */ 


#include "grid_nvm.h"


void grid_nvm_init(struct grid_nvm_model* nvm, struct flash_descriptor* flash_instance){
	
	nvm->toc_count = 0;
	nvm->toc_head = NULL;

	nvm->next_write_address = 0;

	nvm->bank_settings_page_address = GRID_NVM_GLOBAL_BASE_ADDRESS;
	
	nvm->flash = flash_instance;
	
	nvm->status = 1;
	nvm->read_buffer_status = GRID_NVM_BUFFER_STATUS_UNINITIALIZED;
	nvm->write_buffer_status = GRID_NVM_BUFFER_STATUS_UNINITIALIZED;
	
	
	nvm->read_bulk_page_index = 0;
	nvm->read_bulk_status = 0;
	
	nvm->clear_bulk_page_index = 0;
	nvm->clear_bulk_status = 0;	
	
	nvm->write_bulk_page_index = 0;
	nvm->write_bulk_status = 0;
	
	
	grid_nvm_clear_read_buffer(nvm);
	grid_nvm_clear_write_buffer(nvm);

}


/*

GRID_NVM_TOC : Table of Contents

1. Scan through the user configurating memory area and find the next_write_address!
 - Make sure that next_write_address is always 4-byte aligned

2. Scan through the memory area that actually contains user configuration and create toc!
 - toc is a array of pointers that point to specific configuration strings in flash.
 - get stats: bytes of valid configurations, remaining memory space
 - when remaining memory is low, run defagment the memory:
	* copy the 4 pages worth of valid configuration to SRAM
	* clear the first 4 pages and store the config rom SRAM
	* do this until everything is stored in the begining of the memory space

3. When appending new configuration, make sure to update the toc in SRAM as well!
4. When appending new configuration, make sure to clear previous version to 0x00 in FLASH


*/

void grid_nvm_toc_create(struct grid_nvm_model* mod){

	uint8_t flash_write_buffer[512] = {0};

	flash_write_buffer[2] = 0xff;
	flash_write_buffer[3] = 0xff;
	flash_write_buffer[4] = 0xff;
	flash_write_buffer[5] = 0xff;

	flash_append(grid_nvm_state.flash, GRID_NVM_LOCAL_BASE_ADDRESS, flash_write_buffer, 12);

	uint8_t flash_read_buffer[512] = {255};

	uint32_t last_used_page_offset = 0;
	uint32_t last_used_byte_offset = -1;

	// check firest byte of every page to see if there is any useful data
	for (uint32_t i=0; i<GRID_NVM_LOCAL_PAGE_COUNT; i++){

		flash_read(grid_nvm_state.flash, GRID_NVM_LOCAL_BASE_ADDRESS + i*GRID_NVM_PAGE_OFFSET, flash_read_buffer, 1);
		if (flash_read_buffer[0] != 0xff){ // zero index because only first byt of the page was read

			// page is not empty!
			last_used_page_offset = i;
		}
		else{
			// no more user data
			break;
		}

	}

	// read the last page that has actual data
	flash_read(grid_nvm_state.flash, GRID_NVM_LOCAL_BASE_ADDRESS + last_used_page_offset*GRID_NVM_PAGE_OFFSET, flash_read_buffer, GRID_NVM_PAGE_SIZE);	

	for(uint32_t i = 0; i<GRID_NVM_PAGE_SIZE; i++){

		if (flash_read_buffer[i] != 0xff){
			last_used_byte_offset = i;
		}
		else{

			break;
		}	
	}


	mod->next_write_address = GRID_NVM_LOCAL_BASE_ADDRESS + last_used_page_offset*GRID_NVM_PAGE_OFFSET + last_used_byte_offset + 1; // +1 because we want to write to the firest byte after the last used byte
	
	if(mod->next_write_address%4 != 0){
		printf("error.nvm.next_write_address is not aligned! \r\n");
	}


	printf("Flash: next_write_address: %x\r\n", mod->next_write_address);

	uint8_t* str = "123";
	uint32_t len = strlen(str) + 1; // +1 because we print the NULL terminator for easy debugging

	// before append pad to 4 byte words

	

	uint32_t _len = len + (8 - mod->next_write_address%4 - len%4)%4; 
	printf("Flash: len: %d, _len: %d\r\n", len, _len);
	uint8_t _str[_len];

	for (uint8_t j=0; j<_len; j++){
		_str[j] = 0x00;
	}
	
	strcat(_str, str);

	for (uint8_t j=0; j<_len; j++){
		printf(" %d", _str[j]);
	}
	printf("\r\n");

	flash_append(grid_nvm_state.flash, mod->next_write_address, _str, _len);
	mod->next_write_address += len; // not _len because we want to write right next to it

	if(mod->next_write_address%4 != 0){
		printf("error.nvm.next_write_address is not aligned! \r\n");
	}


	printf("Flash: %s %d next_write_address: %x\r\n", str, len, mod->next_write_address);

	grid_nvm_toc_entry_create(&grid_nvm_state, 1, 2, 3, 0, 1);
	grid_nvm_toc_entry_create(&grid_nvm_state, 1, 4, 2, 0, 2);
	grid_nvm_toc_entry_create(&grid_nvm_state, 1, 3, 1, 0, 3);
	grid_nvm_toc_entry_create(&grid_nvm_state, 1, 3, 1, 0, 4);
	grid_nvm_toc_entry_create(&grid_nvm_state, 1, 3, 2, 0, 5);
	grid_nvm_toc_entry_create(&grid_nvm_state, 1, 3, 0, 0, 6);
	grid_nvm_toc_entry_create(&grid_nvm_state, 1, 4, 1, 0, 7);
	grid_nvm_toc_entry_create(&grid_nvm_state, 2, 4, 2, 0, 8);
	grid_nvm_toc_entry_create(&grid_nvm_state, 1, 4, 2, 0, 9);

	grid_nvm_toc_debug(&grid_nvm_state);

}

uint8_t grid_nvm_toc_entry_create(struct grid_nvm_model* mod, uint8_t page_id, uint8_t element_id, uint8_t event_type, uint16_t config_string_length, uint32_t config_string_address){

	struct grid_nvm_toc_entry* prev = NULL;
	struct grid_nvm_toc_entry* next = mod->toc_head;
	struct grid_nvm_toc_entry* entry = NULL;

	uint32_t this_sort = (page_id<<16) + (element_id<<8) + (event_type);

	uint8_t duplicate = 0;

	printf("!!!!\r\n");

	while(1){

		uint32_t next_sort = -1; 

		if (next != NULL){
			
			next_sort = (next->page_id<<16) + (next->element_id<<8) + (next->event_type);
		}


		if (next == NULL){
			// this is the end of the list

			printf("I am last\r\n");
			break;
		}
		else if (next_sort>this_sort){

			printf("place found!\r\n");
			break;
		}
		else if (next_sort==this_sort){
			printf("Duplicate entry\r\n");
			duplicate = 1;
			break;
		}
		else{
			printf("next->next\r\n");
			prev = next;
			next = next->next;
		}

	}


	if (duplicate == 0) {

		entry = malloc(sizeof(struct grid_nvm_toc_entry));
	}
	else{
		entry = next;
	}


	// if new element was successfully created
	if (entry != NULL){

		if (duplicate == 0){ // not duplicate
			mod->toc_count++;

			entry->next = next;
			entry->prev = prev;
			
			if (prev == NULL){
				// this is first element
				mod->toc_head = entry;
			}
			else{
				prev->next = entry;
			}


			if (next != NULL){
				next->prev = entry;
			}

		}

		entry->status = 1;

		entry->page_id = page_id;
		entry->element_id = element_id;
		entry->event_type = event_type;

		// here manipulate NVM if duplicate
		entry->config_string_address = config_string_address;
		entry->config_string_length = config_string_length;


	}
	else{

		printf("error.nvm.malloc failed\r\n");
		return 255;
	
	}




}
uint8_t grid_nvm_toc_entry_destroy(struct grid_nvm_model* mod, uint8_t page_id, uint8_t element_id, uint8_t event_type);

void grid_nvm_toc_debug(struct grid_nvm_model* mod){

	struct grid_nvm_toc_entry* next = mod->toc_head;


	printf("DUMP START\r\n");

	while (next != NULL){


		printf("toc entry: %d %d %d :  %d \r\n", next->page_id, next->element_id, next->event_type, next->config_string_address);

		next = next->next;
	}

	printf("DUMP DONE\r\n");




}








void grid_nvm_ui_bulk_read_init(struct grid_nvm_model* nvm, struct grid_ui_model* ui){

	nvm->read_bulk_page_index = 0;
	nvm->read_bulk_status = 1;
			
}

uint8_t grid_nvm_ui_bulk_read_is_in_progress(struct grid_nvm_model* nvm, struct grid_ui_model* ui){

	return nvm->read_bulk_status;
	
}

void grid_nvm_ui_bulk_read_next(struct grid_nvm_model* nvm, struct grid_ui_model* ui){
	
	if (nvm->read_bulk_status == 1){
		
		
		uint8_t bank    = (nvm->read_bulk_page_index/GRID_NVM_STRATEGY_EVENT_maxcount/GRID_NVM_STRATEGY_ELEMENT_maxcount)%GRID_NVM_STRATEGY_BANK_maxcount;
		uint8_t element = (nvm->read_bulk_page_index/GRID_NVM_STRATEGY_EVENT_maxcount)%GRID_NVM_STRATEGY_ELEMENT_maxcount;
		uint8_t event   = nvm->read_bulk_page_index%GRID_NVM_STRATEGY_EVENT_maxcount;
		
		
		if (bank < ui->bank_list_length){
			
			if (element < ui->bank_list[bank].element_list_length){
				
				if (event < ui->bank_list[bank].element_list[element].event_list_length){
					// Valid memory location
					
					int status = grid_ui_nvm_load_event_configuration(ui, nvm, &ui->bank_list[bank].element_list[element].event_list[event]);
							
					
							
// 					uint8_t debugtext[200] = {0};
// 					sprintf(debugtext, "Bulk Read Valid:: Status: %d, Index: %d => Bank: %d, Ele: %d, Eve: %d", status, nvm->read_bulk_page_index, bank, element, event);
// 					grid_debug_print_text(debugtext);
// 						
						
						
					
					
				}
				
			}
	
		}
		
		
		if (nvm->read_bulk_page_index < GRID_NVM_STRATEGY_EVENT_maxcount*GRID_NVM_STRATEGY_ELEMENT_maxcount*GRID_NVM_STRATEGY_BANK_maxcount-1){ // multiply with bankcount
			
			nvm->read_bulk_page_index++;
			
		}
		else{
			
			nvm->read_bulk_page_index = 0;
			nvm->read_bulk_status = 0;
			
			
			uint8_t acknowledge = 1;

			// Generate ACKNOWLEDGE RESPONSE
			struct grid_msg response;
				
			grid_msg_init(&response);
			grid_msg_init_header(&response, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_ROTATION);

			uint8_t response_payload[10] = {0};
			sprintf(response_payload, GRID_CLASS_LOCALLOAD_frame);

			grid_msg_body_append_text(&response, response_payload, strlen(response_payload));
				
			if (acknowledge == 1){
				grid_msg_text_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);

            }
			else{
				grid_msg_text_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_NACKNOWLEDGE_code);

            }

				
			grid_msg_packet_close(&response);
			grid_msg_packet_send_everywhere(&response);
				
			
		}
		
		
		
	}
	
	
	
	
}


void grid_nvm_ui_bulk_store_init(struct grid_nvm_model* nvm, struct grid_ui_model* ui){

	nvm->store_bulk_page_index = 0;
	nvm->store_bulk_status = 1;
	
}

uint8_t grid_nvm_ui_bulk_store_is_in_progress(struct grid_nvm_model* nvm, struct grid_ui_model* ui){

	return nvm->store_bulk_status;
	
}

// DO THIS!!
void grid_nvm_ui_bulk_store_next(struct grid_nvm_model* nvm, struct grid_ui_model* ui){
     
    // START: NEW
    
    
	if (nvm->store_bulk_status == 1){
		
        uint8_t something_was_stored = 0;
        
        while (something_was_stored == 0){
        
            uint8_t bank    = (nvm->store_bulk_page_index/GRID_NVM_STRATEGY_EVENT_maxcount/GRID_NVM_STRATEGY_ELEMENT_maxcount)%GRID_NVM_STRATEGY_BANK_maxcount;
            uint8_t element = (nvm->store_bulk_page_index/GRID_NVM_STRATEGY_EVENT_maxcount)%GRID_NVM_STRATEGY_ELEMENT_maxcount;
            uint8_t event   = nvm->store_bulk_page_index%GRID_NVM_STRATEGY_EVENT_maxcount;


            if (bank < ui->bank_list_length){

                if (element < ui->bank_list[bank].element_list_length){

                    if (event < ui->bank_list[bank].element_list[element].event_list_length){
                        // Valid memory location

                        struct grid_ui_event* eve = &ui->bank_list[bank].element_list[element].event_list[event];

                        if (eve->cfg_changed_flag == 1){


                            if (grid_ui_nvm_store_event_configuration(ui, nvm, eve)){

                                something_was_stored = 1;


                            }



                        }
                    }


                }

            }
            
            

            if (nvm->store_bulk_page_index < GRID_NVM_STRATEGY_EVENT_maxcount*GRID_NVM_STRATEGY_ELEMENT_maxcount*GRID_NVM_STRATEGY_BANK_maxcount-1){ // multiply with bankcount



                nvm->store_bulk_page_index++;       
            }           
            else{
                
                //done deal, nothing was to be stored
                // lets lie to break out from the loop
                something_was_stored = 1;
            }
            
    
        
        }
		

		
		
		
		if (nvm->store_bulk_page_index < GRID_NVM_STRATEGY_EVENT_maxcount*GRID_NVM_STRATEGY_ELEMENT_maxcount*GRID_NVM_STRATEGY_BANK_maxcount-1){ // multiply with bankcount
			
            
            uint8_t intensity = abs(nvm->store_bulk_page_index%100 - 50)/1.5 + 40;
  
            grid_sys_alert_set_alert(&grid_sys_state, 0, intensity*0.75, intensity, 1, 1000);


		}
		else{
			
			nvm->store_bulk_page_index = 0;
			nvm->store_bulk_status = 0;
			
			
			uint8_t acknowledge = 1;

			// Generate ACKNOWLEDGE RESPONSE
			struct grid_msg response;
				
			grid_msg_init(&response);
			grid_msg_init_header(&response, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_ROTATION);

			uint8_t response_payload[10] = {0};
			sprintf(response_payload, GRID_CLASS_LOCALSTORE_frame);

			grid_msg_body_append_text(&response, response_payload, strlen(response_payload));
				
			if (acknowledge == 1){
				grid_msg_text_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);
                grid_sys_alert_set_alert(&grid_sys_state, 0, 255, 0, 0, 1000);
            }
			else{
				grid_msg_text_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_NACKNOWLEDGE_code);
                grid_sys_alert_set_alert(&grid_sys_state, 0, 255, 0, 0, 1000);
            }

				
			grid_msg_packet_close(&response);
			grid_msg_packet_send_everywhere(&response);
						
		}
		
		
		
	}
	
	
	
	
}






void grid_nvm_ui_bulk_clear_init(struct grid_nvm_model* nvm, struct grid_ui_model* ui){

	nvm->clear_bulk_page_index = 0;
	nvm->clear_bulk_status = 1;
	
}

uint8_t grid_nvm_ui_bulk_clear_is_in_progress(struct grid_nvm_model* nvm, struct grid_ui_model* ui){

	return nvm->clear_bulk_status;
	
}


void grid_nvm_ui_bulk_clear_next(struct grid_nvm_model* nvm, struct grid_ui_model* ui){
	
	if (nvm->clear_bulk_status == 1){
		
		
		uint8_t bank    = (nvm->clear_bulk_page_index/GRID_NVM_STRATEGY_EVENT_maxcount/GRID_NVM_STRATEGY_ELEMENT_maxcount)%GRID_NVM_STRATEGY_BANK_maxcount;
		uint8_t element = (nvm->clear_bulk_page_index/GRID_NVM_STRATEGY_EVENT_maxcount)%GRID_NVM_STRATEGY_ELEMENT_maxcount;
		uint8_t event   = nvm->clear_bulk_page_index%GRID_NVM_STRATEGY_EVENT_maxcount;
		
		
		if (bank < ui->bank_list_length){
			
			if (element < ui->bank_list[bank].element_list_length){
				
				if (event < ui->bank_list[bank].element_list[element].event_list_length){
					// Valid memory location
					
					grid_ui_nvm_clear_event_configuration(ui, nvm, &ui->bank_list[bank].element_list[element].event_list[event]);		
				
				}
				
			}
	
		}
		
		
		
		if (nvm->clear_bulk_page_index < GRID_NVM_STRATEGY_EVENT_maxcount*GRID_NVM_STRATEGY_ELEMENT_maxcount*GRID_NVM_STRATEGY_BANK_maxcount-1){ // multiply with bankcount
			
            
            uint8_t intensity = abs(nvm->clear_bulk_page_index%100 - 50)/1.5 + 40;
  
            grid_sys_alert_set_alert(&grid_sys_state, intensity, intensity*0.75, 0, 1, 1000);


            
            nvm->clear_bulk_page_index++;
		}
		else{
			
			nvm->clear_bulk_page_index = 0;
			nvm->clear_bulk_status = 0;
			
			
			uint8_t acknowledge = 1;

			// Generate ACKNOWLEDGE RESPONSE
			struct grid_msg response;
				
			grid_msg_init(&response);
			grid_msg_init_header(&response, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_ROTATION);

			uint8_t response_payload[10] = {0};
			sprintf(response_payload, GRID_CLASS_LOCALCLEAR_frame);

			grid_msg_body_append_text(&response, response_payload, strlen(response_payload));
				
			if (acknowledge == 1){
				grid_msg_text_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);
                grid_sys_alert_set_alert(&grid_sys_state, 0, 255, 0, 0, 1000);
            }
			else{
				grid_msg_text_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_NACKNOWLEDGE_code);
                grid_sys_alert_set_alert(&grid_sys_state, 0, 255, 0, 0, 1000);
            }

				
			grid_msg_packet_close(&response);
			grid_msg_packet_send_everywhere(&response);
            
            
            
            grid_ui_reinit_local(&grid_ui_state);
						
		}
		
		
		
	}
	
	
	
	
}





void grid_nvm_clear_read_buffer(struct grid_nvm_model* mod){
	
	for (uint32_t i=0; i<GRID_NVM_PAGE_SIZE; i++){
		
		mod->read_buffer[i] = 255;
		
	}

	mod->read_buffer_status = GRID_NVM_BUFFER_STATUS_EMPTY;
	mod->read_buffer_length = 0;
	
}

void grid_nvm_clear_write_buffer(struct grid_nvm_model* mod){
	
	for (uint32_t i=0; i<GRID_NVM_PAGE_SIZE; i++){
		
		mod->write_buffer[i] = 255;
		
	}
	
	mod->write_buffer_status = GRID_NVM_BUFFER_STATUS_EMPTY;
	mod->write_buffer_length = 0;
	mod->write_target_address = -1;
}


uint32_t grid_nvm_calculate_event_page_offset(struct grid_nvm_model* nvm, struct grid_ui_event* eve){
	
	
	
	uint8_t bank_number		= eve->parent->parent->index;
	uint8_t element_number	= eve->parent->index;
	uint8_t event_number	= eve->index;

	return GRID_NVM_STRATEGY_BANK_size * bank_number + GRID_NVM_STRATEGY_ELEMENT_size * element_number + GRID_NVM_STRATEGY_EVENT_size * event_number;
	
}