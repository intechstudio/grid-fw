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

#ifndef HAL_CRC_SYNC_H_INCLUDED
#define HAL_CRC_SYNC_H_INCLUDED

#include <hpl_crc_sync.h>
#include <utils_assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup doc_driver_hal_crc_sync
 *
 *@{
 */

/**
 * \brief CRC descriptor
 */
struct crc_sync_descriptor {
	struct _crc_sync_device dev; /*!< CRC HPL device descriptor */
};

/**
 * \brief Initialize CRC.
 *
 * This function initializes the given CRC descriptor.
 * It checks if the given hardware is not initialized and if the given hardware
 * is permitted to be initialized.
 *
 * \param[out] descr A CRC descriptor to initialize
 * \param[in] hw The pointer to hardware instance
 *
 * \return Initialization status.
 */
int32_t crc_sync_init(struct crc_sync_descriptor *const descr, void *const hw);

/**
 * \brief Deinitialize CRC.
 *
 * This function deinitializes the given CRC descriptor.
 * It checks if the given hardware is initialized and if the given hardware is
 * permitted to be deinitialized.
 *
 * \param[in] descr A CRC descriptor to deinitialize
 *
 * \return De-initialization status.
 */
int32_t crc_sync_deinit(struct crc_sync_descriptor *const descr);

/**
 * \brief Enable CRC
 *
 * This function enables CRC by the given CRC descriptor.
 *
 * \param[in] descr A CRC descriptor to enable
 *
 * \return Enabling status.
 */
int32_t crc_sync_enable(struct crc_sync_descriptor *const descr);

/**
 * \brief Disable CRC
 *
 * This function disables CRC by the given CRC descriptor.
 *
 * \param[in] descr A CRC descriptor to disable
 *
 * \return Disabling status.
 */
int32_t crc_sync_disable(struct crc_sync_descriptor *const descr);

/**
 * \brief Calculate CRC32 value of the buffer
 *
 * This function calculates the standard CRC-32 (IEEE 802.3).
 *
 * \param[in] data Pointer to the input data buffer
 * \param[in] len Length of the input data buffer
 * \param[in,out] pcrc Pointer to the CRC value
 *
 * \return Calculated result.
 */
int32_t crc_sync_crc32(struct crc_sync_descriptor *const descr, uint32_t *const data, const uint32_t len,
                       uint32_t *pcrc);

/**
 * \brief Calculate the CRC16 value of the buffer
 *
 * This function calculates CRC-16 (CCITT).
 *
 * \param[in] data Pointer to the input data buffer
 * \param[in] len Length of the input data buffer
 * \param[in,out] pcrc Pointer to the CRC value
 *
 * \return Calculated result.
 */
int32_t crc_sync_crc16(struct crc_sync_descriptor *const descr, uint16_t *const data, const uint32_t len,
                       uint16_t *pcrc);

/**
 * \brief Retrieve the current driver version
 *
 * \return Current driver version.
 */
uint32_t crc_sync_get_version(void);
/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* HAL_CRC_SYNC_H_INCLUDED */
