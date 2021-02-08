/**
 * \file
 *
 * \brief  SPI NOR Flash implementation.
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

#include "spi_nor_flash.h"

struct spi_nor_flash *spi_nor_flash_construct(struct spi_nor_flash *const me, void *const io,
                                              const struct spi_nor_flash_interface *const interface)
{
	me->io        = io;
	me->interface = interface;

	return me;
}

int32_t spi_nor_flash_read(const struct spi_nor_flash *const me, uint8_t *buf, uint32_t address, uint32_t length)
{
	return me->interface->read(me, buf, address, length);
}

int32_t spi_nor_flash_write(const struct spi_nor_flash *const me, uint8_t *buf, uint32_t address, uint32_t length)
{
	return me->interface->write(me, buf, address, length);
}

int32_t spi_nor_flash_erase(const struct spi_nor_flash *const me, uint32_t address, uint32_t length)
{
	return me->interface->erase(me, address, length);
}

int32_t spi_nor_flash_enable_xip(const struct spi_nor_flash *const me)
{
	return me->interface->enxip(me);
}

int32_t spi_nor_flash_disable_xip(const struct spi_nor_flash *const me)
{
	return me->interface->disxip(me);
}
