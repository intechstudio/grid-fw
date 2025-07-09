#include "grid_ui.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "grid_config.h"
#include "grid_protocol.h"
#include "grid_transport.h"
#include "grid_usb.h"

extern void grid_platform_printf(char const* fmt, ...);
extern int grid_platform_find_actionstring_file(uint8_t page, uint8_t element, uint8_t event_type, union grid_ui_file_handle* file_handle);
extern uint16_t grid_platform_get_actionstring_file_size(union grid_ui_file_handle* file_handle);
extern uint32_t grid_platform_read_actionstring_file_contents(union grid_ui_file_handle* file_handle, char* targetstring);
extern void grid_platform_delete_actionstring_file(union grid_ui_file_handle* file_handle);
extern void grid_platform_write_actionstring_file(uint8_t page, uint8_t element, uint8_t event_type, char* buffer, uint16_t length);
extern int grid_platform_find_next_actionstring_file_on_page(uint8_t page, int* last_element, int* last_event, union grid_ui_file_handle* file_handle);
extern int grid_platform_find_file(char* path, union grid_ui_file_handle* file_handle);
extern uint16_t grid_platform_get_file_size(union grid_ui_file_handle* file_handle);
extern int grid_platform_read_file(union grid_ui_file_handle* file_handle, uint8_t* buffer, uint16_t size);
extern int grid_platform_write_file(char* path, uint8_t* buffer, uint16_t size);
extern int grid_platform_delete_file(union grid_ui_file_handle* file_handle);
extern uint32_t grid_platform_get_cycles();
extern uint32_t grid_platform_get_cycles_per_us();
extern void grid_platform_clear_all_actionstring_files_from_page(uint8_t page);
extern void grid_platform_delete_actionstring_files_all();
extern uint8_t grid_platform_get_nvm_state();
extern uint8_t grid_platform_erase_nvm_next();
extern uint8_t grid_platform_get_adc_bit_depth();

extern const struct luaL_Reg* grid_lua_api_generic_lib_reference;

void grid_ui_semaphore_init(struct grid_ui_semaphore* semaphore, void* handle, void (*lock_fn)(void*), void (*release_fn)(void*)) {

  semaphore->handle = handle;
  semaphore->lock_fn = lock_fn;
  semaphore->release_fn = release_fn;
}

void grid_ui_semaphore_lock(struct grid_ui_semaphore* semaphore) {

  if (semaphore->handle == NULL) {
    return;
  }

  if (semaphore->lock_fn == NULL) {
    return;
  }

  semaphore->lock_fn(semaphore->handle);
}

void grid_ui_semaphore_release(struct grid_ui_semaphore* semaphore) {

  if (semaphore->handle == NULL) {
    return;
  }

  if (semaphore->release_fn == NULL) {
    return;
  }

  semaphore->release_fn(semaphore->handle);
}

struct grid_ui_model grid_ui_state;

void grid_ui_model_init(struct grid_ui_model* ui, uint8_t element_list_length) {

  ui->lua_ui_init_callback = NULL;

  ui->page_activepage = 0;
  ui->page_count = 4;

  ui->page_change_enabled = 1;

  ui->element_list_length = element_list_length;

  ui->element_list = grid_platform_allocate_volatile(element_list_length * sizeof(struct grid_ui_element));

  ui->page_negotiated = 0;

  ui->mapmode_state = 0;

  ui->bulk_status = GRID_UI_BULK_READY;
  ui->bulk_success_callback = NULL;
  ui->bulk_last_page = -1;
  ui->bulk_last_element = -1;
  ui->bulk_last_event = -1;
}

struct grid_ui_element* grid_ui_model_get_elements(struct grid_ui_model* ui) {

  assert(ui->element_list);

  return ui->element_list;
}

void grid_ui_busy_semaphore_lock(struct grid_ui_model* ui) { grid_ui_semaphore_lock(&ui->busy_semaphore); }

void grid_ui_busy_semaphore_release(struct grid_ui_model* ui) { grid_ui_semaphore_release(&ui->busy_semaphore); }

void grid_ui_bulk_semaphore_lock(struct grid_ui_model* ui) { grid_ui_semaphore_lock(&ui->bulk_semaphore); }

void grid_ui_bulk_semaphore_release(struct grid_ui_model* ui) { grid_ui_semaphore_release(&ui->bulk_semaphore); }

struct grid_ui_element* grid_ui_element_model_init(struct grid_ui_model* parent, uint8_t index) {

  if (parent == NULL) {
    return NULL;
  }

  if (index >= parent->element_list_length) {
    return NULL;
  }

  struct grid_ui_element* ele = &parent->element_list[index];

  ele->event_clear_cb = NULL;
  ele->page_change_cb = NULL;

  ele->parent = parent;
  ele->index = index;

  ele->template_parameter_list_length = 0;
  ele->template_parameter_list = NULL;

  ele->template_buffer_list_head = NULL;
  ele->template_parameter_element_position_index_1 = 0;
  ele->template_parameter_element_position_index_2 = 0;

  ele->timer_event_helper = 0;
  ele->timer_source_is_midi = 0;

  return ele;
}

void grid_ui_event_init(struct grid_ui_element* ele, uint8_t index, uint8_t event_type, char* function_name, const char* default_actionstring) {

  assert(index < ele->event_list_length);

  struct grid_ui_event* eve = &ele->event_list[index];

  eve->parent = ele;
  eve->state = GRID_EVE_STATE_INIT;
  eve->default_actionstring = default_actionstring;
  eve->cfg_changed_flag = 0;
  eve->cfg_default_flag = 1;
  eve->type = event_type;
  strcpy(eve->function_name, function_name);
}

