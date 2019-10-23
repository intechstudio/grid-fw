/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file
 * to avoid losing it when reconfiguring.
 */
#ifndef DRIVER_INIT_INCLUDED
#define DRIVER_INIT_INCLUDED

#include "atmel_start_pins.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <hal_atomic.h>
#include <hal_delay.h>
#include <hal_gpio.h>
#include <hal_init.h>
#include <hal_io.h>
#include <hal_sleep.h>

#include <hal_adc_async.h>
#include <hal_adc_async.h>

#include <hal_crc_sync.h>

#include <hal_evsys.h>

#include <hal_flash.h>

#include <hal_timer.h>
#include <hal_usart_async.h>
#include <hal_usart_async.h>
#include <hal_usart_async.h>

#include <hal_spi_m_async.h>
#include <hal_usart_async.h>

#include <hal_i2c_m_async.h>
#include <hal_usart_async.h>

#include <hal_spi_m_dma.h>

#include <hal_delay.h>
#include <hal_timer.h>
#include <hpl_tc_base.h>
#include <hal_timer.h>
#include <hpl_tc_base.h>
#include <hal_timer.h>
#include <hpl_tc_base.h>
#include <hal_timer.h>
#include <hpl_tc_base.h>

#include "hal_usb_device.h"

extern struct adc_async_descriptor ADC_0;
extern struct adc_async_descriptor ADC_1;
extern struct crc_sync_descriptor  CRC_0;

extern struct flash_descriptor       FLASH_0;
extern struct timer_descriptor       RTC_Scheduler;
extern struct usart_async_descriptor USART_EAST;
extern struct usart_async_descriptor USART_NORTH;
extern struct usart_async_descriptor GRID_AUX;

extern struct spi_m_async_descriptor UI_SPI;
extern struct usart_async_descriptor USART_WEST;

extern struct i2c_m_async_desc       SYS_I2C;
extern struct usart_async_descriptor USART_SOUTH;

extern struct spi_m_dma_descriptor GRID_LED;

extern struct timer_descriptor TIMER_0;
extern struct timer_descriptor TIMER_1;
extern struct timer_descriptor TIMER_2;
extern struct timer_descriptor TIMER_3;

void ADC_0_init(void);

void ADC_1_init(void);

void FLASH_0_init(void);
void FLASH_0_CLOCK_init(void);

void USART_EAST_PORT_init(void);
void USART_EAST_CLOCK_init(void);
void USART_EAST_init(void);

void USART_NORTH_PORT_init(void);
void USART_NORTH_CLOCK_init(void);
void USART_NORTH_init(void);

void GRID_AUX_PORT_init(void);
void GRID_AUX_CLOCK_init(void);
void GRID_AUX_init(void);

void UI_SPI_PORT_init(void);
void UI_SPI_CLOCK_init(void);
void UI_SPI_init(void);

void USART_WEST_PORT_init(void);
void USART_WEST_CLOCK_init(void);
void USART_WEST_init(void);

void SYS_I2C_PORT_init(void);
void SYS_I2C_CLOCK_init(void);
void SYS_I2C_init(void);

void USART_SOUTH_PORT_init(void);
void USART_SOUTH_CLOCK_init(void);
void USART_SOUTH_init(void);

void GRID_LED_PORT_init(void);
void GRID_LED_CLOCK_init(void);
void GRID_LED_init(void);

void delay_driver_init(void);

void USB_DEVICE_INSTANCE_CLOCK_init(void);
void USB_DEVICE_INSTANCE_init(void);

/**
 * \brief Perform system initialization, initialize pins and clocks for
 * peripherals
 */
void system_init(void);

#ifdef __cplusplus
}
#endif
#endif // DRIVER_INIT_INCLUDED
