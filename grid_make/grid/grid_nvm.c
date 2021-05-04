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

	nvm->next_write_offset = 0;

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


uint32_t grid_nvm_toc_defragmant(struct grid_nvm_model* mod){

	uint8_t block_buffer[GRID_NVM_BLOCK_SIZE] = {0x00};

	uint16_t block_count = 0;

	uint32_t write_ptr = 0;

	struct grid_nvm_toc_entry* current = mod->toc_head;

	while (current != NULL)
	{

		//current->config_string_offset = block_count * GRID_NVM_BLOCK_SIZE + write_ptr;
		printf("Moving CFG %d %d %d  Offset: %d -> %d\r\n", current->page_id, current->element_id, current->event_type, current->config_string_offset ,block_count * GRID_NVM_BLOCK_SIZE + write_ptr);

		if (write_ptr + current->config_string_length < GRID_NVM_BLOCK_SIZE){

			// the current config_string fit into the block no problem!
			flash_read(mod->flash, GRID_NVM_LOCAL_BASE_ADDRESS + current->config_string_offset, &block_buffer[write_ptr], current->config_string_length);
			write_ptr += current->config_string_length;

		}
		else{

			uint16_t part1_length = GRID_NVM_BLOCK_SIZE - write_ptr;
			uint16_t part2_length = current->config_string_length - part1_length;

			if (part2_length > GRID_NVM_BLOCK_SIZE){
				printf("error.nvm.cfg file is larger than nvm block size!\r\n");
			}

			// read as much as we can fit into the current block
			flash_read(mod->flash, GRID_NVM_LOCAL_BASE_ADDRESS + current->config_string_offset, &block_buffer[write_ptr], part1_length);
			
			// write the current block to flash
			flash_erase(mod->flash, GRID_NVM_LOCAL_BASE_ADDRESS + block_count*GRID_NVM_BLOCK_SIZE, GRID_NVM_BLOCK_SIZE/GRID_NVM_PAGE_SIZE);
			flash_append(mod->flash, GRID_NVM_LOCAL_BASE_ADDRESS + block_count*GRID_NVM_BLOCK_SIZE, block_buffer, GRID_NVM_BLOCK_SIZE);

			// update the write_ptr and block_count
			write_ptr = 0;
			block_count++;

			// clear the blockbuffer
			for (uint16_t i=0; i<GRID_NVM_BLOCK_SIZE; i++){
				block_buffer[i] = 0x00;
			}

			// read the rest of the configuration
			flash_read(mod->flash, GRID_NVM_LOCAL_BASE_ADDRESS + current->config_string_offset + part1_length, &block_buffer[write_ptr], part2_length);
					
			// update the write_ptr
			write_ptr += part2_length;

			//read


		}

		current->config_string_offset = GRID_NVM_BLOCK_SIZE * block_count + write_ptr - current->config_string_length;
		

		if (current->next == NULL){

			// no more elements in the list, write last partial block to NVM
			flash_erase(mod->flash, GRID_NVM_LOCAL_BASE_ADDRESS + block_count*GRID_NVM_BLOCK_SIZE, GRID_NVM_BLOCK_SIZE/GRID_NVM_PAGE_SIZE);
			flash_append(mod->flash, GRID_NVM_LOCAL_BASE_ADDRESS + block_count*GRID_NVM_BLOCK_SIZE, block_buffer, write_ptr);
			break;
		
		}
		else{
			// jump to next and run the loop again
			current = current->next;
		}

	}

	// set the next_write_offset to allow proper writes after defrag
	mod->next_write_offset = block_count*GRID_NVM_BLOCK_SIZE + write_ptr;
	printf("After defrag next_write_offset: %d\r\n", mod->next_write_offset);

	// clear the rest of the memory!
	block_count++;

	while(GRID_NVM_LOCAL_BASE_ADDRESS + block_count*GRID_NVM_BLOCK_SIZE < GRID_NVM_LOCAL_END_ADDRESS){

		flash_erase(mod->flash, GRID_NVM_LOCAL_BASE_ADDRESS + block_count*GRID_NVM_BLOCK_SIZE, GRID_NVM_BLOCK_SIZE/GRID_NVM_PAGE_SIZE);
		block_count++;
	}
	

	grid_nvm_toc_debug(mod);
	
}