void grid_ui_rtc_ms_tick_time(struct grid_ui_model* ui) {

  for (uint8_t i = 0; i < ui->element_list_length; i++) {

    struct grid_ui_element* ele = &ui->element_list[i];

    if (ele->timer_source_is_midi == 0) {

      if (ele->timer_event_helper > 0) {

        ele->timer_event_helper--;

        if (ele->timer_event_helper == 0) {

          struct grid_ui_event* eve = grid_ui_event_find(ele, GRID_PARAMETER_EVENT_TIMER);

          if (eve != NULL) {

            grid_ui_event_state_set(eve, GRID_EVE_STATE_TRIG);
          }
        }
      }
    }
  }
}

void grid_ui_midi_sync_tick_time(struct grid_ui_model* ui) {

  for (uint8_t i = 0; i < ui->element_list_length; i++) {

    struct grid_ui_element* ele = &ui->element_list[i];

    if (ele->timer_source_is_midi != 0) {

      if (ele->timer_event_helper > 0) {

        ele->timer_event_helper--;

        if (ele->timer_event_helper == 0) {

          struct grid_ui_event* eve = grid_ui_event_find(ele, GRID_PARAMETER_EVENT_TIMER);

          if (eve != NULL) {

            grid_ui_event_state_set(eve, GRID_EVE_STATE_TRIG);
          }
        }
      }
    }
  }
}

void grid_ui_rtc_ms_mapmode_handler(struct grid_ui_model* ui, uint8_t new_mapmode_value) {

  if (new_mapmode_value != ui->mapmode_state) {

    ui->mapmode_state = new_mapmode_value;

    if (ui->mapmode_state == 0) { // RELEASE
    } else {                      // PRESS

      struct grid_ui_element* sys_ele = &grid_ui_state.element_list[grid_ui_state.element_list_length - 1];

      struct grid_ui_event* eve = grid_ui_event_find(sys_ele, GRID_PARAMETER_EVENT_MAPMODE);

      if (eve == NULL) {
      } else {
      }

      grid_ui_event_state_set(eve, GRID_EVE_STATE_TRIG);
    }
  }
}

struct grid_ui_template_buffer* grid_ui_template_buffer_create(struct grid_ui_element* ele) {

  struct grid_ui_template_buffer* this = grid_platform_allocate_volatile(sizeof(struct grid_ui_template_buffer));

  if (!this) {
    grid_platform_printf("grid_ui_template_buffer_create malloc failed 1\r\n");
    return NULL;
  }

  this->parent = ele;
  this->next = NULL;

  uint8_t alloc_len = ele->template_parameter_list_length;

  // Always allocate at least one element
  alloc_len = alloc_len ? alloc_len : 1;

  this->template_parameter_list = grid_platform_allocate_volatile(alloc_len * sizeof(int32_t));

  if (!this->template_parameter_list) {
    grid_platform_printf("grid_ui_template_buffer_create malloc failed 2\r\n");
    return NULL;
  }

  if (ele->template_initializer) {
    ele->template_initializer(this);
  }

  struct grid_ui_template_buffer* prev = ele->template_buffer_list_head;

  if (prev) {

    while (prev->next) {

      prev = prev->next;
    }

    prev->next = this;

  } else {

    this->parent->template_buffer_list_head = this;
  }

  return this;
}

uint8_t grid_ui_template_buffer_list_length(struct grid_ui_element* ele) {

  uint8_t count = 0;

  struct grid_ui_template_buffer* this = ele->template_buffer_list_head;

  while (this != NULL) {

    ++count;
    this = this->next;
  }

  return count;
}

struct grid_ui_template_buffer* grid_ui_template_buffer_find(struct grid_ui_element* ele, uint8_t page) {

  uint8_t count = 0;

  struct grid_ui_template_buffer* this = ele->template_buffer_list_head;

  while (this != NULL) {

    if (count == page) {
      return this;
    }

    count++;
    this = this->next;
  }

  return NULL;
}

uint8_t grid_ui_page_get_activepage(struct grid_ui_model* ui) { return ui->page_activepage; }

uint8_t grid_ui_page_get_next(struct grid_ui_model* ui) { return (ui->page_activepage + 1) % ui->page_count; }

uint8_t grid_ui_page_get_prev(struct grid_ui_model* ui) { return (ui->page_activepage + ui->page_count - 1) % ui->page_count; }

