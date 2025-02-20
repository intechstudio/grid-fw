#pragma once

#ifndef GRID_UI_H_INCLUDED
#define GRID_UI_H_INCLUDED

// only for uint definitions
#include <stdint.h>
// only for malloc
#include <stdlib.h>

#include <string.h>

#include "grid_ain.h"
#include "grid_cal.h"
#include "grid_lua_api.h"
#include "grid_protocol.h"

union grid_ui_file_handle {
  char fname[50];
  void* toc_ptr;
};

enum grid_ui_status_t {
  GRID_UI_STATUS_UNDEFINED,
  GRID_UI_STATUS_INITIALIZED,
  GRID_UI_STATUS_TRAP,

  GRID_UI_STATUS_OK,

  GRID_UI_STATUS_READY,
  GRID_UI_STATUS_TRIGGERED,

  GRID_UI_STATUS_TRIGGERED_LOCAL

};

struct grid_ui_event {

  enum grid_ui_status_t status;

  struct grid_ui_element* parent;
  uint8_t index;

  const char* default_actionstring;

  enum grid_ui_status_t trigger;

  uint8_t type;

  uint16_t action_string_length;

  char* action_string;

  char function_name[10];

  uint8_t cfg_changed_flag;
  uint8_t cfg_default_flag;
  uint8_t cfg_flashempty_flag;
};

struct grid_ui_template_buffer {

  struct grid_ui_element* parent;
  uint8_t page_number;
  uint8_t status;
  int32_t* template_parameter_list;
  struct grid_ui_template_buffer* next;
};

typedef void (*template_init_t)(struct grid_ui_template_buffer*);

struct grid_ui_element {
  enum grid_ui_status_t status;

  uint8_t timer_source_is_midi;
  uint32_t timer_event_helper;

  struct grid_ui_model* parent;
  uint8_t index;

  uint8_t type;

  void (*template_initializer)(struct grid_ui_template_buffer*);

  struct grid_ui_template_buffer* template_buffer_list_head;

  uint8_t template_parameter_list_length;
  int32_t* template_parameter_list;

  uint8_t template_parameter_element_position_index_1;
  uint8_t template_parameter_element_position_index_2;

  void (*page_change_cb)(struct grid_ui_element*, uint8_t, uint8_t);
  void (*event_clear_cb)(struct grid_ui_event*);

  uint8_t event_list_length;
  struct grid_ui_event* event_list;
};

enum grid_ui_bluk_status_t { GRID_UI_BULK_READY = 0, GRID_UI_BULK_READ_PROGRESS, GRID_UI_BULK_STORE_PROGRESS, GRID_UI_BULK_CLEAR_PROGRESS, GRID_UI_BULK_ERASE_PROGRESS };

// struct grid_lua_model;

struct grid_ui_model {

  enum grid_ui_status_t status;

  void* busy_semaphore;

  void (*busy_semaphore_lock_fn)(void*);
  void (*busy_semaphore_release_fn)(void*);

  uint8_t page_activepage;
  uint8_t page_count;

  uint8_t page_change_enabled;
  uint8_t page_negotiated;

  uint8_t mapmode_state;

  uint8_t element_list_length;
  struct grid_ui_element* element_list;

  enum grid_ui_bluk_status_t bulk_status;
  void (*bulk_success_callback)(uint8_t);
  uint8_t bulk_lastheader_id;
  int bulk_last_page;
  int bulk_last_element;
  int bulk_last_event;

  // void (*lua_ui_init_callback)(struct grid_lua_model*);
  lua_ui_init_callback_t lua_ui_init_callback;
};

extern struct grid_ui_model grid_ui_state;

void grid_ui_model_init(struct grid_ui_model* ui, uint8_t element_list_length);

void grid_ui_semaphore_init(struct grid_ui_model* ui, void* busy_semaphore, void (*lock_fn)(void*), void (*release_fn)(void*));
void grid_ui_semaphore_lock(struct grid_ui_model* ui);
void grid_ui_semaphore_release(struct grid_ui_model* ui);

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
uint32_t grid_ui_event_render_event(struct grid_ui_event* eve, char* target_string);
void grid_ui_event_generate_actionstring(struct grid_ui_event* eve, char* targetstring);
void grid_ui_event_get_actionstring(struct grid_ui_event* eve, char* targetstring);
uint32_t grid_ui_event_render_action(struct grid_ui_event* eve, char* target_string);
int grid_ui_event_recall_configuration(struct grid_ui_model* ui, uint8_t page, uint8_t element, uint8_t event_type, char* targetstring);

// grid ui event trigger
void grid_ui_event_trigger(struct grid_ui_event* eve);
void grid_ui_event_trigger_local(struct grid_ui_event* eve);
void grid_ui_event_reset(struct grid_ui_event* eve);
uint8_t grid_ui_event_istriggered(struct grid_ui_event* eve);
uint8_t grid_ui_event_istriggered_local(struct grid_ui_event* eve);
uint16_t grid_ui_event_count_istriggered(struct grid_ui_model* ui);
uint16_t grid_ui_event_count_istriggered_local(struct grid_ui_model* ui);

struct grid_ui_element* grid_ui_element_find(struct grid_ui_model* ui, uint8_t element_number);

void grid_ui_element_timer_set(struct grid_ui_element* ele, uint32_t duration);
void grid_ui_element_timer_source(struct grid_ui_element* ele, uint8_t source);
void grid_ui_element_set_template_parameter(struct grid_ui_element* ele, uint8_t template_index, int32_t value);
int32_t grid_ui_element_get_template_parameter(struct grid_ui_element* ele, uint8_t template_index);

enum grid_ui_bluk_status_t grid_ui_get_bulk_status(struct grid_ui_model* ui);
int grid_ui_bulk_anything_is_in_progress(struct grid_ui_model* ui);
int grid_ui_bulk_is_in_progress(struct grid_ui_model* ui, enum grid_ui_bluk_status_t);
uint8_t grid_ui_bulk_get_lastheader(struct grid_ui_model* ui);

int grid_ui_bulk_pageread_init(struct grid_ui_model* ui, uint8_t page, uint8_t lastheader_id, void (*success_cb)(uint8_t));
void grid_ui_bulk_pageread_next(struct grid_ui_model* ui);

int grid_ui_bulk_pagestore_init(struct grid_ui_model* ui, uint8_t page, uint8_t lastheader_id, void (*success_cb)(uint8_t));
void grid_ui_bulk_pagestore_next(struct grid_ui_model* ui);

int grid_ui_bulk_pageclear_init(struct grid_ui_model* ui, uint8_t page, uint8_t lastheader_id, void (*success_cb)(uint8_t));
void grid_ui_bulk_pageclear_next(struct grid_ui_model* ui);

int grid_ui_bulk_nvmerase_init(struct grid_ui_model* ui, uint8_t lastheader_id, void (*success_cb)(uint8_t));
void grid_ui_bulk_nvmerase_next(struct grid_ui_model* ui);

void grid_port_process_ui_local_UNSAFE(struct grid_ui_model* ui);

void grid_port_process_ui_UNSAFE(struct grid_ui_model* ui);

#endif /* GRID_UI_H_INCLUDED */
