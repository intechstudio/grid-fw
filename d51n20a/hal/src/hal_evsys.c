/**
 * \file
 *
 * \brief HAL event system related functionality implementation.
 *
 * Copyright (c) 2016-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */

#include "hal_evsys.h"
#include <hpl_evsys.h>

/**
 * \brief Driver version
 */
#define DRIVER_VERSION 0x00000001u

/**
 * \brief Initialize event system.
 */
int32_t event_system_init(void)
{
	return _event_system_init();
}

/**
 * \brief Deinitialize event system.
 */
int32_t event_system_deinit(void)
{
	return _event_system_deinit();
}

/**
 * \brief Enable event reception by the given user from the given channel
 */
int32_t event_system_enable_user(const uint16_t user, const uint16_t channel)
{
	return _event_system_enable_user(user, channel, true);
}

/**
 * \brief Enable event reception by the given user from the given channel
 */
int32_t event_system_disable_user(const uint16_t user, const uint16_t channel)
{
	return _event_system_enable_user(user, channel, false);
}

/**
 * \brief Enable event generation by the given generator for the given channel
 */
int32_t event_system_enable_generator(const uint16_t generator, const uint16_t channel)
{
	return _event_system_enable_generator(generator, channel, true);
}

/**
 * \brief Enable event generation by the given generator for the given channel
 */
int32_t event_system_disable_generator(const uint16_t generator, const uint16_t channel)
{
	return _event_system_enable_generator(generator, channel, false);
}

/**
 * \brief Retrieve the current driver version
 *
 * \return Current driver version
 */
uint32_t event_system_get_version(void)
{
	return DRIVER_VERSION;
}
