/**
 * \file
 *
 * \brief HAL event system related functionality declaration.
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

#include <hpl_evsys.h>

#ifndef _HAL_EVSYS_H_INCLUDED
#define _HAL_EVSYS_H_INCLUDED

#include <compiler.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup doc_driver_hal_evsys
 *
 * @{
 */

/**
 * \brief Initialize event system.
 *
 * \return Initialization status.
 */
int32_t event_system_init(void);

/**
 * \brief Deinitialize event system.
 *
 * \return De-initialization status.
 */
int32_t event_system_deinit(void);

/**
 * \brief Enable event reception by the given user from the given channel
 *
 * \param[in] user A user to enable
 * \param[in] channel A channel the user is assigned to
 *
 * \return Status of operation.
 */
int32_t event_system_enable_user(const uint16_t user, const uint16_t channel);

/**
 * \brief Disable event reception by the given user from the given channel
 *
 * \param[in] user A user to disable
 * \param[in] channel A channel the user is assigned to
 *
 * \return Status of operation.
 */
int32_t event_system_disable_user(const uint16_t user, const uint16_t channel);

/**
 * \brief Enable event generation by the given generator for the given channel
 *
 * \param[in] generator A generator to disable
 * \param[in] channel A channel the generator is assigned to
 *
 * \return Status of operation.
 */
int32_t event_system_enable_generator(const uint16_t generator, const uint16_t channel);

/**
 * \brief Disable event generation by the given generator for the given channel
 *
 * \param[in] generator A generator to disable
 * \param[in] channel A channel the generator is assigned to
 *
 * \return Status of operation.
 */
int32_t event_system_disable_generator(const uint16_t generator, const uint16_t channel);

/**
 * \brief Retrieve the current driver version
 *
 * \return Current driver version.
 */
uint32_t event_system_get_version(void);

/**@}*/
#ifdef __cplusplus
}
#endif
#endif /* _HAL_EVSYS_H_INCLUDED */
