/**
 * \file
 *
 * \brief Device Service Unit
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

#include <compiler.h>
#include <hpl_pac.h>
#include <hpl_crc_sync.h>
#include <utils_assert.h>
#include <err_codes.h>

/**
 * \brief Initialize CRC.
 */
int32_t _crc_sync_init(struct _crc_sync_device *const device, void *const hw)
{
	device->hw = hw;

	return ERR_NONE;
}

/**
 * \brief De-initialize CRC.
 */
int32_t _crc_sync_deinit(struct _crc_sync_device *const device)
{
	device->hw = NULL;

	return ERR_NONE;
}

/**
 * \brief Enable CRC
 */
int32_t _crc_sync_enable(struct _crc_sync_device *const device)
{
	(void)device;

	return ERR_NONE;
}

/**
 * \brief Disable CRC
 */
int32_t _crc_sync_disable(struct _crc_sync_device *const device)
{
	(void)device;

	return ERR_NONE;
}

/**
 * \brief Calculate CRC value of the buffer
 */
int32_t _crc_sync_crc32(struct _crc_sync_device *const device, uint32_t *const data, const uint32_t len, uint32_t *pcrc)
{
	int32_t rc = ERR_NONE;
	if (((uint32_t)data) & 0x00000003) {
		/* Address must be align with 4 bytes, refer to datasheet */
		return ERR_INVALID_ARG;
	}

	CRITICAL_SECTION_ENTER()
	/* Disable write-protected by PAC1->DSU before write DSU registers */
	_periph_unlock(device->hw);

	hri_dsu_write_ADDR_reg(device->hw, (uint32_t)data);
	hri_dsu_write_LENGTH_LENGTH_bf(device->hw, len);
	hri_dsu_write_DATA_reg(device->hw, *pcrc);
	hri_dsu_write_CTRL_reg(device->hw, DSU_CTRL_CRC);

	while (hri_dsu_get_STATUSA_DONE_bit(device->hw) == 0) {
	}

	if (hri_dsu_get_STATUSA_BERR_bit(device->hw)) {
		hri_dsu_clear_STATUSA_BERR_bit(device->hw);
		hri_dsu_clear_STATUSA_DONE_bit(device->hw);
		rc = ERR_IO;
	} else {
		*pcrc = (uint32_t)hri_dsu_read_DATA_reg(device->hw);
	}
	hri_dsu_clear_STATUSA_DONE_bit(device->hw);

	/* Restore write-protected of PAC->DSU */
	_periph_lock(device->hw);

	CRITICAL_SECTION_LEAVE()

	return rc;
}

/**
 * \brief Compute CRC16 value of the buffer
 */
int32_t _crc_sync_crc16(struct _crc_sync_device *const device, uint16_t *const data, const uint32_t len, uint16_t *pcrc)
{
	(void)device, (void)data, (void)len, (void)pcrc;
	return ERR_UNSUPPORTED_OP;
}
