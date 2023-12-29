/*
 * grid_nvm.c
 *
 * Created: 9/15/2020 3:41:44 PM
 *  Author: suku
 */

#include "grid_d51_nvm.h"

void grid_d51_nvm_init(struct grid_d51_nvm_model *nvm,
                       struct flash_descriptor *flash_instance) {

  nvm->toc_count = 0;
  nvm->toc_head = NULL;

  nvm->next_write_offset = 0;

  nvm->flash = flash_instance;

  nvm->status = 1;

  uint32_t erase_bulk_address;
}

uint32_t grid_d51_nvm_toc_defragment(struct grid_d51_nvm_model *nvm) {

  grid_port_debug_printf("Start defragmentation");

  uint8_t block_buffer[GRID_D51_NVM_BLOCK_SIZE] = {0x00};

  uint16_t block_count = 0;

  uint32_t write_ptr = 0;

  struct grid_d51_nvm_toc_entry *current = nvm->toc_head;

  while (current != NULL) {
    // current->config_string_offset = block_count * GRID_D51_NVM_BLOCK_SIZE +
    // write_ptr;
    //  printf("Moving CFG %d %d %d  Offset: %d -> %d\r\n", current->page_id,
    //  current->element_id, current->event_type, current->config_string_offset
    //  ,block_count * GRID_D51_NVM_BLOCK_SIZE + write_ptr);

    if (write_ptr + current->config_string_length < GRID_D51_NVM_BLOCK_SIZE) {

      // the current config_string fit into the block no problem!
      CRITICAL_SECTION_ENTER()
      flash_read(nvm->flash,
                 GRID_D51_NVM_LOCAL_BASE_ADDRESS +
                     current->config_string_offset,
                 &block_buffer[write_ptr], current->config_string_length);
      CRITICAL_SECTION_LEAVE()

      write_ptr += current->config_string_length;

    } else {

      uint16_t part1_length = GRID_D51_NVM_BLOCK_SIZE - write_ptr;
      uint16_t part2_length = current->config_string_length - part1_length;

      if (part2_length > GRID_D51_NVM_BLOCK_SIZE) {
        // printf("error.nvm.cfg file is larger than nvm block size!\r\n");
      }

      // read as much as we can fit into the current block
      CRITICAL_SECTION_ENTER()
      flash_read(nvm->flash,
                 GRID_D51_NVM_LOCAL_BASE_ADDRESS +
                     current->config_string_offset,
                 &block_buffer[write_ptr], part1_length);
      // write the current block to flash
      flash_erase(nvm->flash,
                  GRID_D51_NVM_LOCAL_BASE_ADDRESS +
                      block_count * GRID_D51_NVM_BLOCK_SIZE,
                  GRID_D51_NVM_BLOCK_SIZE / GRID_D51_NVM_PAGE_SIZE);
      flash_append(nvm->flash,
                   GRID_D51_NVM_LOCAL_BASE_ADDRESS +
                       block_count * GRID_D51_NVM_BLOCK_SIZE,
                   block_buffer, GRID_D51_NVM_BLOCK_SIZE);
      CRITICAL_SECTION_LEAVE()

      // update the write_ptr and block_count
      write_ptr = 0;
      block_count++;

      // clear the blockbuffer
      for (uint16_t i = 0; i < GRID_D51_NVM_BLOCK_SIZE; i++) {
        block_buffer[i] = 0x00;
      }

      // read the rest of the configuration
      CRITICAL_SECTION_ENTER()
      flash_read(nvm->flash,
                 GRID_D51_NVM_LOCAL_BASE_ADDRESS +
                     current->config_string_offset + part1_length,
                 &block_buffer[write_ptr], part2_length);
      CRITICAL_SECTION_LEAVE()

      // update the write_ptr
      write_ptr += part2_length;

      // read
    }

    current->config_string_offset = GRID_D51_NVM_BLOCK_SIZE * block_count +
                                    write_ptr - current->config_string_length;

    if (current->next == NULL) {

      // no more elements in the list, write last partial block to NVM
      // CRITICAL_SECTION_ENTER()
      flash_erase(nvm->flash,
                  GRID_D51_NVM_LOCAL_BASE_ADDRESS +
                      block_count * GRID_D51_NVM_BLOCK_SIZE,
                  GRID_D51_NVM_BLOCK_SIZE / GRID_D51_NVM_PAGE_SIZE);
      flash_append(nvm->flash,
                   GRID_D51_NVM_LOCAL_BASE_ADDRESS +
                       block_count * GRID_D51_NVM_BLOCK_SIZE,
                   block_buffer, write_ptr);
      // CRITICAL_SECTION_LEAVE()

      break;

    } else {
      // jump to next and run the loop again
      current = current->next;
    }
  }

  // set the next_write_offset to allow proper writes after defrag
  nvm->next_write_offset = block_count * GRID_D51_NVM_BLOCK_SIZE + write_ptr;
  // printf("After defrag next_write_offset: %d\r\n", nvm->next_write_offset);

  // clear the rest of the memory!
  block_count++;

  while (GRID_D51_NVM_LOCAL_BASE_ADDRESS +
             block_count * GRID_D51_NVM_BLOCK_SIZE <
         GRID_D51_NVM_LOCAL_END_ADDRESS) {

    CRITICAL_SECTION_ENTER()
    flash_erase(nvm->flash,
                GRID_D51_NVM_LOCAL_BASE_ADDRESS +
                    block_count * GRID_D51_NVM_BLOCK_SIZE,
                GRID_D51_NVM_BLOCK_SIZE / GRID_D51_NVM_PAGE_SIZE);
    CRITICAL_SECTION_LEAVE()

    block_count++;
  }

  struct grid_msg_packet response;

  grid_msg_packet_init(&grid_msg_state, &response,
                       GRID_PARAMETER_GLOBAL_POSITION,
                       GRID_PARAMETER_GLOBAL_POSITION);

  // acknowledge
  grid_msg_packet_body_append_printf(&response, GRID_CLASS_NVMDEFRAG_frame);
  grid_msg_packet_body_append_parameter(&response, GRID_INSTR_offset,
                                        GRID_INSTR_length,
                                        GRID_INSTR_ACKNOWLEDGE_code);

  // debugtext
  grid_msg_packet_body_append_printf(&response,
                                     GRID_CLASS_DEBUGTEXT_frame_start);
  grid_msg_packet_body_append_parameter(
      &response, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);
  grid_msg_packet_body_append_printf(&response, "xdefrag complete 0x%x",
                                     GRID_D51_NVM_LOCAL_BASE_ADDRESS +
                                         nvm->next_write_offset);
  grid_msg_packet_body_append_printf(&response, GRID_CLASS_DEBUGTEXT_frame_end);

  grid_msg_packet_close(&grid_msg_state, &response);

  grid_port_packet_send_everywhere(&response);

  grid_d51_nvm_toc_debug(nvm);
}

