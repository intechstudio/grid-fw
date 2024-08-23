#include "../grid_ui_encoder.h"
#include "../grid_ui.h"
#include "unity.h"

#include <memory.h>
#include <stdio.h>

void setUp(void) {
  // set stuff up here
}

void tearDown(void) {
  // clean stuff up here
}

uint64_t grid_platform_rtc_get_micros(void) { return 0; }

uint64_t grid_platform_rtc_get_elapsed_time(uint64_t told) { return 10 - told; }

void test_function_should_calculateRelativeMode(void) {

  struct grid_ui_model* ui = &grid_ui_state;
 
  grid_ui_model_init(ui, 1);
  ui->ui_interaction_enabled = 1;
  struct grid_ui_element* ele = &ui->element_list[0];
  grid_ui_element_encoder_init(ele);
  
  grid_ui_template_buffer_create(ele);
  grid_ui_page_clear_template_parameters(ui, 0);
  ele->template_parameter_list = ele->template_buffer_list_head->template_parameter_list;

  if (ele->page_change_cb != NULL) {

    ele->page_change_cb(ele, 0, 0);
  }

  for (uint8_t i=0; i<ele->template_parameter_list_length; i++) {
    printf("template_parameter_list[%d] = %d\n", i, ele->template_parameter_list[i]);
  }

  uint64_t last_real_time = 0;
  int16_t delta = 1;
  uint8_t is_endless_pot = 0;

  grid_ui_encoder_update_trigger(ele, &last_real_time, delta, is_endless_pot);

  for (uint8_t i=0; i<ele->template_parameter_list_length; i++) {
    printf("template_parameter_list[%d] = %d\n", i, ele->template_parameter_list[i]);
  }

  struct grid_ui_event* eve = grid_ui_event_find(ele, GRID_PARAMETER_EVENT_ENCODER);

  printf("test_function_should_calculateRelativeMode: %d\n", grid_ui_event_istriggered(eve));

  TEST_ASSERT_EQUAL_UINT8(1, grid_ui_event_istriggered(eve)); // event triggering should happen

  // ....
}


// not needed when using generate_test_runner.rb
int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_function_should_calculateRelativeMode);


  return UNITY_END();
}