void grid_ui_page_load(struct grid_ui_model* ui, uint8_t page) {

  grid_ui_bulk_semaphore_lock(ui);
  grid_ui_busy_semaphore_lock(ui);

  // Reset all state parameters of all leds
  grid_led_reset(&grid_led_state);

  // Store old page index for the page change callback
  uint8_t oldpage = ui->page_activepage;

  // Set active page
  ui->page_activepage = page;

  for (uint8_t i = 0; i < ui->element_list_length; ++i) {

    struct grid_ui_element* ele = grid_ui_element_find(ui, i);

    if (ele == NULL) {
      grid_platform_printf("grid_ui_page_load ele NULL\r\n");
    }

    // Stop the event's timer
    ele->timer_event_helper = 0;

    // Clear all pending/triggered events for the element
    for (uint8_t j = 0; j < ele->event_list_length; j++) {
      grid_ui_event_state_set(&ele->event_list[j], GRID_EVE_STATE_INIT);
    }

    uint8_t template_buffer_length = grid_ui_template_buffer_list_length(ele);

    // Create new template buffers until they cover the index of the loaded page
    while (template_buffer_length < page + 1) {

      if (!grid_ui_template_buffer_create(ele)) {
        grid_platform_printf("grid_ui_page_load failed to create template buffer\r\n");
      }

      template_buffer_length = grid_ui_template_buffer_list_length(ele);
    }

    struct grid_ui_template_buffer* buf = grid_ui_template_buffer_find(ele, page);

    if (buf) {

      // Load the template parameter list
      ele->template_parameter_list = buf->template_parameter_list;

      if (!buf->template_parameter_list) {
        grid_port_debug_print_text("grid_ui_page_load buf template param list NULL");
      }

    } else {

      grid_platform_printf("grid_ui_page_load template buffer is invalid\r\n");
      grid_port_debug_print_text("grid_ui_page_load template buffer is invalid");
    }

    // Invoke page change callback if necessary
    if (ele->page_change_cb) {
      ele->page_change_cb(ele, oldpage, page);
    }
  }

  // Restart VM, register functions
  grid_lua_stop_vm(&grid_lua_state);
  grid_lua_start_vm(&grid_lua_state);
  grid_lua_vm_register_functions(&grid_lua_state, grid_lua_api_generic_lib_reference);

  // Invoke lua UI init callback
  grid_lua_ui_init(&grid_lua_state, grid_ui_state.lua_ui_init_callback);

  grid_ui_bulk_semaphore_release(ui);
  grid_ui_busy_semaphore_release(ui);

  grid_ui_bulk_page_init(ui, GRID_UI_BULK_READ_PROGRESS, grid_ui_page_get_activepage(&grid_ui_state), 0, grid_ui_page_load_success_callback);
}

void grid_ui_page_load_success_callback(uint8_t lastheader) {

  grid_lua_post_init(&grid_lua_state);

  grid_usb_keyboard_enable(&grid_usb_keyboard_state);

  // Phase out the animation
  grid_alert_all_set_timeout_automatic(&grid_led_state);
}

void grid_ui_page_clear_template_parameters(struct grid_ui_model* ui, uint8_t page) {

  for (uint8_t i = 0; i < ui->element_list_length; i++) {

    struct grid_ui_element* ele = &ui->element_list[i];
    struct grid_ui_template_buffer* buf = grid_ui_template_buffer_find(ele, page);

    if (ele->template_initializer) {
      ele->template_initializer(buf);
    }
  }
}

uint8_t grid_ui_page_change_is_enabled(struct grid_ui_model* ui) { return ui->page_change_enabled; }

uint8_t grid_ui_event_isdefault_actionstring(struct grid_ui_event* eve, char* action_string) { return strcmp(action_string, eve->default_actionstring) == 0; }

void grid_ui_actionstring_header(uint8_t index, char* function_name, char* dest) {

  char fn_push[] = "local _efn = EFN; EFN = ";

  sprintf(dest, "ele[%d].%s = function (self) %s\"%s\"; ", index, function_name, fn_push, function_name);
}

void grid_ui_actionstring_center(char* actionstring, char* dest) { sprintf(dest, "%s ", &actionstring[6]); }

void grid_ui_actionstring_footer(char* dest) {

  char fn_pop[] = "EFN = _efn";

  sprintf(dest, "end %s", fn_pop);
}

void grid_ui_event_register_actionstring(struct grid_ui_event* eve, char* action_string) {

  struct grid_ui_element* ele = eve->parent;

  uint32_t len = strlen(action_string);

  if (len == 0) {
    grid_platform_printf("NULLSTRING el:%d, elv:%d\r\n", ele->index, eve->type);
    return;
  }

  char temp[GRID_PARAMETER_ACTIONSTRING_maxlength + 100] = {0};

  action_string[len - 3] = '\0';
  grid_ui_actionstring_header(ele->index, eve->function_name, temp);
  grid_ui_actionstring_center(action_string, &temp[strlen(temp)]);
  grid_ui_actionstring_footer(&temp[strlen(temp)]);
  action_string[len - 3] = ' ';

  eve->cfg_default_flag = grid_ui_event_isdefault_actionstring(eve, action_string);

  if (0 == grid_lua_dostring(&grid_lua_state, temp)) {
    grid_port_debug_printf("LUA not OK, Failed to register action! EL: %d EV: %d", ele->index, eve->type);
  };

  eve->cfg_changed_flag = 1;
}

uint32_t grid_ui_event_render_event(struct grid_ui_event* eve, char* target_string) {

  uint8_t page = eve->parent->parent->page_activepage;
  uint8_t element = eve->parent->index;
  uint8_t event = eve->type;
  uint8_t param1 = 0;
  uint8_t param2 = 0;

  if (eve->parent->template_parameter_list != NULL) {
    param1 = eve->parent->template_parameter_list[eve->parent->template_parameter_element_position_index_1];
    param2 = eve->parent->template_parameter_list[eve->parent->template_parameter_element_position_index_2];
  }

  // map mapmode to element 255
  if (eve->parent->type == GRID_PARAMETER_ELEMENT_SYSTEM) {
    element = 255;
  }

  sprintf(target_string, GRID_CLASS_EVENT_frame);

  grid_str_set_parameter(target_string, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code, NULL);

  grid_str_set_parameter(target_string, GRID_CLASS_EVENT_PAGENUMBER_offset, GRID_CLASS_EVENT_PAGENUMBER_length, page, NULL);
  grid_str_set_parameter(target_string, GRID_CLASS_EVENT_ELEMENTNUMBER_offset, GRID_CLASS_EVENT_ELEMENTNUMBER_length, element, NULL);
  grid_str_set_parameter(target_string, GRID_CLASS_EVENT_EVENTTYPE_offset, GRID_CLASS_EVENT_EVENTTYPE_length, event, NULL);
  grid_str_set_parameter(target_string, GRID_CLASS_EVENT_EVENTPARAM1_offset, GRID_CLASS_EVENT_EVENTPARAM1_length, param1, NULL);
  grid_str_set_parameter(target_string, GRID_CLASS_EVENT_EVENTPARAM2_offset, GRID_CLASS_EVENT_EVENTPARAM2_length, param2, NULL);

  return strlen(target_string);
}