uint32_t grid_d51_nvm_append(struct grid_d51_nvm_model *mod, uint8_t *buffer,
                             uint16_t length) {

  // before append pad to 8 byte words

  // printf("APPEND START len: %d\r\n", length);
  uint32_t append_length = length + (8 - length % 8) % 8;

  uint8_t append_buffer[append_length];

  for (uint16_t i = 0; i < append_length; i++) {

    if (i < length) {
      append_buffer[i] = buffer[i];
    } else {
      append_buffer[i] = 0x00;
    }
  }

  // printf("APPEND: %s", append_buffer);

  // use GRID_D51_NVM_LOCAL_END_ADDRESS instead of 0x81000
  if (GRID_D51_NVM_LOCAL_BASE_ADDRESS + mod->next_write_offset + append_length >
      GRID_D51_NVM_LOCAL_END_ADDRESS - 0x1000) {

    // not enough space for configs
    // run defrag algorithm!

    grid_d51_nvm_toc_defragment(mod);
  }

  // APPEND
  CRITICAL_SECTION_ENTER()
  flash_append(mod->flash,
               GRID_D51_NVM_LOCAL_BASE_ADDRESS + mod->next_write_offset,
               append_buffer, append_length);
  CRITICAL_SECTION_LEAVE()

  // CREATE VERIFY BUFFER
  uint8_t verify_buffer[append_length];

  for (uint16_t i = 0; i < append_length; i++) {
    verify_buffer[i] = 0xff;
  }

  // VERIFY FLASH CONTENT
  CRITICAL_SECTION_ENTER()
  flash_read(mod->flash,
             GRID_D51_NVM_LOCAL_BASE_ADDRESS + mod->next_write_offset,
             verify_buffer, append_length);
  CRITICAL_SECTION_LEAVE()
  uint8_t failed_flag = 0;

  for (uint16_t i = 0; i < append_length; i++) {
    if (verify_buffer[i] != append_buffer[i]) {
      // printf("ERROR: APPEND VERIFY FAILED 0x%x  len:%d (%d!=%d)\r\n\r\n",
      // GRID_D51_NVM_LOCAL_BASE_ADDRESS + mod->next_write_offset + i,
      // append_length, verify_buffer[i], append_buffer[i]);
      grid_port_debug_printf("append verify failed 0x%x len:%d (%d!=%d)",
                             GRID_D51_NVM_LOCAL_BASE_ADDRESS +
                                 mod->next_write_offset + i,
                             append_length, verify_buffer[i], append_buffer[i]);
      failed_flag = 1;
    }
  }

  // IF FAILED, TRY USING FLASH_WRITE
  if (failed_flag) {
    grid_port_debug_printf("Attempt to fix flash content");

    CRITICAL_SECTION_ENTER()
    flash_write(mod->flash,
                GRID_D51_NVM_LOCAL_BASE_ADDRESS + mod->next_write_offset,
                append_buffer, append_length);
    CRITICAL_SECTION_LEAVE()

    // VERIFY AGAIN
    failed_flag = 0;

    for (uint16_t i = 0; i < append_length; i++) {
      verify_buffer[i] = 0xff;
    }

    // VERIFY FLASH CONTENT
    CRITICAL_SECTION_ENTER()
    flash_read(mod->flash,
               GRID_D51_NVM_LOCAL_BASE_ADDRESS + mod->next_write_offset,
               verify_buffer, append_length);
    CRITICAL_SECTION_LEAVE()

    uint8_t failed_flag = 0;

    for (uint16_t i = 0; i < append_length; i++) {
      if (verify_buffer[i] != append_buffer[i]) {
        // printf("ERROR: APPEND VERIFY FAILED 0x%x  len:%d (%d!=%d)\r\n\r\n",
        // GRID_D51_NVM_LOCAL_BASE_ADDRESS + mod->next_write_offset + i,
        // append_length, verify_buffer[i], append_buffer[i]);
        grid_port_debug_printf(
            "append verify failed 0x%x len:%d (%d!=%d)",
            GRID_D51_NVM_LOCAL_BASE_ADDRESS + mod->next_write_offset + i,
            append_length, verify_buffer[i], append_buffer[i]);
        failed_flag = 1;
      }
    }

    if (failed_flag) {
      grid_port_debug_printf("Still failed, sorry!");
    } else {
      grid_port_debug_printf("All Good!");
    }
  }

  mod->next_write_offset += append_length;

  return mod->next_write_offset -
         append_length; // return the start offset of the newly appended item
}
uint32_t grid_d51_nvm_clear(struct grid_d51_nvm_model *mod, uint32_t offset,
                            uint16_t length) {

  uint16_t clear_length = length + (8 - length % 8) % 8;

  uint8_t clear_buffer[clear_length];
  uint8_t verify_buffer[clear_length];

  for (uint16_t i = 0; i < clear_length; i++) {

    clear_buffer[i] = 0x00;
    verify_buffer[i] = 0x00;
  }

  // printf("clear_length: %d offset: %d\r\n", clear_length, offset);
  //  SUKU HACK
  flash_append(mod->flash, GRID_D51_NVM_LOCAL_BASE_ADDRESS + offset,
               clear_buffer, clear_length);
  // flash_read(mod->flash, GRID_D51_NVM_LOCAL_BASE_ADDRESS + offset,
  // verify_buffer, clear_length);

  // for (uint16_t i=0; i<clear_length; i++){
  // 	if (verify_buffer[i] != 0x00){
  // 		printf("\r\n\r\nerror.nvm.clear verify failed at 0x%x cb:%d
  // vb:%d", GRID_D51_NVM_LOCAL_BASE_ADDRESS + offset + i, clear_buffer[i],
  // verify_buffer[i]); 		grid_port_debug_printf("clear verify failed");
  // 		for(uint8_t j=0; j<10; j++){ // retry max 10 times

  // 			// try again chunk
  // 			uint8_t chunk[8] = {0};
  // 			flash_append(mod->flash, GRID_D51_NVM_LOCAL_BASE_ADDRESS
  // + offset + (i/8)*8, &chunk, 8); 			flash_read(mod->flash,
  // GRID_D51_NVM_LOCAL_BASE_ADDRESS + offset + (i/8)*8, &chunk, 8);

  // 			uint8_t failed_count = 0;

  // 			for (uint8_t k = 0; k<8; k++){

  // 				if (chunk[k] != 0x00){

  // 					failed_count++;
  // 				}
  // 			}

  // 			if (failed_count == 0){
  // 				//printf("\r\n\r\nHAPPY!!!!\r\n\r\n");
  // 				break;
  // 			}
  // 			else{
  // 					//printf("\r\n\r\nANGRY!!!!\r\n\r\n");
  // 			}

  // 		}

  // 	}
  // }
}

