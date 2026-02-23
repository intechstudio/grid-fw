#include "grid_ui.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "grid_config.h"
#include "grid_platform.h"
#include "grid_protocol.h"
#include "grid_transport.h"
#include "grid_usb.h"

extern void grid_platform_printf(char const* fmt, ...);

extern const struct luaL_Reg* grid_lua_api_generic_lib_reference;

void grid_ui_semaphore_init(struct grid_ui_semaphore* semaphore, void* handle, void (*lock_fn)(void*), void (*release_fn)(void*), bool (*try_fn)(void*)) {

  semaphore->handle = handle;
  semaphore->lock_fn = lock_fn;
  semaphore->release_fn = release_fn;
  semaphore->try_fn = try_fn;
}

void grid_ui_semaphore_lock(struct grid_ui_semaphore* semaphore) {

  if (!semaphore->handle || !semaphore->lock_fn) {
    return;
  }

  semaphore->lock_fn(semaphore->handle);
}

void grid_ui_semaphore_release(struct grid_ui_semaphore* semaphore) {

  if (!semaphore->handle || !semaphore->release_fn) {
    return;
  }

  semaphore->release_fn(semaphore->handle);
}

bool grid_ui_semaphore_try(struct grid_ui_semaphore* semaphore) {

  if (!semaphore->handle || !semaphore->try_fn) {
    return true;
  }

  return semaphore->try_fn(semaphore->handle);
}

uint8_t GRID_ELE_EVE_TO_VALUE_IDX[GRID_PARAMETER_ELEMENT_COUNT][GRID_PARAMETER_EVENT_COUNT] = {0};

void grid_ele_eve_to_value_idx_init(uint8_t map[GRID_PARAMETER_ELEMENT_COUNT][GRID_PARAMETER_EVENT_COUNT]) {

  // Initialize all indices to 255/-1, signifying an invalid value by default
  memset(map, 255, GRID_PARAMETER_ELEMENT_COUNT * GRID_PARAMETER_EVENT_COUNT);

  // Initialize valid indices
  map[GRID_PARAMETER_ELEMENT_POTMETER][GRID_PARAMETER_EVENT_POTMETER] = GRID_LUA_FNC_P_POTMETER_VALUE_index;
  map[GRID_PARAMETER_ELEMENT_BUTTON][GRID_PARAMETER_EVENT_BUTTON] = GRID_LUA_FNC_B_BUTTON_VALUE_index;
  map[GRID_PARAMETER_ELEMENT_ENCODER][GRID_PARAMETER_EVENT_BUTTON] = GRID_LUA_FNC_E_BUTTON_VALUE_index;
  map[GRID_PARAMETER_ELEMENT_ENCODER][GRID_PARAMETER_EVENT_ENCODER] = GRID_LUA_FNC_E_ENCODER_VALUE_index;
  map[GRID_PARAMETER_ELEMENT_ENDLESS][GRID_PARAMETER_EVENT_BUTTON] = GRID_LUA_FNC_EP_BUTTON_VALUE_index;
  map[GRID_PARAMETER_ELEMENT_ENDLESS][GRID_PARAMETER_EVENT_ENDLESS] = GRID_LUA_FNC_EP_ENDLESS_VALUE_index;
}

struct grid_ui_model grid_ui_state = {0};

void grid_ui_model_init(struct grid_ui_model* ui, uint8_t element_list_length) {

  ui->lua_ui_init_callback = NULL;

  ui->page_activepage = 0;
  ui->page_count = 4;

  ui->page_change_enabled = 1;

  ui->element_list_length = element_list_length;

  ui->element_list = grid_platform_allocate_volatile(element_list_length * sizeof(struct grid_ui_element));

  ui->page_negotiated = 0;

  ui->mapmode_state = 0;

  ui->bulk_success_callback = NULL;
  ui->bulk_last_page = -1;

  assert(grid_swsr_malloc(&ui->bulk, 1 * sizeof(fn_prthread_bulk_t)) == 0);
  ui->bulk_curr = NULL;
  ui->bulk_active = false;

  grid_ele_eve_to_value_idx_init(GRID_ELE_EVE_TO_VALUE_IDX);
}

struct grid_ui_element* grid_ui_model_get_elements(struct grid_ui_model* ui) {