void grid_ui_event_generate_actionstring(struct grid_ui_event* eve, char* targetstring) { strcpy(targetstring, eve->default_actionstring); }

void grid_ui_event_get_actionstring(struct grid_ui_event* eve, char* targetstring) {

  char temp[100] = {0};
  char result[GRID_PARAMETER_ACTIONSTRING_maxlength + 100] = {0};

  sprintf(temp, "gsg(%ld,debug.getinfo(ele[%d].%s,\"S\").source)", (uint32_t)result, eve->parent->index, eve->function_name);

  if (0 == grid_lua_dostring(&grid_lua_state, temp)) {
    grid_port_debug_printf("LUA not OK, Failed to retrieve action! EL: %d EV: %d", eve->parent->index, eve->type);
  };

  char header[100] = {0};
  char footer[100] = {0};

  grid_ui_actionstring_header(eve->parent->index, eve->function_name, header);
  grid_ui_actionstring_footer(footer);

  size_t header_len = strlen(header);
  size_t footer_len = strlen(footer);

  // Check if debug.getinfo is valid by checking for a known prefix
  if (0 == strncmp(header, result, header_len)) {

    // Transform result to an actionstring
    sprintf(&result[strlen(result) - footer_len], " ?>");
    uint8_t offset = header_len - 6;
    memcpy(&result[offset], "<?lua ", 6);

    strcpy(targetstring, &result[offset]);

  } else {

    grid_ui_event_generate_actionstring(eve, targetstring);
    grid_platform_printf("ERROR: invalid debug.getinfo\r\n");
  }
}

uint32_t grid_ui_event_render_action(struct grid_ui_event* eve, char* target_string) {

  char temp[GRID_PARAMETER_ACTIONSTRING_maxlength + 100] = {0};

  sprintf(temp, "ele[%d]:%s(self)", eve->parent->index, eve->function_name);

  if (0 == grid_lua_dostring(&grid_lua_state, temp)) {

    char* stde = grid_lua_get_error_string(&grid_lua_state);
    grid_port_debug_printf("LUA not OK! EL: %d EV: %d MSG: %s", eve->parent->index, eve->type, stde);
    grid_platform_printf("LUA not OK! EL: %d EV: %d MSG: %s\r\n", eve->parent->index, eve->type, stde);
    grid_lua_clear_stde(&grid_lua_state);
  }

  char* stdo = grid_lua_get_output_string(&grid_lua_state);
  strcpy(target_string, stdo);
  grid_lua_clear_stdo(&grid_lua_state);

  grid_lua_clear_stde(&grid_lua_state);

  // Call the event clear callback
  if (eve->parent->event_clear_cb) {

    eve->parent->event_clear_cb(eve);
  }

  return strlen(target_string);
}

int grid_ui_event_recall_configuration(struct grid_ui_model* ui, uint8_t page, uint8_t element, uint8_t event_type, char* targetstring) {

  if (grid_ui_bulk_anything_is_in_progress(ui)) {
    grid_platform_printf("FETCH WHILE NVM BUSY\r\n");
    return 1;
  }

  struct grid_ui_element* ele = &ui->element_list[element];

  struct grid_ui_event* eve = grid_ui_event_find(ele, event_type);

  if (eve == NULL) {
    grid_platform_printf("warning." __FILE__ ".event does not exist!\r\n");
    return 1;
  }

  if (ui->page_activepage == page) {

    if (eve->cfg_default_flag) {

      grid_ui_event_generate_actionstring(eve, targetstring);

    } else {

      grid_ui_event_get_actionstring(eve, targetstring);
    }

  } else {

    union grid_ui_file_handle file_handle = {0};
    int status = grid_platform_find_actionstring_file(page, element, event_type, &file_handle);

    if (status == 0) {

      grid_platform_read_actionstring_file_contents(&file_handle, targetstring);

    } else {

      grid_ui_event_generate_actionstring(eve, targetstring);
    }
  }

  return 0;
}

uint8_t grid_ui_event_istriggered(struct grid_ui_event* eve) { return eve != NULL && eve->state == GRID_EVE_STATE_TRIG; }

uint8_t grid_ui_event_istriggered_local(struct grid_ui_event* eve) { return eve != NULL && eve->state == GRID_EVE_STATE_TRIG_LOCAL; }

uint16_t grid_ui_event_count_istriggered(struct grid_ui_model* ui) {

  uint16_t count = 0;

  for (uint8_t j = 0; j < ui->element_list_length; ++j) {

    for (uint8_t k = 0; k < ui->element_list[j].event_list_length; ++k) {

      if (grid_ui_event_istriggered(&ui->element_list[j].event_list[k])) {

        count++;
      }
    }
  }

  return count;
}

uint16_t grid_ui_event_count_istriggered_local(struct grid_ui_model* ui) {

  uint16_t count = 0;

  for (uint8_t j = 0; j < ui->element_list_length; ++j) {

    for (uint8_t k = 0; k < ui->element_list[j].event_list_length; ++k) {

      if (grid_ui_event_istriggered_local(&ui->element_list[j].event_list[k])) {

        count++;
      }
    }
  }
  return count;
}