uint32_t grid_d51_nvm_erase_all(struct grid_d51_nvm_model *mod) {

  // printf("\r\n\r\nFlash Erase\r\n\r\n");
  CRITICAL_SECTION_ENTER()
  flash_erase(
      mod->flash, GRID_D51_NVM_LOCAL_BASE_ADDRESS,
      (GRID_D51_NVM_LOCAL_END_ADDRESS - GRID_D51_NVM_LOCAL_BASE_ADDRESS) /
          GRID_D51_NVM_PAGE_SIZE);
  CRITICAL_SECTION_LEAVE()

  // printf("\r\n\r\nFlash Verify\r\n\r\n");
  uint32_t address = GRID_D51_NVM_LOCAL_BASE_ADDRESS;
  uint32_t run = 0;
  uint32_t fail_count = 0;

  while (address != GRID_D51_NVM_LOCAL_END_ADDRESS) {

    uint8_t verify_buffer[GRID_D51_NVM_PAGE_SIZE] = {0};
    CRITICAL_SECTION_ENTER()
    flash_read(mod->flash, address, verify_buffer, GRID_D51_NVM_PAGE_SIZE);
    CRITICAL_SECTION_LEAVE()

    for (uint16_t i = 0; i < GRID_D51_NVM_PAGE_SIZE; i++) {

      if (verify_buffer[i] != 0xff) {

        // printf("error.verify failed 0x%d\r\n", address + i);
        fail_count++;
      }
    }
    run++;
    address += GRID_D51_NVM_PAGE_OFFSET;
  }

  // printf("\r\n\r\nFlash Done, Run: %d Fail: %d\r\n\r\n", run, fail_count);

  return fail_count;
}

