#include "grid_d51_module_ef44.h"

#include <string.h>

#include "grid_ain.h"
#include "grid_asc.h"
#include "grid_platform.h"
#include "grid_ui_button.h"
#include "grid_ui_encoder.h"
#include "grid_ui_potmeter.h"
#include "grid_ui_system.h"

#include "grid_cal.h"
#include "grid_config.h"

static volatile uint8_t adc_complete_count = 0;
static volatile uint8_t multiplexer_index = 0;
static const uint8_t mux_positions_bm = 0b00000011;
#define GRID_MODULE_EF44_BUT_NUM 4
#define GRID_MODULE_EF44_ENC_NUM 4
#define GRID_MODULE_EF44_POT_NUM 4

static const uint8_t mux_element_lookup[2][2] = {
    {4, 5}, // MUX_0 -> ADC_1
    {6, 7}, // MUX_1 -> ADC_0
};
static uint16_t element_invert_bm = 0;

static struct adc_async_descriptor* adcs[2] = {&ADC_1, &ADC_0};

static uint64_t last_real_time[4] = {0};

static uint8_t UI_SPI_TX_BUFFER[14] = {0};
static uint8_t UI_SPI_RX_BUFFER[14] = {0};

static struct grid_ui_button_state* ui_button_state = NULL;
static struct grid_ui_encoder_state* ui_encoder_state = NULL;
static struct grid_ui_potmeter_state* ui_potmeter_state = NULL;
static struct grid_asc* asc_state = NULL;

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

    grid_ui_encoder_store_input(&grid_ui_state, i, &ui_encoder_state[i], new_value);

    uint8_t button_value = new_value & 0b00000100;

    grid_ui_button_store_input(&grid_ui_state, i, &ui_button_state[i], button_value, 1);
  }

  hardware_spi_start_transfer();
}

static void adc_transfer_complete_cb(void) {

  if (adc_complete_count == 0) {
    adc_complete_count++;
    return;
  }

  /* Read and process both channels */

  for (int channel = 0; channel < 2; channel++) {
    uint16_t raw = 0;
    adc_async_read_channel(adcs[channel], 0, &raw, 2);

    uint8_t element_index = mux_element_lookup[channel][multiplexer_index];
    uint16_t inverted = GRID_ADC_INVERT_COND(raw, element_index, element_invert_bm);
    uint16_t downsampled = GRID_ADC_DOWNSAMPLE(inverted);

    uint16_t processed;
    if (!grid_asc_process(asc_state, element_index, downsampled, &processed)) {
      continue;
    }

    grid_ui_potmeter_store_input(&grid_ui_state, element_index, &ui_potmeter_state[element_index - GRID_MODULE_EF44_ENC_NUM], processed, GRID_AIN_INTERNAL_RESOLUTION);
  }

  /* Update the multiplexer for next iteration */

  GRID_MUX_INCREMENT(multiplexer_index, mux_positions_bm);
  grid_platform_mux_write(multiplexer_index);

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

  ui_button_state = grid_platform_allocate_volatile(GRID_MODULE_EF44_BUT_NUM * sizeof(struct grid_ui_button_state));
  ui_encoder_state = grid_platform_allocate_volatile(GRID_MODULE_EF44_ENC_NUM * sizeof(struct grid_ui_encoder_state));
  ui_potmeter_state = grid_platform_allocate_volatile(GRID_MODULE_EF44_POT_NUM * sizeof(struct grid_ui_potmeter_state));
  asc_state = grid_platform_allocate_volatile(8 * sizeof(struct grid_asc));
  memset(ui_button_state, 0, GRID_MODULE_EF44_BUT_NUM * sizeof(struct grid_ui_button_state));
  memset(ui_encoder_state, 0, GRID_MODULE_EF44_ENC_NUM * sizeof(struct grid_ui_encoder_state));
  memset(ui_potmeter_state, 0, GRID_MODULE_EF44_POT_NUM * sizeof(struct grid_ui_potmeter_state));
  memset(asc_state, 0, 8 * sizeof(struct grid_asc));

  for (int i = 0; i < GRID_MODULE_EF44_BUT_NUM; ++i) {
    grid_ui_button_state_init(&ui_button_state[i], 1, 0.5, 0.2);
  }

  for (int i = 0; i < GRID_MODULE_EF44_POT_NUM; ++i) {
    grid_ui_potmeter_state_init(&ui_potmeter_state[i], GRID_AIN_INTERNAL_RESOLUTION, GRID_POTMETER_DEADZONE, GRID_POTMETER_CENTER);
  }

  grid_asc_array_set_factors(asc_state, 8, 4, 4, 1);

  uint8_t detent = grid_hwcfg_module_encoder_is_detent(&grid_sys_state);
  int8_t direction = grid_hwcfg_module_encoder_dir(&grid_sys_state);
  for (uint8_t i = 0; i < GRID_MODULE_EF44_ENC_NUM; i++) {
    grid_ui_encoder_state_init(&ui_encoder_state[i], detent, direction);
  }

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
