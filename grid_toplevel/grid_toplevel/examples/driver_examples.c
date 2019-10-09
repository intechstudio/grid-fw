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

/* CRC Data in flash */
COMPILER_ALIGNED(4)
static const uint32_t crc_datas[] = {0x00000000,
                                     0x11111111,
                                     0x22222222,
                                     0x33333333,
                                     0x44444444,
                                     0x55555555,
                                     0x66666666,
                                     0x77777777,
                                     0x88888888,
                                     0x99999999};

/**
 * Example of using CRC_0 to Calculate CRC32 for a buffer.
 */
void CRC_0_example(void)
{
	/* The initial value used for the CRC32 calculation usually be 0xFFFFFFFF,
	 * but can be, for example, the result of a previous CRC32 calculation if
	 * generating a common CRC32 of separate memory blocks.
	 */
	uint32_t crc = 0xFFFFFFFF;
	uint32_t crc2;
	uint32_t ind;

	crc_sync_enable(&CRC_0);
	crc_sync_crc32(&CRC_0, (uint32_t *)crc_datas, 10, &crc);

	/* The read value must be complemented to match standard CRC32
	 * implementations or kept non-inverted if used as starting point for
	 * subsequent CRC32 calculations.
	 */
	crc ^= 0xFFFFFFFF;

	/* Calculate the same data with subsequent CRC32 calculations, the result
	 * should be same as previous way.
	 */
	crc2 = 0xFFFFFFFF;
	for (ind = 0; ind < 10; ind++) {
		crc_sync_crc32(&CRC_0, (uint32_t *)&crc_datas[ind], 1, &crc2);
	}
	crc2 ^= 0xFFFFFFFF;

	/* The calculate result should be same. */
	while (crc != crc2)
		;
}

static uint8_t src_data[512];
static uint8_t chk_data[512];
/**
 * Example of using FLASH_0 to read and write Flash main array.
 */
void FLASH_0_example(void)
{
	uint32_t page_size;
	uint16_t i;

	/* Init source data */
	page_size = flash_get_page_size(&FLASH_0);

	for (i = 0; i < page_size; i++) {
		src_data[i] = i;
	}

	/* Write data to flash */
	flash_write(&FLASH_0, 0x3200, src_data, page_size);

	/* Read data from flash */
	flash_read(&FLASH_0, 0x3200, chk_data, page_size);
}

static struct timer_task RTC_Scheduler_task1, RTC_Scheduler_task2;
/**
 * Example of using RTC_Scheduler.
 */
static void RTC_Scheduler_task1_cb(const struct timer_task *const timer_task)
{
}

static void RTC_Scheduler_task2_cb(const struct timer_task *const timer_task)
{
}

void RTC_Scheduler_example(void)
{
	RTC_Scheduler_task1.interval = 100;
	RTC_Scheduler_task1.cb       = RTC_Scheduler_task1_cb;
	RTC_Scheduler_task1.mode     = TIMER_TASK_REPEAT;
	RTC_Scheduler_task2.interval = 200;
	RTC_Scheduler_task2.cb       = RTC_Scheduler_task2_cb;
	RTC_Scheduler_task2.mode     = TIMER_TASK_REPEAT;

	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_task1);
	timer_add_task(&RTC_Scheduler, &RTC_Scheduler_task2);
	timer_start(&RTC_Scheduler);
}

/**
 * Example of using USART_EAST to write "Hello World" using the IO abstraction.
 *
 * Since the driver is asynchronous we need to use statically allocated memory for string
 * because driver initiates transfer and then returns before the transmission is completed.
 *
 * Once transfer has been completed the tx_cb function will be called.
 */

static uint8_t example_USART_EAST[12] = "Hello World!";

static void tx_cb_USART_EAST(const struct usart_async_descriptor *const io_descr)
{
	/* Transfer completed */
}