/*

GRID_D51_NVM_TOC : Table of Contents

1. Scan through the user configurating memory area and find the
next_write_address!
 - Make sure that next_write_address is always 4-byte aligned (actually 8 byte
because of ECC)

2. Scan through the memory area that actually contains user configuration and
create toc!
 - toc is a array of pointers that point to specific configuration strings in
flash.
 - get stats: bytes of valid configurations, remaining memory space
 - when remaining memory is low, run defagment the memory:
        * copy the 4 pages worth of valid configuration to SRAM
        * clear the first 4 pages and store the config rom SRAM
        * do this until everything is stored in the beginning of the memory
space

3. When appending new configuration, make sure to update the toc in SRAM as
well!
4. When appending new configuration, make sure to clear previous version to 0x00
in FLASH


*/

void grid_d51_nvm_toc_init(struct grid_d51_nvm_model *nvm) {

  uint8_t flash_read_buffer[GRID_D51_NVM_PAGE_SIZE] = {255};

  uint32_t last_used_page_offset = 0;
  uint32_t last_used_byte_offset =
      -8; // -8 because we will add +8 when calculating next_write_address

  // check first byte of every page to see if there is any useful data
  for (uint32_t i = 0; i < GRID_D51_NVM_LOCAL_PAGE_COUNT; i++) {

    CRITICAL_SECTION_ENTER()
    flash_read(grid_d51_nvm_state.flash,
               GRID_D51_NVM_LOCAL_BASE_ADDRESS + i * GRID_D51_NVM_PAGE_OFFSET,
               flash_read_buffer, 1);
    CRITICAL_SECTION_LEAVE()

    if (flash_read_buffer[0] !=
        0xff) { // zero index because only first byt of the page was read

      // page is not empty!
      last_used_page_offset = i;
    } else {
      // no more user data
      break;
    }
  }

  // read the last page that has actual data
  flash_read(grid_d51_nvm_state.flash,
             GRID_D51_NVM_LOCAL_BASE_ADDRESS +
                 last_used_page_offset * GRID_D51_NVM_PAGE_OFFSET,
             flash_read_buffer, GRID_D51_NVM_PAGE_SIZE);

  for (uint32_t i = 0; i < GRID_D51_NVM_PAGE_SIZE;
       i += 8) { // +=8 because we want to keep the offset aligned

    if (flash_read_buffer[i] != 0xff) {
      last_used_byte_offset = i;
    } else {

      break;
    }
  }

  nvm->next_write_offset = last_used_page_offset * GRID_D51_NVM_PAGE_OFFSET +
                           last_used_byte_offset +
                           8; // +8 because we want to write to the next word
                              // after the last used word

  // Read through the whole valid configuration area and parse valid configs
  // into TOC

  uint8_t config_header[6] = {0};
  snprintf(config_header, 5, GRID_CLASS_CONFIG_frame_start);

  for (uint32_t i = 0; i <= last_used_page_offset;
       i++) { // <= because we want to check the last_used_page too

    CRITICAL_SECTION_ENTER()
    flash_read(nvm->flash,
               GRID_D51_NVM_LOCAL_BASE_ADDRESS + i * GRID_D51_NVM_PAGE_SIZE,
               flash_read_buffer, GRID_D51_NVM_PAGE_SIZE);
    CRITICAL_SECTION_LEAVE()

    for (uint16_t j = 0; j < GRID_D51_NVM_PAGE_SIZE; j++) {

      uint32_t current_offset = j + GRID_D51_NVM_PAGE_SIZE * i;

      if (flash_read_buffer[j] == GRID_CONST_STX) {

        uint8_t temp_buffer[33] = {0};

        uint8_t *current_header = temp_buffer;

        if (j > GRID_D51_NVM_PAGE_SIZE - 20) {
          // read from flash, because the whole header is not in the page

          CRITICAL_SECTION_ENTER()
          flash_read(nvm->flash,
                     GRID_D51_NVM_LOCAL_BASE_ADDRESS + current_offset,
                     temp_buffer, 32);
          CRITICAL_SECTION_LEAVE()

        } else {
          current_header = &flash_read_buffer[j];
        }

        if (0 == strncmp(current_header, config_header, 4)) {

          uint8_t page_number = 0;
          uint8_t element_number = 0;
          uint8_t event_type = 0;
          uint16_t config_length = 0;

          uint8_t vmajor = grid_msg_string_get_parameter(
              current_header, GRID_CLASS_CONFIG_VERSIONMAJOR_offset,
              GRID_CLASS_CONFIG_VERSIONMAJOR_length, NULL);
          uint8_t vminor = grid_msg_string_get_parameter(
              current_header, GRID_CLASS_CONFIG_VERSIONMINOR_offset,
              GRID_CLASS_CONFIG_VERSIONMINOR_length, NULL);
          uint8_t vpatch = grid_msg_string_get_parameter(
              current_header, GRID_CLASS_CONFIG_VERSIONPATCH_offset,
              GRID_CLASS_CONFIG_VERSIONPATCH_length, NULL);

          if (vmajor == GRID_PROTOCOL_VERSION_MAJOR &&
              vminor == GRID_PROTOCOL_VERSION_MINOR &&
              vpatch == GRID_PROTOCOL_VERSION_PATCH) {
            // version ok
          } else {
            // grid_port_debug_printf("error.nvm.config version mismatch\r\n");
          }

          page_number = grid_msg_string_get_parameter(
              current_header, GRID_CLASS_CONFIG_PAGENUMBER_offset,
              GRID_CLASS_CONFIG_PAGENUMBER_length, NULL);
          element_number = grid_msg_string_get_parameter(
              current_header, GRID_CLASS_CONFIG_ELEMENTNUMBER_offset,
              GRID_CLASS_CONFIG_ELEMENTNUMBER_length, NULL);
          event_type = grid_msg_string_get_parameter(
              current_header, GRID_CLASS_CONFIG_EVENTTYPE_offset,
              GRID_CLASS_CONFIG_EVENTTYPE_length, NULL);
          config_length = grid_msg_string_get_parameter(
              current_header, GRID_CLASS_CONFIG_ACTIONLENGTH_offset,
              GRID_CLASS_CONFIG_ACTIONLENGTH_length, NULL);

          if (config_length == 0) {

            // printf("\r\nLENGTH 0: %s\r\n\r\n", temp_buffer);
          }

          uint8_t frist_character =
              current_header[GRID_CLASS_CONFIG_ACTIONSTRING_offset];

          if (frist_character == GRID_CONST_ETX) {
            // printf("\r\nETX -> Default config marker!! \r\n\r\n");
            //  this is default config
            struct grid_d51_nvm_toc_entry *entry = NULL;
            entry = grid_d51_nvm_toc_entry_find(nvm, page_number,
                                                element_number, event_type);

            if (entry != NULL) {
              grid_d51_nvm_toc_entry_remove(nvm, entry);
            } else {
            }
          } else {

            grid_d51_nvm_toc_entry_create(&grid_d51_nvm_state, page_number,
                                          element_number, event_type,
                                          current_offset, config_length);
          }
        }
      }
    }
  }

  // create template buffers for all of the pages

  uint8_t pagecount = 4; // by default allocate 4 pages

  struct grid_d51_nvm_toc_entry *current = nvm->toc_head;

  while (current != NULL) {
    if (current->page_id > pagecount) {
      pagecount = current->page_id;
    }
    current = current->next;
  }

  for (uint8_t i = 0; i < grid_ui_state.element_list_length; i++) {

    struct grid_ui_element *ele = &grid_ui_state.element_list[i];

    uint8_t template_buffer_length = grid_ui_template_buffer_list_length(ele);

    while (template_buffer_length < pagecount) {

      grid_ui_template_buffer_create(ele);
      template_buffer_length = grid_ui_template_buffer_list_length(ele);
    }
  }
}