struct grid_ui_element* grid_ui_element_find(struct grid_ui_model* ui, uint8_t element_number) {

  if (element_number < ui->element_list_length) {

    return &ui->element_list[element_number];

  } else {

    return NULL;
  }
}

void grid_ui_element_malloc_events(struct grid_ui_element* ele, int capacity) {

  assert(capacity > 0);

  ele->event_list = grid_platform_allocate_volatile(capacity * sizeof(struct grid_ui_event));
  assert(ele->event_list);

  ele->event_list_length = capacity;
}

void grid_ui_element_timer_set(struct grid_ui_element* ele, uint32_t duration) { ele->timer_event_helper = duration; }

void grid_ui_element_timer_source(struct grid_ui_element* ele, uint8_t source) { ele->timer_source_is_midi = source; }

void grid_ui_element_set_template_parameter(struct grid_ui_element* ele, uint8_t template_index, int32_t value) {

  if (template_index < ele->template_parameter_list_length) {

    ele->template_parameter_list[template_index] = value;
  }
}

int32_t grid_ui_element_get_template_parameter(struct grid_ui_element* ele, uint8_t template_index) {

  if (template_index < ele->template_parameter_list_length) {

    return ele->template_parameter_list[template_index];

  } else {

    return 0;
  }
}

enum grid_ui_bulk_status_t grid_ui_get_bulk_status(struct grid_ui_model* ui) { return ui->bulk_status; }

int grid_ui_bulk_anything_is_in_progress(struct grid_ui_model* ui) { return ui->bulk_status != GRID_UI_BULK_READY; }

int grid_ui_bulk_is_in_progress(struct grid_ui_model* ui, enum grid_ui_bulk_status_t bulk_status) { return ui->bulk_status == bulk_status; }

uint8_t grid_ui_bulk_get_lastheader(struct grid_ui_model* ui) { return ui->bulk_lastheader_id; }

int grid_ui_bulk_page_init(struct grid_ui_model* ui, enum grid_ui_bulk_status_t status, uint8_t page, uint8_t lastheader_id, void (*success_cb)(uint8_t)) {

  if (status != GRID_UI_BULK_READ_PROGRESS && status != GRID_UI_BULK_STORE_PROGRESS && status != GRID_UI_BULK_CLEAR_PROGRESS) {
    return 1;
  }

  grid_platform_printf("NVM: Page init\r\n");
  // update lastheader_id even if busy (during retry)
  ui->bulk_lastheader_id = lastheader_id;

  if (grid_ui_bulk_anything_is_in_progress(ui)) {
    return 1;
  }

  grid_ui_bulk_semaphore_lock(ui);

  ui->bulk_status = status;

  ui->bulk_success_callback = success_cb;

  ui->bulk_last_page = page;
  ui->bulk_last_element = -1;
  ui->bulk_last_event = -1;

  grid_ui_bulk_semaphore_release(ui);

  return 0;
}

int grid_ui_bulk_conf_init(struct grid_ui_model* ui, enum grid_ui_bulk_status_t status, uint8_t lastheader_id, void (*success_cb)(uint8_t)) {

  if (status != GRID_UI_BULK_CONFREAD_PROGRESS && status != GRID_UI_BULK_CONFSTORE_PROGRESS) {
    return 1;
  }

  // update lastheader_id even if busy (during retry)
  ui->bulk_lastheader_id = lastheader_id;

  if (grid_ui_bulk_anything_is_in_progress(ui)) {
    return 1;
  }

  grid_platform_printf("NVM: Config init\r\n");

  grid_ui_bulk_semaphore_lock(ui);

  ui->bulk_status = status;

  ui->bulk_success_callback = success_cb;

  grid_ui_bulk_semaphore_release(ui);

  return 0;
}

int grid_ui_bulk_nvmerase_init(struct grid_ui_model* ui, uint8_t lastheader_id, void (*success_cb)(uint8_t)) {

  grid_platform_printf("NVM: Erase init\r\n");
  // update lastheader_id even if busy (during retry)
  ui->bulk_lastheader_id = lastheader_id;

  if (grid_ui_bulk_anything_is_in_progress(ui)) {
    return 1;
  }

  grid_ui_bulk_semaphore_lock(ui);

  ui->bulk_status = GRID_UI_BULK_ERASE_PROGRESS;

  ui->bulk_success_callback = success_cb;
  ui->bulk_last_page = -1;
  ui->bulk_last_element = -1;
  ui->bulk_last_event = -1;

  grid_ui_bulk_semaphore_release(ui);

  return 0;
}

