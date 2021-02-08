/**
 * \file
 *
 * \brief SPI NOR Flash declaration.
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

#ifndef _SPI_NOR_FLASH_H_INCLUDED
#define _SPI_NOR_FLASH_H_INCLUDED

#include <compiler.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Forward declaration of spi nor flash structure. */
struct spi_nor_flash;

/**
 * \brief Interface of abstract spi nor flash
 */
struct spi_nor_flash_interface {
	int32_t (*read)(const struct spi_nor_flash *const me, uint8_t *buf, uint32_t address, uint32_t length);
	int32_t (*write)(const struct spi_nor_flash *const me, uint8_t *buf, uint32_t address, uint32_t length);
	int32_t (*erase)(const struct spi_nor_flash *const me, uint32_t address, uint32_t length);
	int32_t (*enxip)(const struct spi_nor_flash *const me);
	int32_t (*disxip)(const struct spi_nor_flash *const me);
};

/**
 * \brief Abstract spi nor flash structure
 */
struct spi_nor_flash {
	/** The pointer to interface used to communicate with spi nor flash */
	void *io;
	/** The interface of abstract spi nor flash */
	const struct spi_nor_flash_interface *interface;
};

/**
 * \brief Construct abstract spi nor flash
 *
 * \param[in] me Pointer to the abstract spi nor flash structure
 * \param[in] io Pointer to the instance of controller
 * \param[in] interface Pointer to interface of
 *
 * \return pointer to initialized sensor
 */
struct spi_nor_flash *spi_nor_flash_construct(struct spi_nor_flash *const me, void *const io,
                                              const struct spi_nor_flash_interface *const interface);

/**
 * \brief Read bytes from the spi nor Flash
 *
 * \param[in]  me       Pointer to the abstract spi nor flash structure
 * \param[out] buf      Pointer to the buffer
 * \param[in]  address  Source bytes address to read from flash
 * \param[in]  length   Number of bytes to read
 * \return status.
 */
int32_t spi_nor_flash_read(const struct spi_nor_flash *const me, uint8_t *buf, uint32_t address, uint32_t length);

/**
 * \brief Write bytes to the spi nor Flash
 *
 * \param[in] me        Pointer to the abstract spi nor flash structure
 * \param[in] buf       Pointer to the buffer
 * \param[in] address   Destination bytes address to write into flash
 * \param[in] length    Number of bytes to write
 * \return status.
 */
int32_t spi_nor_flash_write(const struct spi_nor_flash *const me, uint8_t *buf, uint32_t address, uint32_t length);

/**
 * \brief Erase sectors/blocks in the spi nor Flash
 * \param[in] me        Pointer to the abstract spi nor flash structure
 * \param[in] address   Destination bytes address aligned with sector/block start
 * \param[in] length    Number of bytes to be erase
 * \return status.
 *
 * \note length must be multiple of sector size
 */
int32_t spi_nor_flash_erase(const struct spi_nor_flash *const me, uint32_t address, uint32_t length);

/**
 * \brief Enable the XIP mode (continous read)
 *
 * \param[in] me        Pointer to the abstract spi nor flash structure
 * \return status.
 */
int32_t spi_nor_flash_enable_xip(const struct spi_nor_flash *const me);

/**
 * \brief Disable the XIP mode (continous read)
 *
 * \param[in] me        Pointer to the abstract spi nor flash structure
 * \return status.
 */
int32_t spi_nor_flash_disable_xip(const struct spi_nor_flash *const me);

#ifdef __cplusplus
}
#endif

#endif /* _SPI_NOR_FLASH_H_INCLUDED */