uint32_t grid_d51_nvm_config_mock(struct grid_d51_nvm_model *mod) {

  // generate random configuration

  uint8_t buf[300] = {0};
  uint16_t len = 0;

  uint8_t page_number = rand_sync_read8(&RAND_0) % 2;
  uint8_t element_number = rand_sync_read8(&RAND_0) % 3;
  uint8_t event_type = rand_sync_read8(&RAND_0) % 3;

  // append random length of fake actionstrings

  uint8_t mock_length =
      rand_sync_read8(&RAND_0) % 128; // 0...127 is the mock length

  for (uint8_t i = 0; i < mock_length; i++) {

    buf[len + i] = 'a' + i % 26;
  }

  grid_d51_nvm_config_store(mod, page_number, element_number, event_type, buf);
}

uint32_t grid_d51_nvm_config_store(struct grid_d51_nvm_model *nvm,
                                   uint8_t page_number, uint8_t element_number,
                                   uint8_t event_type, uint8_t *actionstring) {

  uint8_t buf[GRID_PARAMETER_ACTIONSTRING_maxlength + 100] = {0};

  uint16_t len = 0;

  sprintf(buf, GRID_CLASS_CONFIG_frame_start);

  grid_msg_string_set_parameter(buf, GRID_CLASS_CONFIG_VERSIONMAJOR_offset,
                                GRID_CLASS_CONFIG_VERSIONMAJOR_length,
                                GRID_PROTOCOL_VERSION_MAJOR, NULL);
  grid_msg_string_set_parameter(buf, GRID_CLASS_CONFIG_VERSIONMINOR_offset,
                                GRID_CLASS_CONFIG_VERSIONMINOR_length,
                                GRID_PROTOCOL_VERSION_MINOR, NULL);
  grid_msg_string_set_parameter(buf, GRID_CLASS_CONFIG_VERSIONPATCH_offset,
                                GRID_CLASS_CONFIG_VERSIONPATCH_length,
                                GRID_PROTOCOL_VERSION_PATCH, NULL);

  grid_msg_string_set_parameter(buf, GRID_CLASS_CONFIG_PAGENUMBER_offset,
                                GRID_CLASS_CONFIG_PAGENUMBER_length,
                                page_number, NULL);
  grid_msg_string_set_parameter(buf, GRID_CLASS_CONFIG_ELEMENTNUMBER_offset,
                                GRID_CLASS_CONFIG_ELEMENTNUMBER_length,
                                element_number, NULL);
  grid_msg_string_set_parameter(buf, GRID_CLASS_CONFIG_EVENTTYPE_offset,
                                GRID_CLASS_CONFIG_EVENTTYPE_length, event_type,
                                NULL);

  len = strlen(buf);

  strcpy(&buf[len], actionstring);

  len = strlen(buf);

  sprintf(&buf[len], GRID_CLASS_CONFIG_frame_end);

  len = strlen(buf);

  uint16_t config_length = len;

  grid_msg_string_set_parameter(buf, GRID_CLASS_CONFIG_ACTIONLENGTH_offset,
                                GRID_CLASS_CONFIG_ACTIONLENGTH_length,
                                config_length, NULL);

  // printf("Config frame len: %d -> %s\r\n", len, buf);

  grid_d51_nvm_toc_debug(nvm);

  struct grid_d51_nvm_toc_entry *entry = NULL;

  entry = grid_d51_nvm_toc_entry_find(&grid_d51_nvm_state, page_number,
                                      element_number, event_type);

  uint32_t append_offset = grid_d51_nvm_append(nvm, buf, config_length);

  if (strlen(actionstring) == 0) {
    // printf("STORE: No Need To Create Toc (Default) \r\n");
    grid_d51_nvm_toc_entry_remove(nvm, entry);
  } else {

    if (entry == NULL) {

      // printf("NEW\r\n");
      grid_d51_nvm_toc_entry_create(&grid_d51_nvm_state, page_number,
                                    element_number, event_type, append_offset,
                                    config_length);

    } else {

      // printf("UPDATE %d %d %d :  0x%x to 0x%x %d\r\n", entry->page_id,
      // entry->element_id, entry->event_type, entry->config_string_offset,
      // append_offset, config_length);

      // actually we don't want to clear previous, to preserve config history
      // grid_d51_nvm_clear(mod, entry->config_string_offset,
      // entry->config_string_length);

      grid_d51_nvm_toc_entry_update(entry, append_offset, config_length);
    }
  }
  grid_d51_nvm_toc_debug(nvm);
}

