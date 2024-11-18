#include "grid_d51_module_ef44.h"

#include "grid_ui_encoder.h"
#include "grid_ui_potmeter.h"
#include "grid_ui_system.h"

static volatile uint8_t adc_complete_count = 0;
static volatile uint8_t multiplexer_index = 0;

static uint64_t last_real_time[4] = {0};

static uint8_t UI_SPI_TX_BUFFER[14] = {0};
static uint8_t UI_SPI_RX_BUFFER[14] = {0};

static volatile uint8_t UI_SPI_RX_BUFFER_LAST[16] = {0};

#define GRID_MODULE_EF44_ENC_NUM 4

static struct grid_ui_encoder_state ui_encoder_state[GRID_MODULE_EF44_ENC_NUM];

static uint8_t detent;

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
    uint8_t old_value = UI_SPI_RX_BUFFER_LAST[j];

    UI_SPI_RX_BUFFER_LAST[j] = new_value;

    uint8_t i = encoder_position_lookup[j];

    grid_ui_encoder_store_input(&ui_encoder_state[i], i, old_value, new_value, detent);
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

  // FAKE CALIBRATION to compensate oversampling and decimation
  uint32_t input_0 = adcresult_0 * 1.03; // 1.03
  if (input_0 > (1 << 16) - 1) {
    input_0 = (1 << 16) - 1;
  }
  adcresult_0 = input_0;

  uint32_t input_1 = adcresult_1 * 1.03;
  if (input_1 > (1 << 16) - 1) {
    input_1 = (1 << 16) - 1;
  }
  adcresult_1 = input_1;

  grid_ui_potmeter_store_input(adc_index_0 + 4, &last_real_time[adc_index_0], adcresult_0, 16); // 16 bit analog values
  grid_ui_potmeter_store_input(adc_index_1 + 4, &last_real_time[adc_index_1], adcresult_1, 16);

  adc_complete_count = 0;
  hardware_adc_start_transfer();
}

static void hardware_init(void) {

  gpio_set_pin_level(PIN_UI_SPI_CS0, false);
  gpio_set_pin_direction(PIN_UI_SPI_CS0, GPIO_DIRECTION_OUT);

  spi_m_async_set_mode(&UI_SPI, SPI_MODE_3);
  spi_m_async_set_baudrate(&UI_SPI,
                           1000000); // was 400000 check clock div setting

  spi_m_async_register_callback(&UI_SPI, SPI_M_ASYNC_CB_XFER, spi_transfer_complete_cb);

  adc_async_register_callback(&ADC_0, 0, ADC_ASYNC_CONVERT_CB, adc_transfer_complete_cb);
  adc_async_register_callback(&ADC_1, 0, ADC_ASYNC_CONVERT_CB, adc_transfer_complete_cb);

  adc_async_enable_channel(&ADC_0, 0);
  adc_async_enable_channel(&ADC_1, 0);
}

void grid_module_ef44_init() {

  grid_module_ef44_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);

  detent = grid_sys_get_hwcfg(&grid_sys_state) != GRID_MODULE_EF44_ND_RevD;

  hardware_init();

  hardware_spi_start_transfer();
  hardware_adc_start_transfer();
}
