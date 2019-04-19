/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file
 * to avoid losing it when reconfiguring.
 */

#include "driver_examples.h"
#include "driver_init.h"
#include "utils.h"

static void convert_cb_ADC_0(const struct adc_async_descriptor *const descr, const uint8_t channel)
{
}

/**
 * Example of using ADC_0 to generate waveform.
 */
void ADC_0_example(void)
{
	adc_async_enable_channel(&ADC_0, 0);
	adc_async_register_callback(&ADC_0, 0, ADC_ASYNC_CONVERT_CB, convert_cb_ADC_0);
	adc_async_start_conversion(&ADC_0);
}

static void convert_cb_ADC_1(const struct adc_async_descriptor *const descr, const uint8_t channel)
{
}

/**
 * Example of using ADC_1 to generate waveform.
 */
void ADC_1_example(void)
{
	adc_async_enable_channel(&ADC_1, 0);
	adc_async_register_callback(&ADC_1, 0, ADC_ASYNC_CONVERT_CB, convert_cb_ADC_1);
	adc_async_start_conversion(&ADC_1);
}

/**
 * Example of using GRID_AUX to write "Hello World" using the IO abstraction.
 *
 * Since the driver is asynchronous we need to use statically allocated memory for string
 * because driver initiates transfer and then returns before the transmission is completed.
 *
 * Once transfer has been completed the tx_cb function will be called.
 */

static uint8_t example_GRID_AUX[12] = "Hello World!";

static void tx_cb_GRID_AUX(const struct usart_async_descriptor *const io_descr)
{
	/* Transfer completed */
}

void GRID_AUX_example(void)
{
	struct io_descriptor *io;

	usart_async_register_callback(&GRID_AUX, USART_ASYNC_TXC_CB, tx_cb_GRID_AUX);
	/*usart_async_register_callback(&GRID_AUX, USART_ASYNC_RXC_CB, rx_cb);
	usart_async_register_callback(&GRID_AUX, USART_ASYNC_ERROR_CB, err_cb);*/
	usart_async_get_io_descriptor(&GRID_AUX, &io);
	usart_async_enable(&GRID_AUX);

	io_write(io, example_GRID_AUX, 12);
}

static uint8_t SYS_I2C_example_str[12] = "Hello World!";

void SYS_I2C_tx_complete(struct i2c_m_async_desc *const i2c)
{
}

void SYS_I2C_example(void)
{
	struct io_descriptor *SYS_I2C_io;

	i2c_m_async_get_io_descriptor(&SYS_I2C, &SYS_I2C_io);
	i2c_m_async_enable(&SYS_I2C);
	i2c_m_async_register_callback(&SYS_I2C, I2C_M_ASYNC_TX_COMPLETE, (FUNC_PTR)SYS_I2C_tx_complete);
	i2c_m_async_set_slaveaddr(&SYS_I2C, 0x12, I2C_M_SEVEN);

	io_write(SYS_I2C_io, SYS_I2C_example_str, 12);
}

/**
 * Example of using GRID_LED to write "Hello World" using the IO abstraction.
 *
 * Since the driver is asynchronous we need to use statically allocated memory for string
 * because driver initiates transfer and then returns before the transmission is completed.
 *
 * Once transfer has been completed the tx_cb function will be called.
 */

static uint8_t example_GRID_LED[12] = "Hello World!";

static void tx_complete_cb_GRID_LED(struct _dma_resource *resource)
{
	/* Transfer completed */
}

void GRID_LED_example(void)
{
	struct io_descriptor *io;
	spi_m_dma_get_io_descriptor(&GRID_LED, &io);

	spi_m_dma_register_callback(&GRID_LED, SPI_M_DMA_CB_TX_DONE, tx_complete_cb_GRID_LED);
	spi_m_dma_enable(&GRID_LED);
	io_write(io, example_GRID_LED, 12);
}

void delay_example(void)
{
	delay_ms(5000);
}