  assert(ui->element_list);

  return ui->element_list;
}

void grid_ui_bulk_semaphore_lock(struct grid_ui_model* ui) { grid_ui_semaphore_lock(&ui->bulk_semaphore); }

void grid_ui_bulk_semaphore_release(struct grid_ui_model* ui) { grid_ui_semaphore_release(&ui->bulk_semaphore); }

bool grid_ui_bulk_semaphore_try(struct grid_ui_model* ui) { return grid_ui_semaphore_try(&ui->bulk_semaphore); }

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
  ele->name[0] = '\0';

  ele->template_parameter_list_length = 0;
  ele->template_parameter_list = NULL;

  ele->template_buffer_list_head = NULL;
  ele->template_parameter_element_position_index_1 = 0;
  ele->template_parameter_element_position_index_2 = 0;
  for (int i = 0; i < 2; ++i) {
    ele->template_parameter_index_value[i] = 0;
    ele->template_parameter_index_min[i] = 0;
    ele->template_parameter_index_max[i] = 0;
  }

  ele->timer_event_helper = 0;
  ele->timer_source_is_midi = 0;

  ele->primary_state = NULL;

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

  grid_lua_semaphore_lock(&grid_lua_state);
  grid_lua_gc_full_unsafe(&grid_lua_state);
  grid_lua_semaphore_release(&grid_lua_state);

  eve->cfg_changed_flag = 1;
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
    // add 1 to footer to compensate for an added space
    sprintf(&result[strlen(result) - (footer_len + 1)], " ?>");
    uint8_t offset = header_len - 6;
    memcpy(&result[offset], "<?lua ", 6);

    strcpy(targetstring, &result[offset]);

  } else {

    grid_ui_event_generate_actionstring(eve, targetstring);
    grid_platform_printf("ERROR: invalid debug.getinfo\r\n");
  }
}

