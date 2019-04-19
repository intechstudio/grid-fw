/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file
 * to avoid losing it when reconfiguring.
 */

#include "driver_init.h"
#include <peripheral_clk_config.h>
#include <utils.h>
#include <hal_init.h>

#include <hpl_adc_base.h>
#include <hpl_adc_base.h>

/* The channel amount for ADC */
#define ADC_0_CH_AMOUNT 1

/* The buffer size for ADC */
#define ADC_0_BUFFER_SIZE 16

/* The maximal channel number of enabled channels */
#define ADC_0_CH_MAX 0

/* The channel amount for ADC */
#define ADC_1_CH_AMOUNT 1

/* The buffer size for ADC */
#define ADC_1_BUFFER_SIZE 16

/* The maximal channel number of enabled channels */
#define ADC_1_CH_MAX 0

/*! The buffer size for USART */
#define GRID_AUX_BUFFER_SIZE 16

struct adc_async_descriptor         ADC_0;
struct adc_async_channel_descriptor ADC_0_ch[ADC_0_CH_AMOUNT];
struct adc_async_descriptor         ADC_1;
struct adc_async_channel_descriptor ADC_1_ch[ADC_1_CH_AMOUNT];
struct usart_async_descriptor       GRID_AUX;

static uint8_t ADC_0_buffer[ADC_0_BUFFER_SIZE];
static uint8_t ADC_0_map[ADC_0_CH_MAX + 1];
static uint8_t ADC_1_buffer[ADC_1_BUFFER_SIZE];
static uint8_t ADC_1_map[ADC_1_CH_MAX + 1];
static uint8_t GRID_AUX_buffer[GRID_AUX_BUFFER_SIZE];

struct i2c_m_async_desc SYS_I2C;

struct spi_m_dma_descriptor GRID_LED;

/**
 * \brief ADC initialization function
 *
 * Enables ADC peripheral, clocks and initializes ADC driver
 */
void ADC_0_init(void)
{
	hri_mclk_set_APBDMASK_ADC0_bit(MCLK);
	hri_gclk_write_PCHCTRL_reg(GCLK, ADC0_GCLK_ID, CONF_GCLK_ADC0_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));
	adc_async_init(&ADC_0, ADC0, ADC_0_map, ADC_0_CH_MAX, ADC_0_CH_AMOUNT, &ADC_0_ch[0], (void *)NULL);
	adc_async_register_channel_buffer(&ADC_0, 0, ADC_0_buffer, ADC_0_BUFFER_SIZE);

	// Disable digital pin circuitry
	gpio_set_pin_direction(PA07, GPIO_DIRECTION_OFF);

	gpio_set_pin_function(PA07, PINMUX_PA07B_ADC0_AIN7);
}

/**
 * \brief ADC initialization function
 *
 * Enables ADC peripheral, clocks and initializes ADC driver
 */
void ADC_1_init(void)
{
	hri_mclk_set_APBDMASK_ADC1_bit(MCLK);
	hri_gclk_write_PCHCTRL_reg(GCLK, ADC1_GCLK_ID, CONF_GCLK_ADC1_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));
	adc_async_init(&ADC_1, ADC1, ADC_1_map, ADC_1_CH_MAX, ADC_1_CH_AMOUNT, &ADC_1_ch[0], (void *)NULL);
	adc_async_register_channel_buffer(&ADC_1, 0, ADC_1_buffer, ADC_1_BUFFER_SIZE);

	// Disable digital pin circuitry
	gpio_set_pin_direction(PC02, GPIO_DIRECTION_OFF);

	gpio_set_pin_function(PC02, PINMUX_PC02B_ADC1_AIN4);
}

/**
 * \brief USART Clock initialization function
 *
 * Enables register interface and peripheral clock
 */
void GRID_AUX_CLOCK_init()
{

	hri_gclk_write_PCHCTRL_reg(GCLK, SERCOM2_GCLK_ID_CORE, CONF_GCLK_SERCOM2_CORE_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));
	hri_gclk_write_PCHCTRL_reg(GCLK, SERCOM2_GCLK_ID_SLOW, CONF_GCLK_SERCOM2_SLOW_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));

	hri_mclk_set_APBBMASK_SERCOM2_bit(MCLK);
}

/**
 * \brief USART pinmux initialization function
 *
 * Set each required pin to USART functionality
 */
void GRID_AUX_PORT_init()
{

	gpio_set_pin_function(PB25, PINMUX_PB25D_SERCOM2_PAD0);

	gpio_set_pin_function(PB24, PINMUX_PB24D_SERCOM2_PAD1);
}

/**
 * \brief USART initialization function
 *
 * Enables USART peripheral, clocks and initializes USART driver
 */
void GRID_AUX_init(void)
{
	GRID_AUX_CLOCK_init();
	usart_async_init(&GRID_AUX, SERCOM2, GRID_AUX_buffer, GRID_AUX_BUFFER_SIZE, (void *)NULL);
	GRID_AUX_PORT_init();
}

