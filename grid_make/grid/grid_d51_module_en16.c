#include "grid_d51_module_en16.h"

#include <string.h>

#include "grid_platform.h"
#include "grid_ui_button.h"
#include "grid_ui_encoder.h"
#include "grid_ui_system.h"

static uint8_t UI_SPI_TX_BUFFER[14] = {0};
static uint8_t UI_SPI_RX_BUFFER[14] = {0};

#define GRID_MODULE_EN16_BUT_NUM 16

#define GRID_MODULE_EN16_ENC_NUM 16

static void hardware_start_transfer(void) {

  gpio_set_pin_level(PIN_UI_SPI_CS0, true);
  spi_m_async_enable(&UI_SPI);
  spi_m_async_transfer(&UI_SPI, UI_SPI_TX_BUFFER, UI_SPI_RX_BUFFER, 8);
}

static void spi_transfer_complete_cb(void) {

  /* Transfer completed */

  // printf("%d\r\n", _irq_get_current());

  // Set the shift registers to continuously load data until new transaction is
  // issued
  gpio_set_pin_level(PIN_UI_SPI_CS0, false);

  uint8_t encoder_position_lookup[GRID_MODULE_EN16_ENC_NUM] = {14, 15, 10, 11, 6, 7, 2, 3, 12, 13, 8, 9, 4, 5, 0, 1};

  // Buffer is only 8 bytes but we check all 16 encoders separately
  for (uint8_t i = 0; i < GRID_MODULE_EN16_ENC_NUM; i++) {

    uint8_t nibble = GRID_UI_ENCODER_NIBBLE_FROM_BUFFER(UI_SPI_RX_BUFFER, i);
    uint8_t element_index = encoder_position_lookup[i];

    struct grid_ui_encoder_sample sample = GRID_UI_ENCODER_SAMPLE_FROM_NIBBLE(nibble);
    grid_ui_encoder_store_input(&grid_ui_state, element_index, sample);
  }

  hardware_start_transfer();
}

static void hardware_init(void) {

  gpio_set_pin_level(PIN_UI_SPI_CS0, false);
  gpio_set_pin_direction(PIN_UI_SPI_CS0, GPIO_DIRECTION_OUT);

  spi_m_async_set_mode(&UI_SPI, SPI_MODE_3);
  spi_m_async_set_baudrate(&UI_SPI,
                           100000); // was 400000 check clock div setting

  spi_m_async_register_callback(&UI_SPI, SPI_M_ASYNC_CB_XFER, spi_transfer_complete_cb);
}

void grid_module_en16_init() {

  grid_module_en16_ui_init(NULL, &grid_led_state, &grid_ui_state);

  // Button state is in secondary_state for encoder elements
  for (int i = 0; i < GRID_MODULE_EN16_BUT_NUM; ++i) {
    struct grid_ui_element* ele = &grid_ui_state.element_list[i];
    struct grid_ui_button_state* state = (struct grid_ui_button_state*)ele->secondary_state;
    grid_ui_button_state_init(state, 1, 0.5, 0.2);
  }

  uint8_t detent = grid_hwcfg_module_encoder_is_detent(&grid_sys_state);
  int8_t direction = grid_hwcfg_module_encoder_dir(&grid_sys_state);
  for (uint8_t i = 0; i < GRID_MODULE_EN16_ENC_NUM; i++) {
    struct grid_ui_element* ele = &grid_ui_state.element_list[i];
    struct grid_ui_encoder_state* state = (struct grid_ui_encoder_state*)ele->primary_state;
    grid_ui_encoder_state_init(state, detent, direction);
  }

  hardware_init();

  hardware_start_transfer();
}