int grid_ui_event_recall_configuration(struct grid_ui_model* ui, uint8_t page, uint8_t element, uint8_t event_type, char* targetstring) {

  if (grid_ui_bulk_in_progress(ui)) {
    grid_platform_printf("grid_ui_event_recall_configuration: nvm busy\n");
    return 1;
  }

  if (ui->page_activepage != page) {
    grid_platform_printf("grid_ui_event_recall_configuration: inactive page\n");
    return 1;
  }

  struct grid_ui_element* ele = grid_ui_element_find(ui, element);

  if (!ele) {
    grid_platform_printf("grid_ui_event_recall_configuration: invalid element\n");
    return 1;
  }

  struct grid_ui_event* eve = grid_ui_event_find(ele, event_type);

  if (!eve) {
    grid_platform_printf("grid_ui_event_recall_configuration: invalid event\n");
    strcpy(targetstring, "<?lua --[[@cb]] --[[event deprecated]] ?>");
    return 0;
  }

  if (eve->cfg_default_flag) {

    grid_ui_event_generate_actionstring(eve, targetstring);

  } else {

    grid_ui_event_get_actionstring(eve, targetstring);
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

void grid_ui_event_render_event(struct grid_ui_event* eve, struct grid_msg* msg) {

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

  if (grid_msg_add_frame(msg, GRID_CLASS_EVENT_frame) <= 0) {
    return;
  }

  grid_msg_set_parameter(msg, INSTR, GRID_INSTR_EXECUTE_code);
  grid_msg_set_parameter(msg, CLASS_EVENT_PAGENUMBER, page);
  grid_msg_set_parameter(msg, CLASS_EVENT_ELEMENTNUMBER, element);
  grid_msg_set_parameter(msg, CLASS_EVENT_EVENTTYPE, event);
  grid_msg_set_parameter(msg, CLASS_EVENT_EVENTPARAM1, param1);
  grid_msg_set_parameter(msg, CLASS_EVENT_EVENTPARAM2, param2);
}

void grid_ui_event_render_action(struct grid_ui_event* eve, struct grid_msg* msg) {

  if (!grid_lua_do_event(&grid_lua_state, eve->parent->index, eve->function_name)) {

    grid_lua_broadcast_stde(&grid_lua_state);
  }

  char* stdo = grid_lua_get_output_string(&grid_lua_state);
  int ret = grid_msg_nprintf(msg, "%s", stdo);
  grid_lua_clear_stdo(&grid_lua_state);

  grid_lua_clear_stde(&grid_lua_state);

  // Call the event clear callback
  if (eve->parent->event_clear_cb) {

    eve->parent->event_clear_cb(eve);
  }
}

void grid_ui_event_render_event_view(struct grid_ui_event* eve, struct grid_msg* msg) {

  struct grid_ui_element* ele = eve->parent;

  if (!ele->template_parameter_list) {
    return;
  }

  uint8_t page = ele->parent->page_activepage;
  uint8_t element = ele->index;
  uint8_t event = eve->type;
  const char* name = ele->name;
  uint32_t name_len = strlen(name);

  // assign mapmode/system to element 255
  if (event == GRID_PARAMETER_ELEMENT_SYSTEM) {
    element = 255;
  }

  grid_msg_add_frame(msg, GRID_CLASS_EVENTVIEW_frame_start);
  grid_msg_set_parameter(msg, INSTR, GRID_INSTR_EXECUTE_code);
  grid_msg_set_parameter(msg, CLASS_EVENTVIEW_PAGE, page);
  grid_msg_set_parameter(msg, CLASS_EVENTVIEW_ELEMENT, element);
  grid_msg_set_parameter(msg, CLASS_EVENTVIEW_EVENT, event);

  uint8_t value_index_valid = GRID_ELE_EVE_TO_VALUE_IDX[ele->type][event] != 255;

  // If the value index is valid, assume that min and max are the next two,
  // for the sake of minimizing the memory footprint of the mapping
  uint8_t index_value = value_index_valid ? GRID_ELE_EVE_TO_VALUE_IDX[ele->type][event] : 0;
  uint8_t index_min = value_index_valid ? index_value + 1 : 0;
  uint8_t index_max = value_index_valid ? index_value + 2 : 0;

  grid_msg_set_parameter(msg, CLASS_EVENTVIEW_VALUE1, ele->template_parameter_list[index_value]);
  grid_msg_set_parameter(msg, CLASS_EVENTVIEW_MIN1, ele->template_parameter_list[index_min]);
  grid_msg_set_parameter(msg, CLASS_EVENTVIEW_MAX1, ele->template_parameter_list[index_max]);

  grid_msg_add_segment_char(msg, GRID_CLASS_EVENTVIEW_LENGTH_length, name_len, name);

  grid_msg_add_frame(msg, GRID_CLASS_EVENTVIEW_frame_end);
}

void grid_port_process_ui_local_UNSAFE(struct grid_ui_model* ui) {

  if (!grid_ui_bulk_semaphore_try(ui)) {
    return;
  }

  uint8_t xy = GRID_PARAMETER_GLOBAL_POSITION;

  struct grid_msg msg_local;
  grid_msg_init_brc(&grid_msg_state, &msg_local, xy, xy);
  uint16_t local_start_length = msg_local.length;

  struct grid_msg msg_global;
  grid_msg_init_brc(&grid_msg_state, &msg_global, xy, xy);
  uint16_t global_start_length = msg_global.length;

  for (uint8_t j = 0; j < ui->element_list_length; j++) {

    // Handle the system element first, then all the UI elements in ascending order
    uint8_t element_index = (j == 0 ? ui->element_list_length - 1 : j - 1);

    struct grid_ui_element* ele = &ui->element_list[element_index];

    for (uint8_t k = 0; k < ele->event_list_length; k++) {

      if (msg_local.length >= GRID_PARAMETER_PACKET_margin) {
        break;
      }

      if (msg_global.length >= GRID_PARAMETER_PACKET_margin) {
        break;
      }

      struct grid_ui_event* eve = &ele->event_list[k];

      if (!grid_ui_event_istriggered_local(eve)) {
        continue;
      }

      grid_ui_event_render_action(eve, &msg_local);

      grid_ui_event_state_set(eve, GRID_EVE_STATE_INIT);

      // Automatically report elementname after config
      if (ele->type != GRID_PARAMETER_ELEMENT_SYSTEM && ele->type != GRID_PARAMETER_ELEMENT_LCD) {

        char command[26] = {0};

        sprintf(command, "gens(%d,ele[%d]:gen())", ele->index, ele->index);

        grid_lua_clear_stdo(&grid_lua_state);
        grid_lua_dostring(&grid_lua_state, command);
        grid_msg_nprintf(&msg_global, "%s", grid_lua_get_output_string(&grid_lua_state));
        grid_lua_clear_stdo(&grid_lua_state);
      }
    }
  }

  if (msg_global.length > global_start_length) {

    if (grid_msg_close_brc(&grid_msg_state, &msg_global) >= 0) {

      grid_transport_send_msg_to_all(&grid_transport_state, &msg_global);
    }
  }

  if (msg_local.length > local_start_length) {

    if (grid_msg_close_brc(&grid_msg_state, &msg_local) >= 0) {

      struct grid_port* port = grid_transport_get_port(&grid_transport_state, 4, GRID_PORT_UI, 0);

      struct grid_swsr_t* tx = grid_port_get_tx(port);

      if (grid_swsr_writable(tx, msg_local.length)) {
        grid_msg_to_swsr(&msg_local, tx);
      }
    }
  }

  grid_ui_bulk_semaphore_release(ui);
}

void grid_port_process_ui_UNSAFE(struct grid_ui_model* ui) {

  if (!grid_ui_bulk_semaphore_try(ui)) {
    return;
  }

  struct grid_msg msg;
  uint8_t xy = GRID_PARAMETER_GLOBAL_POSITION;
  grid_msg_init_brc(&grid_msg_state, &msg, xy, xy);
  uint32_t msg_start_length = msg.length;

  for (uint8_t j = 0; j < ui->element_list_length; j++) {

    // Handle system element first then all the ui elements in ascending order
    uint8_t element_index = (j == 0 ? ui->element_list_length - 1 : j - 1);

    struct grid_ui_element* ele = &ui->element_list[element_index];

    for (uint8_t k = 0; k < ele->event_list_length; k++) {

      if (msg.length >= GRID_PARAMETER_PACKET_margin) {
        break;
      }

      struct grid_ui_event* eve = &ele->event_list[k];

      if (!grid_ui_event_istriggered(eve)) {
        continue;
      }

      grid_ui_event_render_event(eve, &msg);
      grid_ui_event_render_event_view(eve, &msg);
      grid_ui_event_render_action(eve, &msg);

      grid_ui_event_state_set(eve, GRID_EVE_STATE_INIT);
    }
  }

  if (msg_start_length != msg.length) {

    grid_led_render_framebuffer(&grid_led_state);

    uint16_t change_length = grid_protocol_led_change_report_length(&grid_led_state);
    uint8_t editor_connected = grid_sys_get_editor_connected_state(&grid_sys_state);

    if (change_length && editor_connected) {

      char report[300] = {0};
      uint16_t report_len = grid_protocol_led_change_report_generate(&grid_led_state, -1, report);

      grid_msg_add_frame(&msg, GRID_CLASS_LEDPREVIEW_frame_start);
      grid_msg_set_parameter(&msg, INSTR, GRID_INSTR_EXECUTE_code);
      grid_msg_set_parameter(&msg, CLASS_LEDPREVIEW_LENGTH, report_len);
      grid_msg_nprintf(&msg, "%.*s", report_len, report);
      grid_msg_add_frame(&msg, GRID_CLASS_LEDPREVIEW_frame_end);
    }

    if (grid_msg_close_brc(&grid_msg_state, &msg) >= 0) {
      grid_transport_send_msg_to_all(&grid_transport_state, &msg);
    }
  }

  grid_ui_bulk_semaphore_release(ui);
}

uint8_t grid_ui_bulk_get_lastheader(struct grid_ui_model* ui) { return ui->bulk_lastheader_id; }

uint8_t grid_ui_bulk_get_response_code(struct grid_ui_model* ui, uint8_t id) {

  bool lastheadermatch = grid_ui_bulk_get_lastheader(&grid_ui_state) == id;
  bool nothinginprogress = !grid_ui_bulk_in_progress(&grid_ui_state);

  bool ack = lastheadermatch && nothinginprogress;

  return ack ? GRID_INSTR_ACKNOWLEDGE_code : GRID_INSTR_NACKNOWLEDGE_code;
}

bool grid_ui_bulk_operation_known(fn_prthread_bulk_t fn) {

  if (fn == grid_ui_bulk_page_load) {
  } else if (fn == grid_ui_bulk_page_read) {
  } else if (fn == grid_ui_bulk_page_store) {
  } else if (fn == grid_ui_bulk_page_clear) {
  } else if (fn == grid_ui_bulk_conf_read) {
  } else if (fn == grid_ui_bulk_conf_store) {
  } else if (fn == grid_ui_bulk_conf_erase) {
  } else if (fn == grid_ui_bulk_nvm_erase) {
  } else {
    return false;
  }

  return true;
}

static bool grid_ui_bulk_queue_with_state(struct grid_ui_model* ui, fn_prthread_bulk_t next, uint8_t page, uint8_t lastheader_id, void (*success_cb)(uint8_t)) {

  if (!grid_swsr_writable(&ui->bulk, sizeof(fn_prthread_bulk_t))) {
    return false;
  }

  assert(grid_ui_bulk_operation_known(next));

  ui->bulk_last_page = page;
  ui->bulk_lastheader_id = lastheader_id;
  ui->bulk_success_callback = success_cb;

  grid_swsr_write(&ui->bulk, &next, sizeof(fn_prthread_bulk_t));

  return true;
}

bool grid_ui_bulk_start_with_state(struct grid_ui_model* ui, fn_prthread_bulk_t next, uint8_t page, uint8_t lastheader_id, void (*success_cb)(uint8_t)) {

  if (grid_ui_bulk_in_progress(ui)) {
    return false;
  }

  assert(grid_ui_bulk_queue_with_state(ui, next, page, lastheader_id, success_cb));

  return true;
}

bool grid_ui_bulk_in_waiting(struct grid_ui_model* ui) { return grid_swsr_readable(&ui->bulk, sizeof(fn_prthread_bulk_t)); }

bool grid_ui_bulk_in_progress(struct grid_ui_model* ui) { return ui->bulk_curr || grid_ui_bulk_in_waiting(ui); }

void grid_ui_bulk_handle_success_cb(struct grid_ui_model* ui) {

  if (!ui->bulk_success_callback) {
    return;
  }

  ui->bulk_success_callback(ui->bulk_lastheader_id);

  ui->bulk_success_callback = NULL;
}

void grid_ui_bulk_process(struct grid_ui_model* ui) {

  // No currently executing protothread
  if (!ui->bulk_curr) {

    // There are protothreads waiting to be executed
    if (grid_ui_bulk_in_waiting(ui)) {

      // Make the next waiting protothread the next one to be executed
      assert(grid_swsr_readable(&ui->bulk, sizeof(fn_prthread_bulk_t)));
      grid_swsr_read(&ui->bulk, &ui->bulk_curr, sizeof(fn_prthread_bulk_t));

      // Bulk processing is about to begin after a period of inactivity
      if (!ui->bulk_active) {

        grid_ui_bulk_semaphore_lock(ui);

        assert(!ui->bulk_active);
        ui->bulk_active = true;
      }

    } else {

      assert(!ui->bulk_active);
      return;
    }
  }

  // Call protothread
  assert(grid_ui_bulk_operation_known(ui->bulk_curr));
  char status = ui->bulk_curr(&ui->bulk_pt, ui);

  // Protothread requires no further execution
  if (status >= PT_EXITED) {

    // Protothread reached its end (instead of exiting early)
    if (status == PT_ENDED) {
      grid_ui_bulk_handle_success_cb(ui);
    }

    // Unset currently executing protothread
    ui->bulk_curr = NULL;

    // There are no more protohreads waiting to be executed
    if (!grid_ui_bulk_in_waiting(ui)) {

      assert(ui->bulk_active);
      ui->bulk_active = false;

      // Bulk processing is about to end after a period of activity
      grid_ui_bulk_semaphore_release(ui);
    }
  }
}

void grid_ui_bulk_flush(struct grid_ui_model* ui) {

  while (grid_ui_bulk_in_progress(ui)) {
    grid_ui_bulk_process(ui);
  }
}

PT_THREAD(grid_ui_bulk_page_load(proto_pt_t* pt, struct grid_ui_model* ui)) {

  // Reset all state parameters of all leds
  grid_led_reset(&grid_led_state);

  // Store old page index for the page change callback
  uint8_t oldpage = ui->page_activepage;

  // Set active page
  ui->page_activepage = ui->bulk_last_page;

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
    while (template_buffer_length < ui->page_activepage + 1) {

      if (!grid_ui_template_buffer_create(ele)) {
        grid_platform_printf("grid_ui_page_load failed to create template buffer\r\n");
      }

      template_buffer_length = grid_ui_template_buffer_list_length(ele);
    }

    struct grid_ui_template_buffer* buf = grid_ui_template_buffer_find(ele, ui->page_activepage);

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
      ele->page_change_cb(ele, oldpage, ui->page_activepage);
    }
  }

  // Restart VM, register functions
  grid_lua_stop_vm(&grid_lua_state);
  grid_lua_start_vm(&grid_lua_state, grid_lua_api_generic_lib_reference, grid_ui_state.lua_ui_init_callback);

  grid_lua_pre_init(&grid_lua_state);

  uint8_t read_page = grid_ui_page_get_activepage(&grid_ui_state);
  grid_ui_bulk_queue_with_state(ui, grid_ui_bulk_page_read, read_page, 0, NULL);

  return PT_ENDED;
}

