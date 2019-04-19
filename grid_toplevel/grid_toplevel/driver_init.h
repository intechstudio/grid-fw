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

#include <hal_usart_async.h>

#include <hal_i2c_m_async.h>

#include <hal_spi_m_dma.h>

#include <hal_delay.h>

extern struct adc_async_descriptor   ADC_0;
extern struct adc_async_descriptor   ADC_1;
extern struct usart_async_descriptor GRID_AUX;

extern struct i2c_m_async_desc SYS_I2C;

extern struct spi_m_dma_descriptor GRID_LED;

void ADC_0_init(void);

void ADC_1_init(void);

void GRID_AUX_PORT_init(void);
void GRID_AUX_CLOCK_init(void);
void GRID_AUX_init(void);

void SYS_I2C_PORT_init(void);
void SYS_I2C_CLOCK_init(void);
void SYS_I2C_init(void);

void GRID_LED_PORT_init(void);
void GRID_LED_CLOCK_init(void);
void GRID_LED_init(void);

void delay_driver_init(void);

/**
 * \brief Perform system initialization, initialize pins and clocks for
 * peripherals
 */
void system_init(void);

#ifdef __cplusplus
}
#endif
#endif // DRIVER_INIT_INCLUDED