void USART_EAST_example(void)
{
	struct io_descriptor *io;

	usart_async_register_callback(&USART_EAST, USART_ASYNC_TXC_CB, tx_cb_USART_EAST);
	/*usart_async_register_callback(&USART_EAST, USART_ASYNC_RXC_CB, rx_cb);
	usart_async_register_callback(&USART_EAST, USART_ASYNC_ERROR_CB, err_cb);*/
	usart_async_get_io_descriptor(&USART_EAST, &io);
	usart_async_enable(&USART_EAST);

	io_write(io, example_USART_EAST, 12);
}

/**
 * Example of using USART_NORTH to write "Hello World" using the IO abstraction.
 *
 * Since the driver is asynchronous we need to use statically allocated memory for string
 * because driver initiates transfer and then returns before the transmission is completed.
 *
 * Once transfer has been completed the tx_cb function will be called.
 */

static uint8_t example_USART_NORTH[12] = "Hello World!";

static void tx_cb_USART_NORTH(const struct usart_async_descriptor *const io_descr)
{
	/* Transfer completed */
}

void USART_NORTH_example(void)
{
	struct io_descriptor *io;

	usart_async_register_callback(&USART_NORTH, USART_ASYNC_TXC_CB, tx_cb_USART_NORTH);
	/*usart_async_register_callback(&USART_NORTH, USART_ASYNC_RXC_CB, rx_cb);
	usart_async_register_callback(&USART_NORTH, USART_ASYNC_ERROR_CB, err_cb);*/
	usart_async_get_io_descriptor(&USART_NORTH, &io);
	usart_async_enable(&USART_NORTH);

	io_write(io, example_USART_NORTH, 12);
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

/**
 * Example of using UI_SPI to write "Hello World" using the IO abstraction.
 */
static uint8_t example_UI_SPI[12] = "Hello World!";

void UI_SPI_example(void)
{
	struct io_descriptor *io;
	spi_m_sync_get_io_descriptor(&UI_SPI, &io);

	spi_m_sync_enable(&UI_SPI);
	io_write(io, example_UI_SPI, 12);
}

/**
 * Example of using USART_WEST to write "Hello World" using the IO abstraction.
 *
 * Since the driver is asynchronous we need to use statically allocated memory for string
 * because driver initiates transfer and then returns before the transmission is completed.
 *
 * Once transfer has been completed the tx_cb function will be called.
 */

static uint8_t example_USART_WEST[12] = "Hello World!";

static void tx_cb_USART_WEST(const struct usart_async_descriptor *const io_descr)
{
	/* Transfer completed */
}

void USART_WEST_example(void)
{
	struct io_descriptor *io;

	usart_async_register_callback(&USART_WEST, USART_ASYNC_TXC_CB, tx_cb_USART_WEST);
	/*usart_async_register_callback(&USART_WEST, USART_ASYNC_RXC_CB, rx_cb);
	usart_async_register_callback(&USART_WEST, USART_ASYNC_ERROR_CB, err_cb);*/
	usart_async_get_io_descriptor(&USART_WEST, &io);
	usart_async_enable(&USART_WEST);

	io_write(io, example_USART_WEST, 12);
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
 * Example of using USART_SOUTH to write "Hello World" using the IO abstraction.
 *
 * Since the driver is asynchronous we need to use statically allocated memory for string
 * because driver initiates transfer and then returns before the transmission is completed.
 *
 * Once transfer has been completed the tx_cb function will be called.
 */

static uint8_t example_USART_SOUTH[12] = "Hello World!";

static void tx_cb_USART_SOUTH(const struct usart_async_descriptor *const io_descr)
{
	/* Transfer completed */
}

void USART_SOUTH_example(void)
{
	struct io_descriptor *io;

	usart_async_register_callback(&USART_SOUTH, USART_ASYNC_TXC_CB, tx_cb_USART_SOUTH);
	/*usart_async_register_callback(&USART_SOUTH, USART_ASYNC_RXC_CB, rx_cb);
	usart_async_register_callback(&USART_SOUTH, USART_ASYNC_ERROR_CB, err_cb);*/
	usart_async_get_io_descriptor(&USART_SOUTH, &io);
	usart_async_enable(&USART_SOUTH);

	io_write(io, example_USART_SOUTH, 12);
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

static struct timer_task TIMER_1_task1, TIMER_1_task2;

/**
 * Example of using TIMER_1.
 */
static void TIMER_1_task1_cb(const struct timer_task *const timer_task)
{
}

static void TIMER_1_task2_cb(const struct timer_task *const timer_task)
{
}

void TIMER_1_example(void)
{
	TIMER_1_task1.interval = 100;
	TIMER_1_task1.cb       = TIMER_1_task1_cb;
	TIMER_1_task1.mode     = TIMER_TASK_REPEAT;
	TIMER_1_task2.interval = 200;
	TIMER_1_task2.cb       = TIMER_1_task2_cb;
	TIMER_1_task2.mode     = TIMER_TASK_REPEAT;

	timer_add_task(&TIMER_1, &TIMER_1_task1);
	timer_add_task(&TIMER_1, &TIMER_1_task2);
	timer_start(&TIMER_1);
}

static struct timer_task TIMER_0_task1, TIMER_0_task2;

/**
 * Example of using TIMER_0.
 */
static void TIMER_0_task1_cb(const struct timer_task *const timer_task)
{
}

static void TIMER_0_task2_cb(const struct timer_task *const timer_task)
{
}

void TIMER_0_example(void)
{
	TIMER_0_task1.interval = 100;
	TIMER_0_task1.cb       = TIMER_0_task1_cb;
	TIMER_0_task1.mode     = TIMER_TASK_REPEAT;
	TIMER_0_task2.interval = 200;
	TIMER_0_task2.cb       = TIMER_0_task2_cb;
	TIMER_0_task2.mode     = TIMER_TASK_REPEAT;

	timer_add_task(&TIMER_0, &TIMER_0_task1);
	timer_add_task(&TIMER_0, &TIMER_0_task2);
	timer_start(&TIMER_0);
}

static struct timer_task TIMER_2_task1, TIMER_2_task2;

/**
 * Example of using TIMER_2.
 */
static void TIMER_2_task1_cb(const struct timer_task *const timer_task)
{
}

static void TIMER_2_task2_cb(const struct timer_task *const timer_task)
{
}

void TIMER_2_example(void)
{
	TIMER_2_task1.interval = 100;
	TIMER_2_task1.cb       = TIMER_2_task1_cb;
	TIMER_2_task1.mode     = TIMER_TASK_REPEAT;
	TIMER_2_task2.interval = 200;
	TIMER_2_task2.cb       = TIMER_2_task2_cb;
	TIMER_2_task2.mode     = TIMER_TASK_REPEAT;

	timer_add_task(&TIMER_2, &TIMER_2_task1);
	timer_add_task(&TIMER_2, &TIMER_2_task2);
	timer_start(&TIMER_2);
}

static struct timer_task TIMER_3_task1, TIMER_3_task2;

/**
 * Example of using TIMER_3.
 */
static void TIMER_3_task1_cb(const struct timer_task *const timer_task)
{
}

static void TIMER_3_task2_cb(const struct timer_task *const timer_task)
{
}

void TIMER_3_example(void)
{
	TIMER_3_task1.interval = 100;
	TIMER_3_task1.cb       = TIMER_3_task1_cb;
	TIMER_3_task1.mode     = TIMER_TASK_REPEAT;
	TIMER_3_task2.interval = 200;
	TIMER_3_task2.cb       = TIMER_3_task2_cb;
	TIMER_3_task2.mode     = TIMER_TASK_REPEAT;

	timer_add_task(&TIMER_3, &TIMER_3_task1);
	timer_add_task(&TIMER_3, &TIMER_3_task2);
	timer_start(&TIMER_3);
}