PT_THREAD(grid_ui_bulk_page_read(proto_pt_t* pt, struct grid_ui_model* ui)) {

  static int page;
  static int element;
  static int event;

  PT_BEGIN(pt);

  if (!grid_platform_get_nvm_state()) {
    PT_EXIT(pt);
  }

  // Mark all actionstrings as default
  for (uint8_t i = 0; i < ui->element_list_length; ++i) {

    struct grid_ui_element* ele = &ui->element_list[i];

    for (uint8_t j = 0; j < ele->event_list_length; ++j) {

      ele->event_list[j].cfg_default_flag = 1;
    }
  }

  page = ui->bulk_last_page;
  element = -1;
  event = -1;

  // Register all custom actionstring files
  struct grid_file_t handle;
  while (!grid_platform_find_next_actionstring_file_on_page(page, &element, &event, &handle)) {

    uint16_t size = grid_platform_get_file_size(&handle);
    if (size > 0) {

      char temp[GRID_PARAMETER_ACTIONSTRING_maxlength + 100] = {0};
      grid_platform_read_file(&handle, (uint8_t*)temp, size);

      struct grid_ui_event* eve = grid_ui_event_find(&ui->element_list[element], event);
      grid_ui_event_register_actionstring(eve, temp);
      eve->cfg_changed_flag = 0; // clear changed flag

      grid_platform_printf("grid_ui_bulk_page_read, element: %d, event: %d\n", element, event);
    }

    PT_YIELD(pt);
  }

  // Fill all of the remaining default events with default actionstrings
  for (uint8_t i = 0; i < ui->element_list_length; ++i) {

    struct grid_ui_element* ele = &ui->element_list[i];

    for (uint8_t j = 0; j < ele->event_list_length; ++j) {

      struct grid_ui_event* eve = &ele->event_list[j];

      if (eve->cfg_default_flag == 1) {

        char temp[GRID_PARAMETER_ACTIONSTRING_maxlength + 100] = {0};

        grid_ui_event_generate_actionstring(eve, temp);
        grid_ui_event_register_actionstring(eve, temp);
      }
    }
  }

  grid_lua_post_init(&grid_lua_state);

  grid_usb_keyboard_enable(&grid_usb_keyboard_state);

  PT_END(pt);
}

