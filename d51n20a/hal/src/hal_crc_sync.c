/**
 * \file
 *
 * \brief CRC Cyclic Redundancy Check(Sync) functionality declaration.
 *
 * Copyright (c) 2015-2018 Microchip Technology Inc. and its subsidiaries.
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

#include <hal_crc_sync.h>

#define DRIVER_VERSION 0x00000001u

/**
 * \brief Initialize CRC.
 */
int32_t crc_sync_init(struct crc_sync_descriptor *const descr, void *const hw)
{
	ASSERT(descr && hw);

	return _crc_sync_init(&descr->dev, hw);
}

/**
 * \brief Deinitialize CRC.
 */
int32_t crc_sync_deinit(struct crc_sync_descriptor *const descr)
{
	ASSERT(descr);

	return _crc_sync_deinit(&descr->dev);
}

/**
 * \brief Enable CRC
 */
int32_t crc_sync_enable(struct crc_sync_descriptor *const descr)
{
	ASSERT(descr);

	return _crc_sync_enable(&descr->dev);
}

/**
 * \brief Disable CRC
 */
int32_t crc_sync_disable(struct crc_sync_descriptor *const descr)
{
	ASSERT(descr);

	return _crc_sync_disable(&descr->dev);
}
/**
 * \brief Calculate CRC32 value of the buffer
 */
int32_t crc_sync_crc32(struct crc_sync_descriptor *const descr, uint32_t *const data, const uint32_t len,
                       uint32_t *pcrc)
{
	ASSERT(descr && data && len && pcrc);

	return _crc_sync_crc32(&descr->dev, data, len, pcrc);
}

/**
 * \brief Calculate CRC16 value of the buffer
 */
int32_t crc_sync_crc16(struct crc_sync_descriptor *const descr, uint16_t *const data, const uint32_t len,
                       uint16_t *pcrc)
{
	ASSERT(descr && data && len && pcrc);

	return _crc_sync_crc16(&descr->dev, data, len, pcrc);
}

/**
 * \brief Retrieve the current driver version
 */
uint32_t crc_sync_get_version(void)
{
	return DRIVER_VERSION;
}