uint32_t grid_nvm_append(struct grid_nvm_model* mod, uint8_t* buffer, uint16_t length){

	// before append pad to 8 byte words

	uint32_t append_length = length + (8 - length%8)%8; 

	uint8_t append_buffer[append_length];

	for (uint8_t i=0; i<append_length; i++){

		if (i<length){
			append_buffer[i] = buffer[i];
		}
		else{
			append_buffer[i] = 0x00;
		}

	}

	// use GRID_NVM_LOCAL_END_ADDRESS instead of 0x81000
	if (mod->next_write_offset + append_length > 0x1000){

		// not enough space for configs
		// run defrag algorithm!

		grid_nvm_toc_defragmant(mod);

	}

	// SUKU HACK
	flash_append(mod->flash, GRID_NVM_LOCAL_BASE_ADDRESS + mod->next_write_offset, append_buffer, append_length);
	
	uint8_t verify_buffer[append_length];

	for (uint16_t i=0; i<append_length; i++){
		verify_buffer[i] = 0xff;
	}

	flash_read(mod->flash, GRID_NVM_LOCAL_BASE_ADDRESS + mod->next_write_offset, verify_buffer, append_length);
	
	for (uint16_t i=0; i<append_length; i++){
		if (verify_buffer[i] != append_buffer[i]){
			printf("error.nvm.append verify failed! 0x%x  nwo:%d len:%d (%d!=%d)\r\n\r\n", GRID_NVM_LOCAL_BASE_ADDRESS + mod->next_write_offset + i, mod->bank_settings_page_address, append_length, verify_buffer[i], append_buffer[i]);
		}
	}



	mod->next_write_offset += append_length;

	return mod->next_write_offset-append_length; // return the start offset of the newly appended item

}
uint32_t grid_nvm_clear(struct grid_nvm_model* mod, uint32_t offset, uint16_t length){


	uint16_t clear_length = length + (8 - length%8)%8; 
	
	uint8_t clear_buffer[clear_length];
	uint8_t verify_buffer[clear_length];

	for (uint16_t i=0; i<clear_length; i++){

		clear_buffer[i] = 0x00;
		verify_buffer[i] = 0x00;
		

	}

	printf("clear_length: %d offset: %d\r\n", clear_length, offset);
	// SUKU HACK
	flash_append(mod->flash, GRID_NVM_LOCAL_BASE_ADDRESS + offset, clear_buffer, clear_length);

	flash_read(mod->flash, GRID_NVM_LOCAL_BASE_ADDRESS + offset, verify_buffer, clear_length);

	for (uint16_t i=0; i<clear_length; i++){
		if (verify_buffer[i] != 0x00){
			printf("\r\n\r\nerror.nvm.clear verify failed at 0x%x cb:%d vb:%d", GRID_NVM_LOCAL_BASE_ADDRESS + offset + i, clear_buffer[i], verify_buffer[i]);
			
			for(uint8_t j=0; j<10; j++){ // retry max 10 times

				// try again chunk
				uint8_t chunk[8] = {0};
				flash_append(mod->flash, GRID_NVM_LOCAL_BASE_ADDRESS + offset + (i/8)*8, &chunk, 8);
				flash_read(mod->flash, GRID_NVM_LOCAL_BASE_ADDRESS + offset + (i/8)*8, &chunk, 8);
				
				uint8_t failed_count = 0;

				for (uint8_t k = 0; k<8; k++){

					if (chunk[k] != 0x00){
					
						failed_count++;
					}
				}

				if (failed_count == 0){
					//printf("\r\n\r\nHAPPY!!!!\r\n\r\n");
					break;
				}
				else{
						//printf("\r\n\r\nANGRY!!!!\r\n\r\n");
				}

			}

		}
	}
	

}

