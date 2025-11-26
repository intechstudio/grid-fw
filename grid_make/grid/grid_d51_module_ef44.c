#include "grid_d51_module_ef44.h"

#include "grid_ui_button.h"
#include "grid_ui_encoder.h"
#include "grid_ui_potmeter.h"
#include "grid_ui_system.h"

#include "grid_cal.h"
#include "grid_config.h"

static volatile uint8_t adc_complete_count = 0;
static volatile uint8_t multiplexer_index = 0;

static uint64_t last_real_time[4] = {0};

static uint8_t UI_SPI_TX_BUFFER[14] = {0};
static uint8_t UI_SPI_RX_BUFFER[14] = {0};

#define GRID_MODULE_EF44_BUT_NUM 4

#define GRID_MODULE_EF44_ENC_NUM 4

#define GRID_MODULE_EF44_POT_NUM 4

static struct grid_ui_button_state ui_button_state[GRID_MODULE_EF44_BUT_NUM] = {0};
static struct grid_ui_encoder_state ui_encoder_state[GRID_MODULE_EF44_ENC_NUM] = {0};
static struct grid_ui_potmeter_state ui_potmeter_state[GRID_MODULE_EF44_POT_NUM] = {0};
static struct grid_ui_element* elements = NULL;

static void hardware_spi_start_transfer(void) {

  gpio_set_pin_level(PIN_UI_SPI_CS0, true);
  spi_m_async_enable(&UI_SPI);
  spi_m_async_transfer(&UI_SPI, UI_SPI_TX_BUFFER, UI_SPI_RX_BUFFER, 8);
}

static void hardware_adc_start_transfer(void) {

  adc_async_start_conversion(&ADC_0);
  adc_async_start_conversion(&ADC_1);
}

static void spi_transfer_complete_cb(void) {

  /* Transfer completed */

  // Set the shift registers to continuously load data until new transaction is
  // issued
  gpio_set_pin_level(PIN_UI_SPI_CS0, false);

  uint8_t encoder_position_lookup[GRID_MODULE_EF44_ENC_NUM] = {2, 3, 0, 1};

  // Buffer is only 8 bytes but we check all 16 encoders separately
  for (uint8_t j = 0; j < GRID_MODULE_EF44_ENC_NUM; j++) {

    uint8_t new_value = (UI_SPI_RX_BUFFER[j / 2] >> (4 * (j % 2))) & 0x0F;

    uint8_t i = encoder_position_lookup[j];

    struct grid_ui_element* ele = &elements[i];

    grid_ui_encoder_store_input(ele, &ui_encoder_state[i], new_value);

    uint8_t button_value = new_value & 0b00000100;

    grid_ui_button_store_input(ele, &ui_button_state[i], button_value, 1);
  }

  hardware_spi_start_transfer();
}

static void adc_transfer_complete_cb(void) {

  if (adc_complete_count == 0) {
    adc_complete_count++;
    return;
  }

  /* Read conversion results */

  uint16_t adcresult_0 = 0;
  uint16_t adcresult_1 = 0;

  uint8_t adc_index_0 = multiplexer_index + 2;
  uint8_t adc_index_1 = multiplexer_index;

  adc_async_read_channel(&ADC_0, 0, &adcresult_0, 2);
  adc_async_read_channel(&ADC_1, 0, &adcresult_1, 2);

  /* Update the multiplexer */

  multiplexer_index++;
  multiplexer_index %= 2;

  gpio_set_pin_level(MUX_A, multiplexer_index / 1 % 2);
  gpio_set_pin_level(MUX_B, multiplexer_index / 2 % 2);
  gpio_set_pin_level(MUX_C, multiplexer_index / 4 % 2);

  struct grid_ui_element* ele_0 = &elements[adc_index_0 + 4];

  grid_ui_potmeter_store_input(ele_0, adc_index_0 + 4, &ui_potmeter_state[adc_index_0], adcresult_0 >> 4, 12);

  struct grid_ui_element* ele_1 = &elements[adc_index_1 + 4];

  grid_ui_potmeter_store_input(ele_1, adc_index_1 + 4, &ui_potmeter_state[adc_index_1], adcresult_1 >> 4, 12);

  adc_complete_count = 0;
  hardware_adc_start_transfer();
}

static void hardware_init(void) {

  gpio_set_pin_level(PIN_UI_SPI_CS0, false);
  gpio_set_pin_direction(PIN_UI_SPI_CS0, GPIO_DIRECTION_OUT);

  spi_m_async_set_mode(&UI_SPI, SPI_MODE_3);
  spi_m_async_set_baudrate(&UI_SPI,
                           100000); // was 400000 check clock div setting

  spi_m_async_register_callback(&UI_SPI, SPI_M_ASYNC_CB_XFER, spi_transfer_complete_cb);

  adc_async_register_callback(&ADC_0, 0, ADC_ASYNC_CONVERT_CB, adc_transfer_complete_cb);
  adc_async_register_callback(&ADC_1, 0, ADC_ASYNC_CONVERT_CB, adc_transfer_complete_cb);

  adc_async_enable_channel(&ADC_0, 0);
  adc_async_enable_channel(&ADC_1, 0);
}

void grid_module_ef44_init() {

  grid_module_ef44_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);

  for (int i = 0; i < GRID_MODULE_EF44_BUT_NUM; ++i) {
    grid_ui_button_state_init(&ui_button_state[i], 1, 0.5, 0.2);
  }

  for (int i = 0; i < GRID_MODULE_EF44_POT_NUM; ++i) {
    grid_ui_potmeter_state_init(&ui_potmeter_state[i], 12, 64, 2048);
  }

  uint8_t detent = grid_sys_get_hwcfg(&grid_sys_state) != GRID_MODULE_EF44_ND_RevD;
  int8_t direction = grid_hwcfg_module_encoder_dir(&grid_sys_state);
  for (uint8_t i = 0; i < GRID_MODULE_EF44_ENC_NUM; i++) {
    grid_ui_encoder_state_init(&ui_encoder_state[i], detent, direction);
  }

  elements = grid_ui_model_get_elements(&grid_ui_state);

  grid_config_init(&grid_config_state, &grid_cal_state);

  grid_cal_init(&grid_cal_state, grid_ui_state.element_list_length, 12);

  for (int i = 4; i < 8; ++i) {
    assert(grid_cal_set(&grid_cal_state, i, GRID_CAL_LIMITS, &ui_potmeter_state[i - 4].limits) == 0);
  }

  assert(grid_ui_bulk_conf_init(&grid_ui_state, GRID_UI_BULK_CONFREAD_PROGRESS, 0, NULL) == 0);
  grid_ui_bulk_confread_next(&grid_ui_state);

  hardware_init();

  hardware_spi_start_transfer();
  hardware_adc_start_transfer();
}