PT_THREAD(grid_ui_bulk_page_store(proto_pt_t* pt, struct grid_ui_model* ui)) {

  static int page;
  static uint8_t i;
  static uint8_t j;
  static struct grid_ui_element* ele;
  static struct grid_ui_event* eve;

  PT_BEGIN(pt);

  if (!grid_platform_get_nvm_state()) {
    PT_EXIT(pt);
  }

  page = ui->bulk_last_page;

  for (i = 0; i < ui->element_list_length; ++i) {

    ele = &ui->element_list[i];

    for (j = 0; j < ele->event_list_length; ++j) {

      eve = &ele->event_list[j];

      if (!eve->cfg_changed_flag) {
        continue;
      }

      if (eve->cfg_default_flag) {

        struct grid_file_t handle;
        if (grid_platform_find_actionstring_file(page, ele->index, eve->type, &handle) == 0) {
          grid_platform_delete_file(&handle);
          grid_platform_printf("grid_ui_bulk_page_store, delete: %s\n", handle.path);
        }

      } else {

        char buffer[GRID_PARAMETER_ACTIONSTRING_maxlength + 100] = {0};
        grid_ui_event_get_actionstring(eve, buffer);
        grid_platform_write_actionstring_file(page, ele->index, eve->type, buffer, strlen(buffer));
        grid_platform_printf("grid_ui_bulk_page_store, element: %d, event: %d\n", i, j);
      }

      // Clear changed flag
      eve->cfg_changed_flag = 0;

      PT_YIELD(pt);
    }
  }

  PT_END(pt);
}