void SYS_I2C_PORT_init(void)
{

	gpio_set_pin_pull_mode(PA23,
	                       // <y> Pull configuration
	                       // <id> pad_pull_config
	                       // <GPIO_PULL_OFF"> Off
	                       // <GPIO_PULL_UP"> Pull-up
	                       // <GPIO_PULL_DOWN"> Pull-down
	                       GPIO_PULL_OFF);

	gpio_set_pin_function(PA23, PINMUX_PA23D_SERCOM5_PAD0);

	gpio_set_pin_pull_mode(PA22,
	                       // <y> Pull configuration
	                       // <id> pad_pull_config
	                       // <GPIO_PULL_OFF"> Off
	                       // <GPIO_PULL_UP"> Pull-up
	                       // <GPIO_PULL_DOWN"> Pull-down
	                       GPIO_PULL_OFF);

	gpio_set_pin_function(PA22, PINMUX_PA22D_SERCOM5_PAD1);
}

void SYS_I2C_CLOCK_init(void)
{
	hri_gclk_write_PCHCTRL_reg(GCLK, SERCOM5_GCLK_ID_CORE, CONF_GCLK_SERCOM5_CORE_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));
	hri_gclk_write_PCHCTRL_reg(GCLK, SERCOM5_GCLK_ID_SLOW, CONF_GCLK_SERCOM5_SLOW_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));

	hri_mclk_set_APBDMASK_SERCOM5_bit(MCLK);
}

void SYS_I2C_init(void)
{
	SYS_I2C_CLOCK_init();
	i2c_m_async_init(&SYS_I2C, SERCOM5);
	SYS_I2C_PORT_init();
}

void GRID_LED_PORT_init(void)
{

	gpio_set_pin_level(PB21,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	// Set pin direction to output
	gpio_set_pin_direction(PB21, GPIO_DIRECTION_OUT);

	gpio_set_pin_function(PB21, PINMUX_PB21D_SERCOM7_PAD0);

	gpio_set_pin_level(PB20,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	// Set pin direction to output
	gpio_set_pin_direction(PB20, GPIO_DIRECTION_OUT);

	gpio_set_pin_function(PB20, PINMUX_PB20D_SERCOM7_PAD1);

	// Set pin direction to input
	gpio_set_pin_direction(PB18, GPIO_DIRECTION_IN);

	gpio_set_pin_pull_mode(PB18,
	                       // <y> Pull configuration
	                       // <id> pad_pull_config
	                       // <GPIO_PULL_OFF"> Off
	                       // <GPIO_PULL_UP"> Pull-up
	                       // <GPIO_PULL_DOWN"> Pull-down
	                       GPIO_PULL_OFF);

	gpio_set_pin_function(PB18, PINMUX_PB18D_SERCOM7_PAD2);
}

void GRID_LED_CLOCK_init(void)
{
	hri_gclk_write_PCHCTRL_reg(GCLK, SERCOM7_GCLK_ID_CORE, CONF_GCLK_SERCOM7_CORE_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));
	hri_gclk_write_PCHCTRL_reg(GCLK, SERCOM7_GCLK_ID_SLOW, CONF_GCLK_SERCOM7_SLOW_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));

	hri_mclk_set_APBDMASK_SERCOM7_bit(MCLK);
}

void GRID_LED_init(void)
{
	GRID_LED_CLOCK_init();
	spi_m_dma_init(&GRID_LED, SERCOM7);
	GRID_LED_PORT_init();
}

void delay_driver_init(void)
{
	delay_init(SysTick);
}

void system_init(void)
{
	init_mcu();

	// GPIO on PA05

	gpio_set_pin_level(LED0,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	// Set pin direction to output
	gpio_set_pin_direction(LED0, GPIO_DIRECTION_OUT);

	gpio_set_pin_function(LED0, GPIO_PIN_FUNCTION_OFF);

	// GPIO on PC14

	gpio_set_pin_level(UI_PWR_EN,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	// Set pin direction to output
	gpio_set_pin_direction(UI_PWR_EN, GPIO_DIRECTION_OUT);

	gpio_set_pin_function(UI_PWR_EN, GPIO_PIN_FUNCTION_OFF);

	// GPIO on PC19

	gpio_set_pin_level(MUX_A,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	// Set pin direction to output
	gpio_set_pin_direction(MUX_A, GPIO_DIRECTION_OUT);

	gpio_set_pin_function(MUX_A, GPIO_PIN_FUNCTION_OFF);

	// GPIO on PC20

	gpio_set_pin_level(MUX_B,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	// Set pin direction to output
	gpio_set_pin_direction(MUX_B, GPIO_DIRECTION_OUT);

	gpio_set_pin_function(MUX_B, GPIO_PIN_FUNCTION_OFF);

	// GPIO on PC21

	gpio_set_pin_level(MUX_C,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	// Set pin direction to output
	gpio_set_pin_direction(MUX_C, GPIO_DIRECTION_OUT);

	gpio_set_pin_function(MUX_C, GPIO_PIN_FUNCTION_OFF);

	ADC_0_init();
	ADC_1_init();

	GRID_AUX_init();

	SYS_I2C_init();

	GRID_LED_init();

	delay_driver_init();
}
