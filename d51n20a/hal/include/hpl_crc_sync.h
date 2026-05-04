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

#ifndef HPL_CRC_SYNC_H_INCLUDED
#define HPL_CRC_SYNC_H_INCLUDED

#include <compiler.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup hpl__crc__sync CRC Sync Driver
 *
 * \section crc_rev Revision History
 * - v0.0.0.1 Initial Commit
 *
 *@{
 */

/**
 * \brief CRC Device
 */
struct _crc_sync_device {
	void *hw; /*!< Hardware module instance handler */
};

/**
 * \brief Initialize CRC.
 *
 * \param[in] device The pointer to device instance
 * \param[in] hw The pointer to hardware instance
 *
 * \return Initialization status.
 * \retval -1 Passed parameters were invalid or the CRC is already initialized
 * \retval 0 The initialization is completed successfully
 */
int32_t _crc_sync_init(struct _crc_sync_device *const device, void *const hw);

/**
 * \brief Deinitialize CRC.
 *
 * \param[in] device The pointer to device instance
 *
 * \return De-initialization status.
 * \retval -1 Passed parameters were invalid or the CRC is not initialized
 * \retval 0 The de-initialization is completed successfully
 */
int32_t _crc_sync_deinit(struct _crc_sync_device *const device);

/**
 * \brief Enable CRC
 *
 * \param[in] device The pointer to device instance
 *
 * \return Enabling status.
 * \retval -1 Passed parameters were invalid
 * \retval 0 The enable procedure is completed successfully
 */
int32_t _crc_sync_enable(struct _crc_sync_device *const device);

/**
 * \brief Disable CRC
 *
 * \param[in] device The pointer to device instance
 *
 * \return Disabling status.
 * \retval -1 Passed parameters were invalid
 * \retval 0 The disable procedure is completed successfully
 */
int32_t _crc_sync_disable(struct _crc_sync_device *const device);

/**
 * \brief Calculate CRC value of the buffer
 *
 * \param[in] device The pointer to device instance
 * \param[in] data Pointer to the input data buffer
 * \param[in] len Length of the input data buffer
 * \param[in,out] pcrc Pointer to the CRC value
 *
 * \return Calculate result.
 * \retval -1 Passed parameters were invalid or hardware error occurs.
 * \retval 0 The CRC calculate success.
 */
int32_t _crc_sync_crc32(struct _crc_sync_device *const device, uint32_t *const data, const uint32_t len,
                        uint32_t *pcrc);

/**
 * \brief Compute CRC16 value of the buffer
 *
 * This function calculate CRC-16 (CCITT)
 *
 * \param[in] device The pointer to device instance
 * \param[in] data Pointer to the input data buffer
 * \param[in] len Length of the input data buffer
 * \param[in,out] pcrc Pointer to the CRC value
 *
 * \return Calculate result.
 * \retval -1 Passed parameters were invalid or hardware error occurs.
 * \retval 0 The CRC calculate success.
 */
int32_t _crc_sync_crc16(struct _crc_sync_device *const device, uint16_t *const data, const uint32_t len,
                        uint16_t *pcrc);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* HPL_CRC_SYNC_H_INCLUDED */