uint8_t grid_d51_nvm_toc_entry_create(struct grid_d51_nvm_model *mod,
                                      uint8_t page_id, uint8_t element_id,
                                      uint8_t event_type,
                                      uint32_t config_string_offset,
                                      uint16_t config_string_length) {

  struct grid_d51_nvm_toc_entry *prev = NULL;
  struct grid_d51_nvm_toc_entry *next = mod->toc_head;
  struct grid_d51_nvm_toc_entry *entry = NULL;

  uint32_t this_sort = (page_id << 16) + (element_id << 8) + (event_type);

  uint8_t duplicate = 0;

  while (1) {

    uint32_t next_sort = -1;

    if (next != NULL) {

      next_sort =
          (next->page_id << 16) + (next->element_id << 8) + (next->event_type);
    }

    if (next == NULL) {
      // this is the end of the list
      // printf("A)\r\n");
      break;
    } else if (next_sort > this_sort) {
      // printf("B)\r\n");

      break;
    } else if (next_sort == this_sort) {
      // printf("C)\r\n");
      duplicate = 1;
      break;
    } else {
      // printf("next)\r\n");
      prev = next;
      next = next->next;
    }
  }

  if (duplicate == 0) {

    entry = malloc(sizeof(struct grid_d51_nvm_toc_entry));
  } else {
    entry = next;
  }

  // if new element was successfully created
  if (entry != NULL) {

    if (duplicate == 0) { // not duplicate
      mod->toc_count++;

      entry->next = next;
      entry->prev = prev;

      if (prev == NULL) {
        // this is first element
        mod->toc_head = entry;
        // printf("toc head\r\n");
      } else {
        prev->next = entry;
      }

      if (next != NULL) {
        next->prev = entry;
      }
    }

    entry->status = 1;

    entry->page_id = page_id;
    entry->element_id = element_id;
    entry->event_type = event_type;
    entry->config_string_offset = config_string_offset;
    entry->config_string_length = config_string_length;

    // printf("toc create: %d %d %d offs: 0x%x len: %d\r\n", page_id,
    // element_id, event_type, entry->config_string_offset,
    // entry->config_string_length);

    // here manipulate NVM if duplicate

  } else {

    printf("error.nvm.malloc failed\r\n");
    return 255;
  }
}

