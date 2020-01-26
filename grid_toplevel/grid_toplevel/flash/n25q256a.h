/**
 * \file
 *
 * \brief N25Q256A component declaration
 *
 * Copyright (c) 2018 Microchip Technology Inc. and its subsidiaries.
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
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */

#ifndef _N25Q256A_H_INCLUDED
#define _N25Q256A_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "spi_nor_flash.h"

typedef void (*func)(void);

/**
 * \brief N25Q256A spi nor flash
 */
struct n25q256a {
	/** The parent abstract spi nor flash */
	struct spi_nor_flash parent;
	/** Pin exit xip function pointer*/
	func pin_exit_xip;
	/** Quad mode used by SPI read/write data */
	uint8_t quad_mode;
	/** XIP mode used */
	uint8_t xip_mode;
};

/**
 * \brief Construct n25q256a spi nor flash
 *
 * \param[in] me        Pointer to the abstract spi nor flash structure
 * \param[in] io        Pointer to the instance of controller
 * \param[in] quad_mode 0 single-bit SPI data, 1 quad SPI data
 *
 * \return pointer to initialized flash
 */
struct spi_nor_flash *n25q256a_construct(struct spi_nor_flash *const me, void *const io, func pin_exit_xip,
                                         const uint8_t quad_mode);

/**
 * \brief Read bytes from the spi nor Flash
 *
 * \param[in]  me       Pointer to the abstract spi nor flash structure
 * \param[out] buf      Pointer to the buffer
 * \param[in]  address  Source bytes address to read from flash
 * \param[in]  length   Number of bytes to read
 * \return status.
 */
int32_t n25q256a_read(const struct spi_nor_flash *const me, uint8_t *buf, uint32_t address, uint32_t length);

/**
 * \brief Write bytes to the spi nor Flash
 *
 * \param[in] me        Pointer to the abstract spi nor flash structure
 * \param[in] buf       Pointer to the buffer
 * \param[in] address   Destination bytes address to write into flash
 * \param[in] length    Number of bytes to write
 * \return status.
 */
int32_t n25q256a_write(const struct spi_nor_flash *const me, uint8_t *buf, uint32_t address, uint32_t length);

/**
 * \brief Erase sectors/blocks in the spi nor Flash
 * \param[in] me        Pointer to the abstract spi nor flash structure
 * \param[in] address   Destination bytes address aligned with sector/block start
 * \param[in] length    Number of bytes to be erase
 * \return status.
 *
 * \note length must be multiple of sector size
 */
int32_t n25q256a_erase(const struct spi_nor_flash *const me, uint32_t address, uint32_t length);

/**
 * \brief Enable the XIP mode (continous read)
 *
 * \param[in] me        Pointer to the abstract spi nor flash structure
 * \return status.
 */
int32_t n25q256a_enable_xip(const struct spi_nor_flash *const me);

/**
 * \brief Disable the XIP mode (continous read)
 *
 * \param[in] me        Pointer to the abstract spi nor flash structure
 * \return status.
 */
int32_t n25q256a_disable_xip(const struct spi_nor_flash *const me);

#ifdef __cplusplus
}
#endif

#endif /* _N25Q256A_H_INCLUDED */
