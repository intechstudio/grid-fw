#include "test.h"

#include "grid_ui.h"
#include "grid_ui_encoder.h"

TEST_DECL(grid_ui_encoder_relative) {

  struct grid_ui_model* ui = &grid_ui_state;
  grid_ui_model_init(ui, 1);

  struct grid_ui_element* ele = grid_ui_element_find(ui, 0);
  TEST_ASSERT(ele);

  grid_ui_element_encoder_init(ele);

  struct grid_ui_event* eve = grid_ui_event_find(ele, GRID_PARAMETER_EVENT_ENCODER);
  TEST_ASSERT(eve);

  TEST_ASSERT(grid_ui_template_buffer_create(ele));
  grid_ui_page_clear_template_parameters(ui, 0);
  ele->template_parameter_list = ele->template_buffer_list_head->template_parameter_list;

  if (ele->page_change_cb) {
    ele->page_change_cb(ele, 0, 0);
  }

  // Set encoder to relative mode
  grid_ui_element_set_template_parameter(ele, GRID_LUA_FNC_E_ENCODER_MODE_index, 1);

  grid_ui_event_state_set(eve, GRID_EVE_STATE_INIT);
  if (eve->parent->event_clear_cb) {
    eve->parent->event_clear_cb(eve);
  }

  // In relative mode, value should be 64 after event reset
  TEST_ASSERT(64 == grid_ui_element_get_template_parameter(ele, GRID_LUA_FNC_E_ENCODER_VALUE_index));
  grid_ui_element_set_template_parameter(ele, GRID_LUA_FNC_E_ENCODER_MODE_index, 1);

  // In relative mode, in a range of [-100, 100], value should be 0 after reset
  grid_ui_element_set_template_parameter(ele, GRID_LUA_FNC_E_ENCODER_MIN_index, -100);
  grid_ui_element_set_template_parameter(ele, GRID_LUA_FNC_E_ENCODER_MAX_index, 100);
  grid_ui_event_state_set(eve, GRID_EVE_STATE_INIT);
  if (eve->parent->event_clear_cb) {
    eve->parent->event_clear_cb(eve);
  }
  TEST_ASSERT(0 == grid_ui_element_get_template_parameter(ele, GRID_LUA_FNC_E_ENCODER_VALUE_index));

  // In relative mode, in a range of [100, 200], value should be 150 after reset
  grid_ui_element_set_template_parameter(ele, GRID_LUA_FNC_E_ENCODER_MIN_index, 100);
  grid_ui_element_set_template_parameter(ele, GRID_LUA_FNC_E_ENCODER_MAX_index, 200);
  grid_ui_event_state_set(eve, GRID_EVE_STATE_INIT);
  if (eve->parent->event_clear_cb) {
    eve->parent->event_clear_cb(eve);
  }
  TEST_ASSERT(150 == grid_ui_element_get_template_parameter(ele, GRID_LUA_FNC_E_ENCODER_VALUE_index));

  uint64_t last_real_time = 0;
  grid_ui_encoder_update_trigger(ele, &last_real_time, 1);
  TEST_ASSERT(200 != grid_ui_element_get_template_parameter(ele, GRID_LUA_FNC_E_ENCODER_VALUE_index));

  return TEST_SUCCESS;
}