void grid_ui_bulk_pageread_next(struct grid_ui_model* ui) {

  grid_platform_printf("NVM: Read next\r\n");
  if (!grid_ui_bulk_is_in_progress(ui, GRID_UI_BULK_READ_PROGRESS)) {
    return;
  }

  if (!grid_platform_get_nvm_state()) {
    return;
  }

  grid_ui_busy_semaphore_lock(ui);
  grid_ui_bulk_semaphore_lock(ui);

  // step 1: mark all action strings as default
  if (ui->bulk_last_element == -1 && ui->bulk_last_event == -1) {

    for (uint8_t i = 0; i < ui->element_list_length; i++) {

      struct grid_ui_element* ele = &ui->element_list[i];

      for (uint8_t j = 0; j < ele->event_list_length; j++) {

        struct grid_ui_event* eve = &ele->event_list[j];

        eve->cfg_default_flag = 1;
      }
    }
  }

  // step 2: we register all custom actionstring files from FS or TOC
  union grid_ui_file_handle file_handle = {0};
  uint8_t was_last_one = grid_platform_find_next_actionstring_file_on_page(ui->bulk_last_page, &ui->bulk_last_element, &ui->bulk_last_event, &file_handle);

  if (!was_last_one) {

    struct grid_ui_event* eve = grid_ui_event_find(&ui->element_list[ui->bulk_last_element], ui->bulk_last_event);
    uint16_t size = grid_platform_get_actionstring_file_size(&file_handle);

    if (size > 0) {
      char temp[GRID_PARAMETER_ACTIONSTRING_maxlength + 100] = {0};

      grid_platform_read_actionstring_file_contents(&file_handle, temp);

      grid_ui_event_register_actionstring(eve, temp);
      grid_platform_printf("Ele:%d Eve:%d\r\n", ui->bulk_last_element, ui->bulk_last_event);
      eve->cfg_changed_flag = 0; // clear changed flag
    }

    grid_ui_bulk_semaphore_release(ui);
    grid_ui_busy_semaphore_release(ui);
    return;
  }

  // step 3: fill all of the remaining default events with default actionstrings
  for (uint8_t i = 0; i < ui->element_list_length; i++) {

    struct grid_ui_element* ele = &ui->element_list[i];

    for (uint8_t j = 0; j < ele->event_list_length; j++) {

      struct grid_ui_event* eve = &ele->event_list[j];

      if (eve->cfg_default_flag == 1) {

        char temp[GRID_PARAMETER_ACTIONSTRING_maxlength + 100] = {0};

        grid_ui_event_generate_actionstring(eve, temp);
        grid_ui_event_register_actionstring(eve, temp);
      }
    }
  }

  // step 4: trigger all init events
  grid_lua_post_init(&grid_lua_state);

  // step 5: run the success callback if available

  // Set ready before callback so callback can start new nvm operation
  ui->bulk_status = GRID_UI_BULK_READY;

  grid_ui_bulk_semaphore_release(ui);
  grid_ui_busy_semaphore_release(ui);
  grid_platform_printf("NVM: Read done\r\n");

  if (ui->bulk_success_callback != NULL) {
    ui->bulk_success_callback(ui->bulk_lastheader_id);
    ui->bulk_success_callback = NULL;
  }

  return;
}

void grid_ui_bulk_pagestore_next(struct grid_ui_model* ui) {

  grid_platform_printf("NVM: Store next\r\n");
  if (!grid_ui_bulk_is_in_progress(ui, GRID_UI_BULK_STORE_PROGRESS)) {
    return;
  }

  if (!grid_platform_get_nvm_state()) {
    return;
  }

  grid_ui_busy_semaphore_lock(ui);
  grid_ui_bulk_semaphore_lock(ui);

  for (uint8_t i = 0; i < ui->element_list_length; i++) {

    struct grid_ui_element* ele = &ui->element_list[i];

    for (uint8_t j = 0; j < ele->event_list_length; j++) {

      struct grid_ui_event* eve = &ele->event_list[j];

      if (!eve->cfg_changed_flag) {
        continue;
      }

      if (eve->cfg_default_flag) {

        union grid_ui_file_handle file_handle = {0};
        int status = grid_platform_find_actionstring_file(ui->bulk_last_page, ele->index, eve->type, &file_handle);

        // File found
        if (status == 0) {
          grid_platform_delete_actionstring_file(&file_handle);
        }

      } else {

        char buffer[GRID_PARAMETER_ACTIONSTRING_maxlength + 100] = {0};
        grid_ui_event_get_actionstring(eve, buffer);

        grid_platform_write_actionstring_file(ui->bulk_last_page, ele->index, eve->type, buffer, strlen(buffer));
      }

      // Clear changed flag
      eve->cfg_changed_flag = 0;

      // Return from this function after the first successful store
      grid_ui_bulk_semaphore_release(ui);
      grid_ui_busy_semaphore_release(ui);
      return;
    }
  }

  // Execution will reach this point once every change was stored

  // Set ready before callback so callback can start new nvm operation
  ui->bulk_status = GRID_UI_BULK_READY;

  grid_ui_bulk_semaphore_release(ui);
  grid_ui_busy_semaphore_release(ui);
  grid_platform_printf("NVM: Store done\r\n");

  if (ui->bulk_success_callback != NULL) {

    ui->bulk_success_callback(ui->bulk_lastheader_id);
    ui->bulk_success_callback = NULL;
  }
}

void grid_ui_bulk_pageclear_next(struct grid_ui_model* ui) {

  grid_platform_printf("NVM: Clear next\r\n");
  if (!grid_ui_bulk_is_in_progress(ui, GRID_UI_BULK_CLEAR_PROGRESS)) {
    return;
  }

  if (!grid_platform_get_nvm_state()) {
    return;
  }

  grid_ui_busy_semaphore_lock(ui);
  grid_ui_bulk_semaphore_lock(ui);

  union grid_ui_file_handle file_handle = {0};
  uint8_t was_last_one = grid_platform_find_next_actionstring_file_on_page(ui->bulk_last_page, &ui->bulk_last_element, &ui->bulk_last_event, &file_handle);

  if (!was_last_one) {

    grid_platform_delete_actionstring_file(&file_handle);
    grid_ui_bulk_semaphore_release(ui);
    grid_ui_busy_semaphore_release(ui);
    return;
  }

  // Set ready before callback so callback can start new nvm operation
  ui->bulk_status = GRID_UI_BULK_READY;

  grid_platform_printf("NVM: Clear done\r\n");

  grid_ui_bulk_semaphore_release(ui);
  grid_ui_busy_semaphore_release(ui);

  if (ui->bulk_success_callback != NULL) {

    ui->bulk_success_callback(ui->bulk_lastheader_id);
    ui->bulk_success_callback = NULL;
  }
}

