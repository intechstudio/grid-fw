#ifndef GRID_UI_H
#define GRID_UI_H

#include <stdint.h>

#include "grid_lua.h"

struct grid_ui_semaphore {
  void* handle;
  void (*lock_fn)(void*);
  void (*release_fn)(void*);
};

void grid_ui_semaphore_init(struct grid_ui_semaphore* semaphore, void* handle, void (*lock_fn)(void*), void (*release_fn)(void*));
void grid_ui_semaphore_lock(struct grid_ui_semaphore* semaphore);
void grid_ui_semaphore_release(struct grid_ui_semaphore* semaphore);

enum grid_eve_state_t {
  GRID_EVE_STATE_INIT = 0,
  GRID_EVE_STATE_TRIG,
  GRID_EVE_STATE_TRIG_LOCAL,
};

struct grid_ui_event {

  struct grid_ui_element* parent;

  enum grid_eve_state_t state;

  const char* default_actionstring;

  uint8_t cfg_changed_flag;
  uint8_t cfg_default_flag;

  uint8_t type;

  char function_name[10];
};

void grid_ui_event_state_set(struct grid_ui_event* eve, enum grid_eve_state_t state);

struct grid_ui_template_buffer {

  struct grid_ui_element* parent;

  struct grid_ui_template_buffer* next;

  int32_t* template_parameter_list;
};

typedef void (*template_init_t)(struct grid_ui_template_buffer*);

enum { GRID_ELEMENT_NAME_MAX = 19 };
enum { GRID_ELEMENT_NAME_SIZE = GRID_ELEMENT_NAME_MAX + 1 };

struct grid_ui_element {

  uint8_t timer_source_is_midi;
  uint32_t timer_event_helper;

  struct grid_ui_model* parent;
  uint8_t index;
  uint8_t type;
  char name[GRID_ELEMENT_NAME_SIZE];

  template_init_t template_initializer;

  struct grid_ui_template_buffer* template_buffer_list_head;
  uint8_t template_parameter_list_length;
  int32_t* template_parameter_list;

  uint8_t template_parameter_element_position_index_1;
  uint8_t template_parameter_element_position_index_2;
  uint8_t template_parameter_index_value[2];
  uint8_t template_parameter_index_min[2];
  uint8_t template_parameter_index_max[2];

  void (*page_change_cb)(struct grid_ui_element*, uint8_t, uint8_t);
  void (*event_clear_cb)(struct grid_ui_event*);

  uint8_t event_list_length;
  struct grid_ui_event* event_list;
};

enum grid_ui_bulk_status_t {
  GRID_UI_BULK_READY = 0,
  GRID_UI_BULK_READ_PROGRESS,
  GRID_UI_BULK_STORE_PROGRESS,
  GRID_UI_BULK_CLEAR_PROGRESS,
  GRID_UI_BULK_CONFREAD_PROGRESS,
  GRID_UI_BULK_CONFSTORE_PROGRESS,
  GRID_UI_BULK_ERASE_PROGRESS
};

// struct grid_lua_model;

struct grid_ui_model {

  struct grid_ui_semaphore busy_semaphore;
  struct grid_ui_semaphore bulk_semaphore;

  uint8_t page_activepage;
  uint8_t page_count;

  uint8_t page_change_enabled;
  uint8_t page_negotiated;

  uint8_t mapmode_state;

  uint8_t element_list_length;
  struct grid_ui_element* element_list;

  enum grid_ui_bulk_status_t bulk_status;
  void (*bulk_success_callback)(uint8_t);
  uint8_t bulk_lastheader_id;
  int bulk_last_page;
  int bulk_last_element;
  int bulk_last_event;

  lua_ui_init_callback_t lua_ui_init_callback;
};

extern struct grid_ui_model grid_ui_state;

void grid_ui_model_init(struct grid_ui_model* ui, uint8_t element_list_length);
struct grid_ui_element* grid_ui_model_get_elements(struct grid_ui_model* ui);

void grid_ui_busy_semaphore_lock(struct grid_ui_model* ui);
void grid_ui_busy_semaphore_release(struct grid_ui_model* ui);
void grid_ui_bulk_semaphore_lock(struct grid_ui_model* ui);
void grid_ui_bulk_semaphore_release(struct grid_ui_model* ui);

struct grid_ui_element* grid_ui_element_model_init(struct grid_ui_model* parent, uint8_t index);
void grid_ui_event_init(struct grid_ui_element* ele, uint8_t index, uint8_t event_type, char* function_name, const char* default_actionstring);

