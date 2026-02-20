#include "grid_d51_encoder.h"

#include <string.h>

#include "grid_d51_module.h"
#include "grid_platform.h"
#include <peripheral_clk_config.h>

struct grid_d51_encoder_model grid_d51_encoder_state;

static void spi_start_transfer(struct grid_d51_encoder_model* enc) {

  gpio_set_pin_level(PIN_UI_SPI_CS0, true);
  spi_m_async_enable(&UI_SPI);
  spi_m_async_transfer(&UI_SPI, NULL, enc->rx_buffer, enc->transfer_length);
}

static void spi_transfer_complete_cb(void) {

  struct grid_d51_encoder_model* enc = &grid_d51_encoder_state;

  gpio_set_pin_level(PIN_UI_SPI_CS0, false);

  struct grid_encoder_result result = {.data = enc->rx_buffer, .length = enc->transfer_length};
  enc->process_encoder(&result);

  spi_start_transfer(enc);
}

void grid_d51_encoder_init(struct grid_d51_encoder_model* enc, uint8_t transfer_length, uint32_t clock_rate, grid_process_encoder_t process_encoder) {

  enc->rx_buffer = grid_platform_allocate_volatile(transfer_length);
  memset(enc->rx_buffer, 0, transfer_length);

  enc->transfer_length = transfer_length;
  enc->process_encoder = process_encoder;

  gpio_set_pin_level(PIN_UI_SPI_CS0, false);
  gpio_set_pin_direction(PIN_UI_SPI_CS0, GPIO_DIRECTION_OUT);

  spi_m_async_set_mode(&UI_SPI, SPI_MODE_3);
  uint32_t baud_reg = (CONF_GCLK_SERCOM3_CORE_FREQUENCY / (2 * clock_rate)) - 1;
  spi_m_async_set_baudrate(&UI_SPI, baud_reg);

  spi_m_async_register_callback(&UI_SPI, SPI_M_ASYNC_CB_XFER, spi_transfer_complete_cb);

  spi_start_transfer(enc);
}
