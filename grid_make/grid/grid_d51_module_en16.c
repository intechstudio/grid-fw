#include "grid_d51_module_en16.h"

#include "grid_ui_button.h"
#include "grid_ui_encoder.h"
#include "grid_ui_system.h"

static uint8_t UI_SPI_TX_BUFFER[14] = {0};
static uint8_t UI_SPI_RX_BUFFER[14] = {0};

#define GRID_MODULE_EN16_BUT_NUM 16

#define GRID_MODULE_EN16_ENC_NUM 16

static struct grid_ui_button_state ui_button_state[GRID_MODULE_EN16_BUT_NUM] = {0};
static struct grid_ui_encoder_state ui_encoder_state[GRID_MODULE_EN16_ENC_NUM] = {0};
static struct grid_ui_element* elements = NULL;

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
  for (uint8_t j = 0; j < GRID_MODULE_EN16_ENC_NUM; j++) {

    uint8_t new_value = (UI_SPI_RX_BUFFER[j / 2] >> (4 * (j % 2))) & 0x0F;

    uint8_t i = encoder_position_lookup[j];

    struct grid_ui_element* ele = &elements[i];

    grid_ui_encoder_store_input(ele, &ui_encoder_state[i], new_value);

    uint8_t button_value = new_value & 0b00000100;

    grid_ui_button_store_input(ele, &ui_button_state[i], button_value, 1);
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

  for (int i = 0; i < GRID_MODULE_EN16_BUT_NUM; ++i) {
    grid_ui_button_state_init(&ui_button_state[i], 1, 0.5, 0.2);
  }

  uint8_t detent = grid_sys_get_hwcfg(&grid_sys_state) != GRID_MODULE_EN16_ND_RevA && grid_sys_get_hwcfg(&grid_sys_state) != GRID_MODULE_EN16_ND_RevD;
  int8_t direction = grid_hwcfg_module_encoder_dir(&grid_sys_state);
  for (uint8_t i = 0; i < GRID_MODULE_EN16_ENC_NUM; i++) {
    grid_ui_encoder_state_init(&ui_encoder_state[i], detent, direction);
  }

  elements = grid_ui_model_get_elements(&grid_ui_state);

  hardware_init();

  hardware_start_transfer();
}
