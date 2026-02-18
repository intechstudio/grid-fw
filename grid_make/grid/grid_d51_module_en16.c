#include "grid_d51_module_en16.h"

#include <string.h>

#include "grid_d51_encoder.h"
#include "grid_platform.h"
#include "grid_ui_button.h"
#include "grid_ui_encoder.h"

#define GRID_MODULE_EN16_ENCODER_COUNT 16

static struct grid_ui_model* ui_ptr = NULL;

static void en16_process_encoder(struct grid_encoder_result* result) {

  uint8_t encoder_position_lookup[GRID_MODULE_EN16_ENCODER_COUNT] = {14, 15, 10, 11, 6, 7, 2, 3, 12, 13, 8, 9, 4, 5, 0, 1};

  for (uint8_t i = 0; i < GRID_MODULE_EN16_ENCODER_COUNT; i++) {

    uint8_t nibble = GRID_UI_ENCODER_NIBBLE_FROM_BUFFER(result->data, i);
    uint8_t element_index = encoder_position_lookup[i];

    struct grid_ui_encoder_sample sample = GRID_UI_ENCODER_SAMPLE_FROM_NIBBLE(nibble);
    struct grid_ui_element* ele = &ui_ptr->element_list[element_index];
    grid_ui_encoder_store_input(ele, sample);
  }
}

void grid_d51_module_en16_init(struct grid_sys_model* sys, struct grid_ui_model* ui, struct grid_d51_encoder_model* enc) {

  ui_ptr = ui;

  uint8_t detent = grid_hwcfg_module_encoder_is_detent(sys);
  int8_t direction = grid_hwcfg_module_encoder_dir(sys);
  for (int i = 0; i < ui->element_list_length; ++i) {
    struct grid_ui_element* ele = &ui->element_list[i];
    if (ele->type == GRID_PARAMETER_ELEMENT_ENCODER) {
      struct grid_ui_encoder_state* state = (struct grid_ui_encoder_state*)ele->primary_state;
      grid_ui_encoder_configure(state, detent, direction, 1, 0.5, 0.2);
    }
  }

  uint8_t transfer_length = 1 + GRID_MODULE_EN16_ENCODER_COUNT / 2;
  // SPI clock rate chosen so callback fires at 2000 Hz: rate = 2000 * transfer_length * 8
  uint32_t clock_rate = 2000 * transfer_length * 8;
  grid_d51_encoder_init(enc, transfer_length, clock_rate, en16_process_encoder);
}
