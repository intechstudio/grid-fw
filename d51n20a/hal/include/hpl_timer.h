/**
 * \file
 *
 * \brief Timer related functionality declaration.
 *
 * Copyright (c) 2014-2018 Microchip Technology Inc. and its subsidiaries.
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

#ifndef _HPL_TIMER_H_INCLUDED
#define _HPL_TIMER_H_INCLUDED

/**
 * \addtogroup HPL Timer
 *
 * \section hpl_timer_rev Revision History
 * - v1.0.0 Initial Release
 *
 *@{
 */

#include <compiler.h>
#include <hpl_irq.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Timer device structure
 *
 * The Timer device structure forward declaration.
 */
struct _timer_device;

/**
 * \brief Timer interrupt callbacks
 */
struct _timer_callbacks {
	void (*period_expired)(struct _timer_device *device);
};

/**
 * \brief Timer device structure
 */
struct _timer_device {
	struct _timer_callbacks timer_cb;
	struct _irq_descriptor  irq;
	void *                  hw;
};

/**
 * \brief Timer functions, pointers to low-level functions
 */
struct _timer_hpl_interface {
	int32_t (*init)(struct _timer_device *const device, void *const hw);
	void (*deinit)(struct _timer_device *const device);
	void (*start_timer)(struct _timer_device *const device);
	void (*stop_timer)(struct _timer_device *const device);
	void (*set_timer_period)(struct _timer_device *const device, const uint32_t clock_cycles);
	uint32_t (*get_period)(const struct _timer_device *const device);
	bool (*is_timer_started)(const struct _timer_device *const device);
	void (*set_timer_irq)(struct _timer_device *const device);
};

#ifdef __cplusplus
}
#endif
/**@}*/
#endif /* _HPL_TIMER_H_INCLUDED */