int confread_parse_from_file(struct grid_ui_model* ui) {

  int status;

  union grid_ui_file_handle file_handle = {0};

  status = grid_platform_find_file(GRID_UI_CONFIG_PATH, &file_handle);
  if (status) {
    grid_platform_printf("grid_platform_find_file returned %d\n", status);
    return 1;
  }

  uint16_t file_size = grid_platform_get_file_size(&file_handle);
  if (file_size == 0) {
    grid_platform_printf("grid_platform_get_file_size returned %d\n", status);
    return 1;
  }

  char* buffer = (char*)malloc(file_size);
  if (buffer == NULL) {
    grid_platform_printf("confread_parse_from_file malloc\n");
    return 1;
  }

  status = grid_platform_read_file(&file_handle, (uint8_t*)buffer, file_size);
  if (status) {
    grid_platform_printf("grid_platform_read_file returned %d\n", status);
    free(buffer);
    return 1;
  }

  status = grid_config_parse(&grid_config_state, buffer);
  if (status) {
    grid_platform_printf("grid_config_parse returned %d\n", status);
    free(buffer);
    return 1;
  }

  free(buffer);
  return 0;
}

void grid_ui_bulk_confread_next(struct grid_ui_model* ui) {

  grid_platform_printf("NVM: Read config next\r\n");
  if (!grid_ui_bulk_is_in_progress(ui, GRID_UI_BULK_CONFREAD_PROGRESS)) {
    return;
  }

  if (!grid_platform_get_nvm_state()) {
    return;
  }

  grid_ui_busy_semaphore_lock(ui);
  grid_ui_bulk_semaphore_lock(ui);

  /*int status = */ confread_parse_from_file(ui);

  ui->bulk_status = GRID_UI_BULK_READY;

  grid_ui_bulk_semaphore_release(ui);
  grid_ui_busy_semaphore_release(ui);
  grid_platform_printf("NVM: Read config done\r\n");

  if (ui->bulk_success_callback != NULL) {
    ui->bulk_success_callback(ui->bulk_lastheader_id);
    ui->bulk_success_callback = NULL;
  }
}

int confstore_generate_to_file(struct grid_ui_model* ui) {

  char* config = (char*)malloc(grid_config_bytes(&grid_config_state));
  if (config == NULL) {
    grid_platform_printf("confstore_generate_to_file malloc\n");
    return 1;
  }

  int status;

  status = grid_config_generate(&grid_config_state, config);
  if (status) {
    grid_platform_printf("grid_config_generate returned %d\n", status);
    free(config);
    return 1;
  }

  status = grid_platform_write_file(GRID_UI_CONFIG_PATH, (uint8_t*)config, strlen(config) + 1);
  if (status) {
    grid_platform_printf("grid_platform_write_file returned %d\n", status);
    free(config);
    return 1;
  }

  free(config);
  return 0;
}

void grid_ui_bulk_confstore_next(struct grid_ui_model* ui) {

  grid_platform_printf("NVM: Store config next\r\n");
  if (!grid_ui_bulk_is_in_progress(ui, GRID_UI_BULK_CONFSTORE_PROGRESS)) {
    return;
  }

  if (!grid_platform_get_nvm_state()) {
    return;
  }

  grid_ui_busy_semaphore_lock(ui);
  grid_ui_bulk_semaphore_lock(ui);

  /*int status = */ confstore_generate_to_file(ui);

  ui->bulk_status = GRID_UI_BULK_READY;

  grid_ui_bulk_semaphore_release(ui);
  grid_ui_busy_semaphore_release(ui);
  grid_platform_printf("NVM: Store config done\r\n");

  if (ui->bulk_success_callback != NULL) {
    ui->bulk_success_callback(ui->bulk_lastheader_id);
    ui->bulk_success_callback = NULL;
  }
}

void grid_ui_bulk_nvmerase_next(struct grid_ui_model* ui) {

  grid_platform_printf("NVM: Erase next\r\n");
  if (!grid_ui_bulk_is_in_progress(ui, GRID_UI_BULK_ERASE_PROGRESS)) {
    return;
  }

  if (!grid_platform_get_nvm_state()) {
    return;
  }

  grid_ui_busy_semaphore_lock(ui);
  grid_ui_bulk_semaphore_lock(ui);

  if (ui->bulk_last_page < 0) {
    ui->bulk_last_page = 0;
  }

  // STEP 1: Delete all actionstring files
  if (ui->bulk_last_page < ui->page_count) {

    union grid_ui_file_handle file_handle = {0};
    uint8_t was_last_one = grid_platform_find_next_actionstring_file_on_page(ui->bulk_last_page, &ui->bulk_last_element, &ui->bulk_last_event, &file_handle);

    if (!was_last_one) {
      grid_platform_delete_actionstring_file(&file_handle);
    } else {
      ui->bulk_last_page++;
    }

    grid_ui_bulk_semaphore_release(ui);
    grid_ui_busy_semaphore_release(ui);
    return;
  }

  // STEP 2: Delete config file
  if (ui->bulk_last_page == ui->page_count) {

    union grid_ui_file_handle file_handle = {0};

    if (grid_platform_find_file(GRID_UI_CONFIG_PATH, &file_handle) == 0) {
      grid_platform_delete_file(&file_handle);
    }

    ui->bulk_last_page++;

    grid_ui_bulk_semaphore_release(ui);
    grid_ui_busy_semaphore_release(ui);
    return;
  }

  // STEP 3: erase the flash pages
  if (0 == grid_platform_erase_nvm_next()) {

    // There is still stuff to be erased
    grid_ui_bulk_semaphore_release(ui);
    grid_ui_busy_semaphore_release(ui);
    return;
  }

  // Set ready before callback so callback can start new nvm operation
  ui->bulk_status = GRID_UI_BULK_READY;

  grid_ui_bulk_semaphore_release(ui);
  grid_ui_busy_semaphore_release(ui);
  grid_platform_printf("NVM: Erase done\r\n");

  // Call success callback
  if (ui->bulk_success_callback != NULL) {

    ui->bulk_success_callback(ui->bulk_lastheader_id);
    ui->bulk_success_callback = NULL;
  }

  return;
}