uint32_t grid_nvm_erase_all(struct grid_nvm_model* mod){

	printf("\r\n\r\nFlash Erase\r\n\r\n");
	flash_erase(mod->flash, GRID_NVM_LOCAL_BASE_ADDRESS, (GRID_NVM_LOCAL_END_ADDRESS-GRID_NVM_LOCAL_BASE_ADDRESS)/GRID_NVM_PAGE_SIZE);
	
	printf("\r\n\r\nFlash Verify\r\n\r\n");
	uint32_t address = GRID_NVM_LOCAL_BASE_ADDRESS;
	uint32_t run = 0;
	uint32_t fail_count = 0;

	while (address != GRID_NVM_LOCAL_END_ADDRESS)
	{

		uint8_t verify_buffer[GRID_NVM_PAGE_SIZE] = {0};
		flash_read(mod->flash, address, verify_buffer, GRID_NVM_PAGE_SIZE);

		for(uint16_t i=0; i<GRID_NVM_PAGE_SIZE; i++){

			if (verify_buffer[i] != 0xff){

				printf("error.verify failed 0x%d\r\n", address + i);
				fail_count++;

			}
		}
		run++;
		address += GRID_NVM_PAGE_OFFSET;
	}
	
	printf("\r\n\r\nFlash Done, Run: %d Fail: %d\r\n\r\n", run, fail_count);

	return fail_count;

}



