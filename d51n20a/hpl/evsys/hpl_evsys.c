
/**
 * \file
 *
 * \brief EVSYS related functionality implementation.
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

#include <hpl_evsys_config.h>
#include <hpl_init.h>
#include <utils_repeat_macro.h>
#include <err_codes.h>

/* Stub macros for reserved channel */
#define CONF_CHANNEL_38 0
#define CONF_ONDEMAND_38 0
#define CONF_RUNSTDBY_38 0
#define CONF_EDGSEL_38 0
#define CONF_PATH_38 0
#define CONF_EVGEN_38 0

#define CONF_CHANNEL_39 0
#define CONF_ONDEMAND_39 0
#define CONF_RUNSTDBY_39 0
#define CONF_EDGSEL_39 0
#define CONF_PATH_39 0
#define CONF_EVGEN_39 0

#define CONF_CHANNEL_113 0
#define CONF_ONDEMAND_113 0
#define CONF_RUNSTDBY_113 0
#define CONF_EDGSEL_113 0
#define CONF_PATH_113 0
#define CONF_EVGEN_113 0

/* Event user configuration */
#define USER_MUX_CONF(i, n) EVSYS_USER_CHANNEL(CONF_CHANNEL_##n),

/* This macro is used for repeat macro: i - unused, n - amount of channels.
 * It contains channel configuration. */
#define CHANNEL_CONF(i, n)                                                                                             \
	(CONF_ONDEMAND_##n << EVSYS_CHANNEL_ONDEMAND_Pos) | (CONF_RUNSTDBY_##n << EVSYS_CHANNEL_RUNSTDBY_Pos)              \
	    | EVSYS_CHANNEL_EDGSEL(CONF_EDGSEL_##n) | EVSYS_CHANNEL_PATH(CONF_PATH_##n)                                    \
	    | EVSYS_CHANNEL_EVGEN(CONF_EVGEN_##n),

/* This macro is used for repeat macro: i - unused, n - amount of channels
 * It contains interrupts configuration. */
#define INT_CFG(i, n) (CONF_OVR_##n << EVSYS_CHINTFLAG_OVR_Pos) | (CONF_EVD_##n << EVSYS_CHINTFLAG_EVD_Pos),

static const uint8_t user_mux_confs[] = {REPEAT_MACRO(USER_MUX_CONF, i, EVSYS_USERS)};

static const uint16_t channel_confs[] = {REPEAT_MACRO(CHANNEL_CONF, i, EVSYS_CHANNELS)};

static const uint32_t interrupt_cfg[] = {REPEAT_MACRO(INT_CFG, i, EVSYS_CHANNELS)};

/**
 * \brief Initialize event system
 */
int32_t _event_system_init(void)
{
	uint8_t i;
	/* configure user multiplexers */
	for (i = 0; i < EVSYS_USERS; i++) {
		hri_evsys_write_USER_reg(EVSYS, i, user_mux_confs[i]);
	}

	/* configure channels */
	for (i = 0; i < EVSYS_CHANNELS; i++) {
		hri_evsys_write_CHANNEL_reg(EVSYS, i, channel_confs[i]);
		hri_evsys_write_CHINTEN_reg(EVSYS, i, interrupt_cfg[i]);
	}

	return ERR_NONE;
}

/**
 * \brief De-initialize event system.
 */
int32_t _event_system_deinit()
{
	hri_evsys_write_CTRLA_reg(EVSYS, EVSYS_CTRLA_SWRST);

	return ERR_NONE;
}

/**
 * \brief Enable/disable event reception by the given user from the given
 *        channel
 */
int32_t _event_system_enable_user(const uint16_t user, const uint16_t channel, const bool on)
{
	if (on) {
		hri_evsys_write_USER_reg(EVSYS, user, channel);
	} else {
		hri_evsys_write_USER_reg(EVSYS, user, 0);
	}

	return ERR_NONE;
}

/**
 * \brief Enable/disable event generation by the given generator for the given
 *        channel
 */
int32_t _event_system_enable_generator(const uint16_t generator, const uint16_t channel, const bool on)
{
	if (on) {
		hri_evsys_write_CHANNEL_EVGEN_bf(EVSYS, channel, generator);
	} else {
		hri_evsys_write_CHANNEL_EVGEN_bf(EVSYS, channel, 0);
	}

	return ERR_NONE;
}