struct grid_d51_nvm_toc_entry *
grid_d51_nvm_toc_entry_find(struct grid_d51_nvm_model *mod, uint8_t page_id,
                            uint8_t element_id, uint8_t event_type) {

  struct grid_d51_nvm_toc_entry *current = mod->toc_head;

  while (current != NULL) {
    if (current->page_id == page_id && current->element_id == element_id &&
        current->event_type == event_type) {

      // Found the list item that we were looking for

      return current;
    }

    current = current->next;
  }

  return NULL;
}

uint8_t grid_d51_nvm_toc_entry_update(struct grid_d51_nvm_toc_entry *entry,
                                      uint32_t config_string_offset,
                                      uint16_t config_string_length) {

  // printf("UPDATE: %d -> %d\r\n", entry->config_string_offset,
  // config_string_offset);

  entry->config_string_offset = config_string_offset;
  entry->config_string_length = config_string_length;
}

uint8_t grid_d51_nvm_toc_entry_destroy(struct grid_d51_nvm_model *nvm,
                                       struct grid_d51_nvm_toc_entry *entry) {

  if (entry == NULL) {
    return -1;
  }

  // don't clear
  // grid_d51_nvm_clear(nvm, entry->config_string_offset,
  // entry->config_string_length);

  // append a new empty config to signal that it was cleard
  grid_d51_nvm_config_store(nvm, entry->page_id, entry->element_id,
                            entry->event_type, "");

  return 0;
}