/*

GRID_NVM_TOC : Table of Contents

1. Scan through the user configurating memory area and find the next_write_address!
 - Make sure that next_write_address is always 4-byte aligned (actually 8 byte because of ECC)

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

void grid_nvm_toc_init(struct grid_nvm_model* mod){

	uint8_t flash_read_buffer[GRID_NVM_PAGE_SIZE] = {255};

	uint32_t last_used_page_offset = 0;
	uint32_t last_used_byte_offset = -8; // -8 because we will add +8 when calculating next_write_address

	// check first byte of every page to see if there is any useful data
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

	for(uint32_t i = 0; i<GRID_NVM_PAGE_SIZE; i+=8){ // +=8 because we want to keep the offset aligned

		if (flash_read_buffer[i] != 0xff){
			last_used_byte_offset = i;
		}
		else{

			break;
		}	
	}


	mod->next_write_offset = last_used_page_offset*GRID_NVM_PAGE_OFFSET + last_used_byte_offset + 8; // +8 because we want to write to the next word after the last used word


	// Read through the whole valid configuration area and parse valid configs into TOC


	uint8_t config_header[6] = {0};
	snprintf(config_header, 5, GRID_CLASS_CONFIG_frame_start); 

	for (uint32_t i=0; i<=last_used_page_offset; i++){ // <= because we want to check the last_used_page too

		flash_read(mod->flash, GRID_NVM_LOCAL_BASE_ADDRESS + i*GRID_NVM_PAGE_SIZE, flash_read_buffer, GRID_NVM_PAGE_SIZE);

		for (uint16_t j=0; j<GRID_NVM_PAGE_SIZE; j++){

			uint32_t current_offset = j + GRID_NVM_PAGE_SIZE*i;


			if (flash_read_buffer[j] == GRID_CONST_STX){

				uint8_t temp_buffer[20] = {0};

				uint8_t* current_header = temp_buffer;

				if (j>GRID_NVM_PAGE_SIZE-20){
					// read from flash, because the whole header is not in the page
					flash_read(mod->flash, GRID_NVM_LOCAL_BASE_ADDRESS + current_offset, temp_buffer, 19);
				}
				else{
					current_header = &flash_read_buffer[j];
				}

				if (0 == strncmp(current_header, config_header, 4)){

					uint8_t page_number = 0;
					uint8_t element_number = 0;
					uint8_t event_type = 0;
					uint8_t action_length = 0;

					page_number = grid_msg_get_parameter(current_header, GRID_CLASS_CONFIG_PAGENUMBER_offset, GRID_CLASS_CONFIG_PAGENUMBER_length, NULL);
					element_number = grid_msg_get_parameter(current_header, GRID_CLASS_CONFIG_ELEMENTNUMBER_offset, GRID_CLASS_CONFIG_ELEMENTNUMBER_length, NULL);
					event_type = grid_msg_get_parameter(current_header, GRID_CLASS_CONFIG_EVENTTYPE_offset, GRID_CLASS_CONFIG_EVENTTYPE_length, NULL);
					action_length = grid_msg_get_parameter(current_header, GRID_CLASS_CONFIG_ACTIONLENGTH_offset, GRID_CLASS_CONFIG_ACTIONLENGTH_length, NULL);

					grid_nvm_toc_entry_create(&grid_nvm_state, page_number, element_number, event_type, current_offset, action_length);

					
				}

			}


		}	
	
	}



}

uint32_t grid_nvm_config_mock(struct grid_nvm_model* mod){



	// generate random configuration
	

	uint8_t buf[300] = {0};
	uint16_t len = 0;

	uint8_t page_number = 0;
	uint8_t element_number = rand_sync_read8(&RAND_0)%3;
	uint8_t event_type = rand_sync_read8(&RAND_0)%3;

	sprintf(buf, GRID_CLASS_CONFIG_frame_start);

	grid_msg_set_parameter(buf, GRID_CLASS_CONFIG_PAGENUMBER_offset, GRID_CLASS_CONFIG_PAGENUMBER_length, page_number, NULL);
	grid_msg_set_parameter(buf, GRID_CLASS_CONFIG_ELEMENTNUMBER_offset, GRID_CLASS_CONFIG_ELEMENTNUMBER_length, element_number, NULL);
	grid_msg_set_parameter(buf, GRID_CLASS_CONFIG_EVENTTYPE_offset, GRID_CLASS_CONFIG_EVENTTYPE_length, event_type, NULL);

	len = strlen(buf);

	// append random length of fake actionstrings

	uint8_t mock_length = rand_sync_read8(&RAND_0)%128; // 0...127 is the mock length

	for (uint8_t i=0; i<mock_length; i++){ 

		buf[len+i] = 'a' + i%26;

	}

	len = strlen(buf);

	sprintf(&buf[len], GRID_CLASS_CONFIG_frame_end);

	len = strlen(buf);


	uint16_t config_length = len;

	grid_msg_set_parameter(buf, GRID_CLASS_CONFIG_ACTIONLENGTH_offset, GRID_CLASS_CONFIG_ACTIONLENGTH_length, config_length, NULL);

	printf("Mock frame len: %d -> %s\r\n", len, buf);


	grid_nvm_config_store(mod, page_number, element_number, event_type, buf, len);



}


uint32_t grid_nvm_config_store(struct grid_nvm_model* mod, uint8_t page_number, uint8_t element_number, uint8_t event_type, uint8_t* config_buffer, uint16_t config_length){


	struct grid_nvm_toc_entry* entry = NULL;

	entry = grid_nvm_toc_entry_find(&grid_nvm_state, page_number, element_number, event_type);

	uint32_t append_offset = grid_nvm_append(mod, config_buffer, config_length);

	if (entry == NULL){

		printf("NEW\r\n");
		grid_nvm_toc_entry_create(&grid_nvm_state, page_number, element_number, event_type, append_offset, config_length);

	}
	else{

		printf("UPDATE %d %d %d :  0x%x to 0x%x %d\r\n", entry->page_id, entry->element_id, entry->event_type, entry->config_string_offset, append_offset, config_length);
		grid_nvm_clear(mod, entry->config_string_offset, entry->config_string_length);
		grid_nvm_toc_entry_update(entry, append_offset, config_length);
		
	}



}

uint8_t grid_nvm_toc_entry_create(struct grid_nvm_model* mod, uint8_t page_id, uint8_t element_id, uint8_t event_type, uint32_t config_string_offset, uint16_t config_string_length){

	struct grid_nvm_toc_entry* prev = NULL;
	struct grid_nvm_toc_entry* next = mod->toc_head;
	struct grid_nvm_toc_entry* entry = NULL;

	uint32_t this_sort = (page_id<<16) + (element_id<<8) + (event_type);

	uint8_t duplicate = 0;


	while(1){

		uint32_t next_sort = -1; 

		if (next != NULL){
			
			next_sort = (next->page_id<<16) + (next->element_id<<8) + (next->event_type);
		}


		if (next == NULL){
			// this is the end of the list

			break;
		}
		else if (next_sort>this_sort){

			break;
		}
		else if (next_sort==this_sort){
			duplicate = 1;
			break;
		}
		else{
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
		entry->config_string_offset = config_string_offset;
		entry->config_string_length = config_string_length;


	}
	else{

		printf("error.nvm.malloc failed\r\n");
		return 255;
	
	}

}


struct grid_nvm_toc_entry* grid_nvm_toc_entry_find(struct grid_nvm_model* mod, uint8_t page_id, uint8_t element_id, uint8_t event_type){

	struct grid_nvm_toc_entry* current = mod->toc_head;

	while (current != NULL)
	{
		if (current->page_id == page_id && current->element_id == element_id && current->event_type == event_type){

			// Found the list item that we were looking for

			return current;

		}

		current = current->next;
	}
	
	return NULL;
}

uint8_t grid_nvm_toc_entry_update(struct grid_nvm_toc_entry* entry, uint32_t config_string_offset, uint16_t config_string_length){

	printf("UPDATE: %d -> %d\r\n", entry->config_string_offset, config_string_offset);

	entry->config_string_offset = config_string_offset;
	entry->config_string_length = config_string_length;

}

uint8_t grid_nvm_toc_entry_destroy(struct grid_nvm_model* mod, uint8_t page_id, uint8_t element_id, uint8_t event_type);

void grid_nvm_toc_debug(struct grid_nvm_model* mod){

	struct grid_nvm_toc_entry* next = mod->toc_head;


	printf("DUMP START\r\n");

	while (next != NULL){


		printf("toc entry: %d %d %d :  0x%x %d\r\n", next->page_id, next->element_id, next->event_type, next->config_string_offset, next->config_string_length);

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
	
	printf("grid_nvm_ui_bulk_read_next not implemented\r\n");


// 	if (nvm->read_bulk_status == 1){
		
		
// 		uint8_t bank    = (nvm->read_bulk_page_index/GRID_NVM_STRATEGY_EVENT_maxcount/GRID_NVM_STRATEGY_ELEMENT_maxcount)%GRID_NVM_STRATEGY_BANK_maxcount;
// 		uint8_t element = (nvm->read_bulk_page_index/GRID_NVM_STRATEGY_EVENT_maxcount)%GRID_NVM_STRATEGY_ELEMENT_maxcount;
// 		uint8_t event   = nvm->read_bulk_page_index%GRID_NVM_STRATEGY_EVENT_maxcount;
		
		
// 		if (bank < ui->bank_list_length){
			
// 			if (element < ui->bank_list[bank].element_list_length){
				
// 				if (event < ui->bank_list[bank].element_list[element].event_list_length){
// 					// Valid memory location
					
// 					int status = grid_ui_nvm_load_event_configuration(ui, nvm, &ui->bank_list[bank].element_list[element].event_list[event]);
							
					
							
// // 					uint8_t debugtext[200] = {0};
// // 					sprintf(debugtext, "Bulk Read Valid:: Status: %d, Index: %d => Bank: %d, Ele: %d, Eve: %d", status, nvm->read_bulk_page_index, bank, element, event);
// // 					grid_debug_print_text(debugtext);
// // 						
						
						
					
					
// 				}
				
// 			}
	
// 		}
		
		
// 		if (nvm->read_bulk_page_index < GRID_NVM_STRATEGY_EVENT_maxcount*GRID_NVM_STRATEGY_ELEMENT_maxcount*GRID_NVM_STRATEGY_BANK_maxcount-1){ // multiply with bankcount
			
// 			nvm->read_bulk_page_index++;
			
// 		}
// 		else{
			
// 			nvm->read_bulk_page_index = 0;
// 			nvm->read_bulk_status = 0;
			
			
// 			uint8_t acknowledge = 1;

// 			// Generate ACKNOWLEDGE RESPONSE
// 			struct grid_msg response;
				
// 			grid_msg_init(&response);
// 			grid_msg_init_header(&response, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_ROTATION);

// 			uint8_t response_payload[10] = {0};
// 			sprintf(response_payload, GRID_CLASS_LOCALLOAD_frame);

// 			grid_msg_body_append_text(&response, response_payload, strlen(response_payload));
				
// 			if (acknowledge == 1){
// 				grid_msg_text_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);

//             }
// 			else{
// 				grid_msg_text_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_NACKNOWLEDGE_code);

//             }

				
// 			grid_msg_packet_close(&response);
// 			grid_msg_packet_send_everywhere(&response);
				
			
// 		}
		
		
		
// 	}
	
	
	
	
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
    
	printf("grid_nvm_ui_bulk_store_next not implemented\r\n");
    
	// if (nvm->store_bulk_status == 1){
		
    //     uint8_t something_was_stored = 0;
        
    //     while (something_was_stored == 0){
        
    //         uint8_t bank    = (nvm->store_bulk_page_index/GRID_NVM_STRATEGY_EVENT_maxcount/GRID_NVM_STRATEGY_ELEMENT_maxcount)%GRID_NVM_STRATEGY_BANK_maxcount;
    //         uint8_t element = (nvm->store_bulk_page_index/GRID_NVM_STRATEGY_EVENT_maxcount)%GRID_NVM_STRATEGY_ELEMENT_maxcount;
    //         uint8_t event   = nvm->store_bulk_page_index%GRID_NVM_STRATEGY_EVENT_maxcount;


    //         if (bank < ui->bank_list_length){

    //             if (element < ui->bank_list[bank].element_list_length){

    //                 if (event < ui->bank_list[bank].element_list[element].event_list_length){
    //                     // Valid memory location

    //                     struct grid_ui_event* eve = &ui->bank_list[bank].element_list[element].event_list[event];

    //                     if (eve->cfg_changed_flag == 1){


    //                         if (grid_ui_nvm_store_event_configuration(ui, nvm, eve)){

    //                             something_was_stored = 1;


    //                         }



    //                     }
    //                 }


    //             }

    //         }
            
            

    //         if (nvm->store_bulk_page_index < GRID_NVM_STRATEGY_EVENT_maxcount*GRID_NVM_STRATEGY_ELEMENT_maxcount*GRID_NVM_STRATEGY_BANK_maxcount-1){ // multiply with bankcount



    //             nvm->store_bulk_page_index++;       
    //         }           
    //         else{
                
    //             //done deal, nothing was to be stored
    //             // lets lie to break out from the loop
    //             something_was_stored = 1;
    //         }
            
    
        
    //     }
		

		
		
		
	// 	if (nvm->store_bulk_page_index < GRID_NVM_STRATEGY_EVENT_maxcount*GRID_NVM_STRATEGY_ELEMENT_maxcount*GRID_NVM_STRATEGY_BANK_maxcount-1){ // multiply with bankcount
			
            
    //         uint8_t intensity = abs(nvm->store_bulk_page_index%100 - 50)/1.5 + 40;
  
    //         grid_sys_alert_set_alert(&grid_sys_state, 0, intensity*0.75, intensity, 1, 1000);


	// 	}
	// 	else{
			
	// 		nvm->store_bulk_page_index = 0;
	// 		nvm->store_bulk_status = 0;
			
			
	// 		uint8_t acknowledge = 1;

	// 		// Generate ACKNOWLEDGE RESPONSE
	// 		struct grid_msg response;
				
	// 		grid_msg_init(&response);
	// 		grid_msg_init_header(&response, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_ROTATION);

	// 		uint8_t response_payload[10] = {0};
	// 		sprintf(response_payload, GRID_CLASS_LOCALSTORE_frame);

	// 		grid_msg_body_append_text(&response, response_payload, strlen(response_payload));
				
	// 		if (acknowledge == 1){
	// 			grid_msg_text_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);
    //             grid_sys_alert_set_alert(&grid_sys_state, 0, 255, 0, 0, 1000);
    //         }
	// 		else{
	// 			grid_msg_text_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_NACKNOWLEDGE_code);
    //             grid_sys_alert_set_alert(&grid_sys_state, 0, 255, 0, 0, 1000);
    //         }

				
	// 		grid_msg_packet_close(&response);
	// 		grid_msg_packet_send_everywhere(&response);
						
	// 	}
		
		
		
	// }
	
	
	
	
}






void grid_nvm_ui_bulk_clear_init(struct grid_nvm_model* nvm, struct grid_ui_model* ui){

	nvm->clear_bulk_page_index = 0;
	nvm->clear_bulk_status = 1;
	
}

uint8_t grid_nvm_ui_bulk_clear_is_in_progress(struct grid_nvm_model* nvm, struct grid_ui_model* ui){

	return nvm->clear_bulk_status;
	
}


void grid_nvm_ui_bulk_clear_next(struct grid_nvm_model* nvm, struct grid_ui_model* ui){
	

	printf("grid_nvm_ui_bulk_clear_next not implemented\r\n");

	// if (nvm->clear_bulk_status == 1){
		
		
	// 	uint8_t bank    = (nvm->clear_bulk_page_index/GRID_NVM_STRATEGY_EVENT_maxcount/GRID_NVM_STRATEGY_ELEMENT_maxcount)%GRID_NVM_STRATEGY_BANK_maxcount;
	// 	uint8_t element = (nvm->clear_bulk_page_index/GRID_NVM_STRATEGY_EVENT_maxcount)%GRID_NVM_STRATEGY_ELEMENT_maxcount;
	// 	uint8_t event   = nvm->clear_bulk_page_index%GRID_NVM_STRATEGY_EVENT_maxcount;
		
		
	// 	if (bank < ui->bank_list_length){
			
	// 		if (element < ui->bank_list[bank].element_list_length){
				
	// 			if (event < ui->bank_list[bank].element_list[element].event_list_length){
	// 				// Valid memory location
					
	// 				grid_ui_nvm_clear_event_configuration(ui, nvm, &ui->bank_list[bank].element_list[element].event_list[event]);		
				
	// 			}
				
	// 		}
	
	// 	}
		
		
		
	// 	if (nvm->clear_bulk_page_index < GRID_NVM_STRATEGY_EVENT_maxcount*GRID_NVM_STRATEGY_ELEMENT_maxcount*GRID_NVM_STRATEGY_BANK_maxcount-1){ // multiply with bankcount
			
            
    //         uint8_t intensity = abs(nvm->clear_bulk_page_index%100 - 50)/1.5 + 40;
  
    //         grid_sys_alert_set_alert(&grid_sys_state, intensity, intensity*0.75, 0, 1, 1000);


            
    //         nvm->clear_bulk_page_index++;
	// 	}
	// 	else{
			
	// 		nvm->clear_bulk_page_index = 0;
	// 		nvm->clear_bulk_status = 0;
			
			
	// 		uint8_t acknowledge = 1;

	// 		// Generate ACKNOWLEDGE RESPONSE
	// 		struct grid_msg response;
				
	// 		grid_msg_init(&response);
	// 		grid_msg_init_header(&response, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_ROTATION);

	// 		uint8_t response_payload[10] = {0};
	// 		sprintf(response_payload, GRID_CLASS_LOCALCLEAR_frame);

	// 		grid_msg_body_append_text(&response, response_payload, strlen(response_payload));
				
	// 		if (acknowledge == 1){
	// 			grid_msg_text_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_ACKNOWLEDGE_code);
    //             grid_sys_alert_set_alert(&grid_sys_state, 0, 255, 0, 0, 1000);
    //         }
	// 		else{
	// 			grid_msg_text_set_parameter(&response, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_NACKNOWLEDGE_code);
    //             grid_sys_alert_set_alert(&grid_sys_state, 0, 255, 0, 0, 1000);
    //         }

				
	// 		grid_msg_packet_close(&response);
	// 		grid_msg_packet_send_everywhere(&response);
            
            
            
    //         grid_ui_reinit_local(&grid_ui_state);
						
	// 	}
		
		
		
	// }
	
	
	
	
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
	
	

	printf("grid_nvm_calculate_event_page_offset not implemented\r\n");

	
	// uint8_t bank_number		= eve->parent->parent->index;
	// uint8_t element_number	= eve->parent->index;
	// uint8_t event_number	= eve->index;

	// return GRID_NVM_STRATEGY_BANK_size * bank_number + GRID_NVM_STRATEGY_ELEMENT_size * element_number + GRID_NVM_STRATEGY_EVENT_size * event_number;
	
}