void grid_ui_rtc_ms_tick_time(struct grid_ui_model* ui);
void grid_ui_midi_sync_tick_time(struct grid_ui_model* ui);

void grid_ui_rtc_ms_mapmode_handler(struct grid_ui_model* ui, uint8_t new_mapmode_value);

struct grid_ui_template_buffer* grid_ui_template_buffer_create(struct grid_ui_element* ele);
uint8_t grid_ui_template_buffer_list_length(struct grid_ui_element* ele);
struct grid_ui_template_buffer* grid_ui_template_buffer_find(struct grid_ui_element* ele, uint8_t page);

uint8_t grid_ui_page_get_activepage(struct grid_ui_model* ui);
uint8_t grid_ui_page_get_next(struct grid_ui_model* ui);
uint8_t grid_ui_page_get_prev(struct grid_ui_model* ui);

void grid_ui_page_load(struct grid_ui_model* ui, uint8_t page);
void grid_ui_page_load_success_callback(uint8_t lastheader);

void grid_ui_page_clear_template_parameters(struct grid_ui_model* ui, uint8_t page);
uint8_t grid_ui_page_change_is_enabled(struct grid_ui_model* ui);

struct grid_ui_event* grid_ui_event_find(struct grid_ui_element* ele, uint8_t event_type);

uint8_t grid_ui_event_isdefault_actionstring(struct grid_ui_event* eve, char* action_string);
void grid_ui_event_register_actionstring(struct grid_ui_event* eve, char* targetstring);
void grid_ui_event_generate_actionstring(struct grid_ui_event* eve, char* targetstring);
void grid_ui_event_get_actionstring(struct grid_ui_event* eve, char* targetstring);
int grid_ui_event_recall_configuration(struct grid_ui_model* ui, uint8_t page, uint8_t element, uint8_t event_type, char* targetstring);

uint8_t grid_ui_event_istriggered(struct grid_ui_event* eve);
uint8_t grid_ui_event_istriggered_local(struct grid_ui_event* eve);
uint16_t grid_ui_event_count_istriggered(struct grid_ui_model* ui);
uint16_t grid_ui_event_count_istriggered_local(struct grid_ui_model* ui);

struct grid_ui_element* grid_ui_element_find(struct grid_ui_model* ui, uint8_t element_number);

void grid_ui_element_malloc_events(struct grid_ui_element* ele, int capacity);
void grid_ui_element_timer_set(struct grid_ui_element* ele, uint32_t duration);
void grid_ui_element_timer_source(struct grid_ui_element* ele, uint8_t source);
void grid_ui_element_set_template_parameter(struct grid_ui_element* ele, uint8_t template_index, int32_t value);
int32_t grid_ui_element_get_template_parameter(struct grid_ui_element* ele, uint8_t template_index);

enum grid_ui_bulk_status_t grid_ui_get_bulk_status(struct grid_ui_model* ui);
uint8_t grid_ui_bulk_get_response_code(struct grid_ui_model* ui, uint8_t id);
int grid_ui_bulk_anything_is_in_progress(struct grid_ui_model* ui);
int grid_ui_bulk_is_in_progress(struct grid_ui_model* ui, enum grid_ui_bulk_status_t);
uint8_t grid_ui_bulk_get_lastheader(struct grid_ui_model* ui);

int grid_ui_bulk_operation_init(struct grid_ui_model* ui, enum grid_ui_bulk_status_t status, uint8_t page, uint8_t lastheader_id, void (*success_cb)(uint8_t));

void grid_ui_bulk_pageread_next(struct grid_ui_model* ui);
void grid_ui_bulk_pagestore_next(struct grid_ui_model* ui);
void grid_ui_bulk_pageclear_next(struct grid_ui_model* ui);

#define GRID_UI_CONFIG_PATH "config.toml"

int grid_ui_bulk_conf_init(struct grid_ui_model* ui, enum grid_ui_bulk_status_t status, uint8_t lastheader_id, void (*success_cb)(uint8_t));

void grid_ui_bulk_confread_next(struct grid_ui_model* ui);
void grid_ui_bulk_confstore_next(struct grid_ui_model* ui);

int grid_ui_bulk_nvmerase_init(struct grid_ui_model* ui, uint8_t lastheader_id, void (*success_cb)(uint8_t));
void grid_ui_bulk_nvmerase_next(struct grid_ui_model* ui);

void grid_port_process_ui_local_UNSAFE(struct grid_ui_model* ui);

void grid_port_process_ui_UNSAFE(struct grid_ui_model* ui);

#endif /* GRID_UI_H */