void grid_port_process_ui_local_UNSAFE(struct grid_ui_model* ui) {

  grid_ui_busy_semaphore_lock(ui);

  // Prepare packet header LOCAL
  struct grid_msg_packet message_local;
  grid_msg_packet_init(&grid_msg_state, &message_local, GRID_PARAMETER_DEFAULT_POSITION, GRID_PARAMETER_DEFAULT_POSITION);
  char payload_local[GRID_PARAMETER_PACKET_maxlength] = {0};
  uint32_t offset_local = 0;

  // Prepare packet header GLOBAL
  struct grid_msg_packet message_global;
  grid_msg_packet_init(&grid_msg_state, &message_global, GRID_PARAMETER_DEFAULT_POSITION, GRID_PARAMETER_DEFAULT_POSITION);
  char payload_global[GRID_PARAMETER_PACKET_maxlength] = {0};
  uint32_t offset_global = 0;

  for (uint8_t j = 0; j < ui->element_list_length; j++) {

    // Handle the system element first, then all the UI elements in ascending order
    uint8_t element_index = (j == 0 ? ui->element_list_length - 1 : j - 1);

    struct grid_ui_element* ele = &ui->element_list[element_index];

    for (uint8_t k = 0; k < ele->event_list_length; k++) {

      struct grid_ui_event* eve = &ele->event_list[k];

      if (offset_local > GRID_PARAMETER_PACKET_marign || offset_global > GRID_PARAMETER_PACKET_marign) {
        continue;
      }

      if (!grid_ui_event_istriggered_local(eve)) {
        continue;
      }

      offset_local += grid_ui_event_render_action(eve, &payload_local[offset_local]);

      grid_ui_event_state_set(eve, GRID_EVE_STATE_INIT);

      // Automatically report elementname after config
      if (ele->type != GRID_PARAMETER_ELEMENT_SYSTEM && ele->type != GRID_PARAMETER_ELEMENT_LCD) {

        char command[26] = {0};

        sprintf(command, "gens(%d,ele[%d]:gen())", ele->index, ele->index);

        grid_lua_clear_stdo(&grid_lua_state);
        grid_lua_dostring(&grid_lua_state, command);
        strcat(payload_global, grid_lua_get_output_string(&grid_lua_state));
        grid_lua_clear_stdo(&grid_lua_state);
      }
    }
  }

  if (strlen(payload_global) > 0) {

    grid_msg_packet_body_append_text(&message_global, payload_global);
    grid_msg_packet_close(&grid_msg_state, &message_global);
    grid_transport_send_msg_packet_to_all(&grid_transport_state, &message_global);
  }

  grid_msg_packet_body_append_text(&message_local, payload_local);
  grid_msg_packet_close(&grid_msg_state, &message_local);

  uint32_t message_length = grid_msg_packet_get_length(&message_local);

  struct grid_port* port = grid_transport_get_port(&grid_transport_state, 4, GRID_PORT_UI, 0);

  struct grid_swsr_t* tx = grid_port_get_tx(port);

  if (grid_swsr_writable(tx, message_length)) {
    grid_msg_packet_to_swsr(&message_local, tx);
  }

  grid_ui_busy_semaphore_release(ui);
}

void grid_port_process_ui_UNSAFE(struct grid_ui_model* ui) {

  grid_ui_busy_semaphore_lock(ui);

  struct grid_msg_packet message;
  grid_msg_packet_init(&grid_msg_state, &message, GRID_PARAMETER_GLOBAL_POSITION, GRID_PARAMETER_GLOBAL_POSITION);

  for (uint8_t j = 0; j < ui->element_list_length; j++) {

    // Handle system element first then all the ui elements in ascending order
    uint8_t element_index = (j == 0 ? ui->element_list_length - 1 : j - 1);

    struct grid_ui_element* ele = &ui->element_list[element_index];

    for (uint8_t k = 0; k < ele->event_list_length; k++) {

      struct grid_ui_event* eve = &ele->event_list[k];

      if (grid_msg_packet_get_length(&message) > GRID_PARAMETER_PACKET_marign) {
        continue;
      }

      if (!grid_ui_event_istriggered(eve)) {
        continue;
      }

      uint32_t offset = grid_msg_packet_body_get_length(&message);

      message.body_length += grid_ui_event_render_event(eve, &message.body[offset]);

      offset = grid_msg_packet_body_get_length(&message);

      message.body_length += grid_ui_event_render_action(eve, &message.body[offset]);

      grid_ui_event_state_set(eve, GRID_EVE_STATE_INIT);
    }
  }

  grid_msg_packet_close(&grid_msg_state, &message);
  grid_transport_send_msg_packet_to_all(&grid_transport_state, &message);

  // LEDREPORT
  if (grid_protocol_led_change_report_length(&grid_led_state) && grid_sys_get_editor_connected_state(&grid_sys_state)) {

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
    grid_transport_send_msg_packet_to_all(&grid_transport_state, &response);
  }

  grid_ui_busy_semaphore_release(ui);
}