uint8_t grid_d51_nvm_toc_entry_remove(struct grid_d51_nvm_model *nvm,
                                      struct grid_d51_nvm_toc_entry *entry) {

  if (entry == NULL) {
    return -1;
  }

  if (nvm->toc_count > 0) {
    nvm->toc_count--;
  }

  if (entry->prev != NULL) {
    entry->prev->next = entry->next;
  } else {
    nvm->toc_head = entry->next;
  }

  if (entry->next != NULL) {
    entry->next->prev = entry->prev;
  }

  free(entry);

  return 0;
}

void grid_d51_nvm_toc_debug(struct grid_d51_nvm_model *mod) {

  // return; // degub disabled

  struct grid_d51_nvm_toc_entry *next = mod->toc_head;

  printf("DUMP START\r\n");

  while (next != NULL) {

    printf("toc entry: %d %d %d :  0x%x %d\r\n", next->page_id,
           next->element_id, next->event_type, next->config_string_offset,
           next->config_string_length);

    next = next->next;
  }

  printf("DUMP DONE\r\n");
}

uint32_t
grid_d51_nvm_toc_generate_actionstring(struct grid_d51_nvm_model *nvm,
                                       struct grid_d51_nvm_toc_entry *entry,
                                       char *targetstring) {

  // -GRID_CLASS_CONFIG_ACTIONSTRING_offset to get rid of the config class
  // header
  CRITICAL_SECTION_ENTER()
  flash_read(nvm->flash,
             GRID_D51_NVM_LOCAL_BASE_ADDRESS + entry->config_string_offset +
                 GRID_CLASS_CONFIG_ACTIONSTRING_offset,
             targetstring,
             entry->config_string_length -
                 GRID_CLASS_CONFIG_ACTIONSTRING_offset - 1); //-1 etx
  CRITICAL_SECTION_LEAVE()

  uint16_t length =
      entry->config_string_length - GRID_CLASS_CONFIG_ACTIONSTRING_offset - 1;

  targetstring[length] = '\0';

  for (uint16_t i = 0; i < length; i++) {
    if (targetstring[i] < 32 || targetstring[i] > 127) {

      printf("char: @%d is %d length: %d\r\n", i, targetstring[i], length);

      grid_port_debug_printf(
          "toc invalid g a %d --- char: @%d is %d length: %d",
          entry->config_string_length, i, targetstring[i], length);

      printf("toc invalid g a %d %s\r\n", entry->config_string_length,
             targetstring);

      break;
    }
  }

  // printf("toc g a %d %s\r\n", entry->config_string_length, targetstring);

  return strlen(targetstring);
}

uint8_t grid_d51_nvm_is_ready(struct grid_d51_nvm_model *nvm) {

  return hri_nvmctrl_get_STATUS_READY_bit(nvm->flash->dev.hw);
}