PT_THREAD(grid_ui_bulk_page_clear(proto_pt_t* pt, struct grid_ui_model* ui)) {

  static int page;
  static int element;
  static int event;

  PT_BEGIN(pt);

  if (!grid_platform_get_nvm_state()) {
    PT_EXIT(pt);
  }

  page = ui->bulk_last_page;
  element = -1;
  event = -1;

  struct grid_file_t handle;
  while (!grid_platform_find_next_actionstring_file_on_page(page, &element, &event, &handle)) {

    grid_platform_delete_file(&handle);

    PT_YIELD(pt);
  }

  char path[50] = {0};
  sprintf(path, "%02x", ui->bulk_last_page);
  grid_platform_remove_dir(path);

  PT_END(pt);
}

int confread_parse_from_file(struct grid_ui_model* ui) {

  int status;

  struct grid_file_t handle = {0};

  status = grid_platform_find_file(GRID_UI_CONFIG_PATH, &handle);
  if (status) {
    grid_platform_printf("grid_platform_find_file returned %d\n", status);
    return 1;
  }

  uint16_t file_size = grid_platform_get_file_size(&handle);
  if (file_size == 0) {
    grid_platform_printf("grid_platform_get_file_size returned %d\n", status);
    return 1;
  }

  char* buffer = (char*)malloc(file_size);
  if (buffer == NULL) {
    grid_platform_printf("confread_parse_from_file malloc\n");
    return 1;
  }

  status = grid_platform_read_file(&handle, (uint8_t*)buffer, file_size);
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

PT_THREAD(grid_ui_bulk_conf_read(proto_pt_t* pt, struct grid_ui_model* ui)) {

  PT_BEGIN(pt);

  if (!grid_platform_get_nvm_state()) {
    PT_EXIT(pt);
  }

  int status = confread_parse_from_file(ui);

  grid_platform_printf("grid_ui_bulk_conf_read, status: %d\n", status);

  PT_END(pt);
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

PT_THREAD(grid_ui_bulk_conf_store(proto_pt_t* pt, struct grid_ui_model* ui)) {

  PT_BEGIN(pt);

  if (!grid_platform_get_nvm_state()) {
    PT_EXIT(pt);
  }

  int status = confstore_generate_to_file(ui);

  grid_platform_printf("grid_ui_bulk_conf_store, status: %d\n", status);

  PT_END(pt);
}

PT_THREAD(grid_ui_bulk_conf_erase(proto_pt_t* pt, struct grid_ui_model* ui)) {

  PT_BEGIN(pt);

  if (!grid_platform_get_nvm_state()) {
    PT_EXIT(pt);
  }

  struct grid_file_t handle = {0};

  if (grid_platform_find_file(GRID_UI_CONFIG_PATH, &handle) == 0) {
    grid_platform_printf("grid_ui_bulk_conf_erase\n");
    grid_platform_delete_file(&handle);
  }

  PT_END(pt);
}

PT_THREAD(grid_ui_bulk_nvm_erase(proto_pt_t* pt, struct grid_ui_model* ui)) {

  PT_BEGIN(pt);

  if (!grid_platform_get_nvm_state()) {
    PT_EXIT(pt);
  }

  if (ui->bulk_last_page < 0) {
    ui->bulk_last_page = 0;
  }

  grid_platform_nvm_format_and_mount();

  PT_END(pt);
}
