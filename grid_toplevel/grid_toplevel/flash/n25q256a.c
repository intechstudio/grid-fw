/**
 * \file
 *
 * \brief N25Q256A component implement
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

#include <hal_gpio.h>
#include <hal_qspi_dma.h>
#include "spi_nor_flash.h"
#include "n25q256a.h"
#include "driver_init.h"

/** Register addresses of n25q256a */
/** Resume from deep power-down command code. */
#define N25Q_SOFT_RESET_ENABLE 0x66
/** Resume from deep power-down command code. */
#define N25Q_SOFT_RESET 0x99
/** Read status register command code. */
#define N25Q_READ_STATUS_REGISTER 0x05
/** Write status register command code. */
#define N25Q_WRITE_STATUS_REGISTER 0x01
/** Read status register command code. */
#define N25Q_READ_FLAG_STATUS_REGISTER 0x70
/** Write volatile config register command code. */
#define N25Q_WRITE_VOLATILE_CONFIG_REGISTER 0x81
/** Read volatile config register command code. */
#define N25Q_READ_VOLATILE_CONFIG_REGISTER 0x85
/** Write enhanced volatile config register command code. */
#define N25Q_WRITE_ENHANCED_VOLATILE_CONFIG_REGISTER 0x61
/** Read enhanced volatile config register command code. */
#define N25Q_READ_ENHANCED_VOLATILE_CONFIG_REGISTER 0x65
/** Write enable command code. */
#define N25Q_WRITE_ENABLE 0x06
/** Write disable command code. */
#define N25Q_WRITE_DISABLE 0x04
/** Byte/page program command code. */
#define N25Q_BYTE_PAGE_PROGRAM 0x02
/** Byte/page program command code. */
#define N25Q_QUAD_INPUT_PROGRAM 0x32
/** Block erase command code (4K block). */
#define N25Q_BLOCK_ERASE_4K 0x20
/** Block erase command code (64K block). */
#define N25Q_BLOCK_ERASE_64K 0xD8
/** Chip erase command code 2. */
#define N25Q_BULK_ERASE 0xC7
/** Read array (low frequency) command code. */
#define N25Q_READ_ARRAY_LF 0x03
/** Read array command code. */
#define N25Q_FAST_READ 0x0B
/** Fast Read array  command code. */
#define N25Q_DUAL_OUTPUT_READ 0x3B
/** Fast Read array  command code. */
#define N25Q_QUAD_OUTPUT_READ 0x6B
/** Fast Read array  command code. */
#define N25Q_DUAL_IO_FAST_READ 0xBB
/** Fast Read array  command code. */
#define N25Q_QUAD_I0_FAST_READ 0xEB
/** Read manufacturer and device ID command code. */
#define N25Q_READ_JEDEC_ID 0x9F

/** Size of n25q256a reg */
#define N25Q_RD_REG_SIZE(op) (1)
#define N25Q_WR_REG_SIZE(op) (1)

/** Size of n25q256a */
#define N25Q_PAGE_SIZE 256
#define N25Q_SECTOR_SIZE 4096
#define N25Q_BLOCK_SIZE 65536
#define N25Q_FLASH_SIZE 0x200000

/** wait here for CS to go high */


#define FLASH_CS PB11
void wait_cs_is_low()
{
	while (!gpio_get_pin_level(FLASH_CS))
		;
}

/** N25Q256A spi nor flash's interface */
static const struct spi_nor_flash_interface n25q256a_interface = {
    n25q256a_read,
    n25q256a_write,
    n25q256a_erase,
    n25q256a_enable_xip,
    n25q256a_disable_xip,
};

int32_t n25q256a_xip_confirm(const struct spi_nor_flash *const me, const bool on_off)
{
	uint8_t                     dummy;
	struct n25q256a *           n25q  = (struct n25q256a *)me;
	struct qspi_dma_descriptor *descr = (struct qspi_dma_descriptor *)(me->io);
	struct _qspi_command        cmd
	    = {.inst_frame.bits.width        = n25q->quad_mode ? QSPI_INST4_ADDR4_DATA4 : QSPI_INST1_ADDR1_DATA1,
	       .inst_frame.bits.inst_en      = 1,
	       .inst_frame.bits.data_en      = 1,
	       .inst_frame.bits.addr_en      = 1,
	       .inst_frame.bits.opt_en       = 1,
	       .inst_frame.bits.opt_len      = QSPI_OPT_8BIT,
	       .inst_frame.bits.dummy_cycles = n25q->quad_mode ? 8 : 0,
	       .inst_frame.bits.tfr_type     = QSPI_READMEM_ACCESS,
	       .instruction                  = N25Q_FAST_READ,
	       .option                       = on_off ? 0x00 : 0xFF,
	       .address                      = 0,
	       .buf_len                      = 1,
	       .rx_buf                       = &dummy};
	qspi_dma_serial_run_command(descr, &cmd);
	n25q->xip_mode = on_off ? 2 : 1;
	return ERR_NONE;
}

uint32_t n25q256a_read_reg(const struct spi_nor_flash *const me, uint8_t width, uint8_t inst)
{
	uint32_t                    status = 0;
	struct n25q256a *           n25q   = (struct n25q256a *)me;
	struct qspi_dma_descriptor *descr  = (struct qspi_dma_descriptor *)(me->io);
	struct _qspi_command        cmd    = {.inst_frame.bits.width    = width,
                                .inst_frame.bits.inst_en  = 1,
                                .inst_frame.bits.data_en  = 1,
                                .inst_frame.bits.tfr_type = QSPI_READ_ACCESS,
                                .instruction              = inst,
                                .buf_len                  = N25Q_RD_REG_SIZE(inst),
                                .rx_buf                   = &status};
	if (n25q->xip_mode) {
		n25q256a_xip_confirm(me, false);
	}
	qspi_dma_serial_run_command(descr, &cmd);
	if (n25q->xip_mode) {
		n25q256a_xip_confirm(me, true);
	}
	return status;
}
void n25q256a_write_reg(const struct spi_nor_flash *const me, uint8_t width, uint8_t inst, uint32_t data)
{
	struct n25q256a *           n25q  = (struct n25q256a *)me;
	struct qspi_dma_descriptor *descr = (struct qspi_dma_descriptor *)(me->io);
	struct _qspi_command        cmd   = {.inst_frame.bits.width    = width,
                                .inst_frame.bits.inst_en  = 1,
                                .inst_frame.bits.data_en  = 1,
                                .inst_frame.bits.tfr_type = QSPI_WRITE_ACCESS,
                                .instruction              = inst,
                                .buf_len                  = N25Q_WR_REG_SIZE(inst),
                                .tx_buf                   = &data};
	if (n25q->xip_mode) {
		n25q256a_xip_confirm(me, false);
	}
	qspi_dma_serial_run_command(descr, &cmd);
	if (n25q->xip_mode) {
		n25q256a_xip_confirm(me, true);
	}
}

void n25q256a_write_enable(const struct spi_nor_flash *const me, uint8_t width, bool en)
{
	uint8_t                     status;
	struct qspi_dma_descriptor *descr = (struct qspi_dma_descriptor *)(me->io);
	uint8_t                     inst  = (en == true ? N25Q_WRITE_ENABLE : N25Q_WRITE_DISABLE);
	struct _qspi_command cmd_en = {.inst_frame.bits.width = width, .inst_frame.bits.inst_en = 1, .instruction = inst};
	struct _qspi_command cmd_st = {.inst_frame.bits.width    = width,
	                               .inst_frame.bits.inst_en  = 1,
	                               .inst_frame.bits.data_en  = 1,
	                               .inst_frame.bits.tfr_type = QSPI_READ_ACCESS,
	                               .instruction              = N25Q_READ_STATUS_REGISTER,
	                               .buf_len                  = 1,
	                               .rx_buf                   = &status};
	do {
		qspi_dma_serial_run_command(descr, &cmd_en);
		qspi_dma_serial_run_command(descr, &cmd_st);
	} while ((status & (1 << 1)) == 0);
}

void n25q256a_switch_mode(const struct spi_nor_flash *const me, uint8_t mode)
{
	struct n25q256a *n25q   = (struct n25q256a *)me;
	uint8_t          width  = n25q->quad_mode ? QSPI_INST4_ADDR4_DATA4 : QSPI_INST1_ADDR1_DATA1;
	uint32_t         evcfg  = n25q256a_read_reg(me, width, N25Q_READ_ENHANCED_VOLATILE_CONFIG_REGISTER);
	uint8_t          modify = 0;
	if (n25q->quad_mode == mode) {
		return;
	}
	if (mode) {
		if (evcfg & 0x80) {
			evcfg &= 0x1F;
			modify = 1;
		}
	} else {
		if (!(evcfg & 0x80)) {
			evcfg |= 0xC0;
			modify = 1;
		}
	}
	if (modify) {
		n25q->quad_mode = mode;
		n25q256a_write_enable(me, width, true);
		n25q256a_write_reg(me, width, N25Q_WRITE_ENHANCED_VOLATILE_CONFIG_REGISTER, evcfg);
		width           = mode ? QSPI_INST4_ADDR4_DATA4 : QSPI_INST1_ADDR1_DATA1;
		evcfg           = n25q256a_read_reg(me, width, N25Q_READ_ENHANCED_VOLATILE_CONFIG_REGISTER);
		n25q->quad_mode = mode;
	}
}

/**
 * \brief Construct n25q256a spi nor flash
 */
struct spi_nor_flash *n25q256a_construct(struct spi_nor_flash *const me, void *const io, func pin_exit_xip,
                                         const uint8_t quad_mode)
{
	struct n25q256a *n25q = (struct n25q256a *)me;
	spi_nor_flash_construct(me, io, &n25q256a_interface);
	n25q->quad_mode    = 0;
	n25q->xip_mode     = false;
	n25q->pin_exit_xip = pin_exit_xip;
	n25q256a_switch_mode(me, quad_mode);
	return me;
}

int32_t n25q256a_read(const struct spi_nor_flash *const me, uint8_t *buf, uint32_t address, uint32_t length)
{
	struct n25q256a *           n25q  = (struct n25q256a *)me;
	struct qspi_dma_descriptor *descr = (struct qspi_dma_descriptor *)(me->io);

	struct _qspi_command cmd = {
	    .inst_frame.bits.width        = n25q->quad_mode ? QSPI_INST4_ADDR4_DATA4 : QSPI_INST1_ADDR1_DATA1,
	    .inst_frame.bits.inst_en      = 1,
	    .inst_frame.bits.data_en      = 1,
	    .inst_frame.bits.addr_en      = 1,
	    .inst_frame.bits.dummy_cycles = n25q->quad_mode ? 10 : 8,
	    .inst_frame.bits.tfr_type     = QSPI_READMEM_ACCESS,
	    .instruction                  = N25Q_FAST_READ,
	    .address                      = address,
	    .buf_len                      = length,
	    .rx_buf                       = buf,
	};
	qspi_dma_serial_run_command(descr, &cmd);
	return ERR_NONE;
}

int32_t n25q256a_write(const struct spi_nor_flash *const me, uint8_t *buf, uint32_t address, uint32_t length)
{
	struct n25q256a *           n25q  = (struct n25q256a *)me;
	struct qspi_dma_descriptor *descr = (struct qspi_dma_descriptor *)(me->io);
	struct _qspi_command        cmd   = {
        .inst_frame.bits.inst_en  = 1,
        .inst_frame.bits.data_en  = 1,
        .inst_frame.bits.addr_en  = 1,
        .inst_frame.bits.tfr_type = QSPI_WRITEMEM_ACCESS,
        .instruction              = N25Q_BYTE_PAGE_PROGRAM, /* Command is for all modes */
        .address                  = address,
        .buf_len                  = length,
        .tx_buf                   = buf,
    };
	/* Command, address, data width are different for different modes */
	cmd.inst_frame.bits.width = n25q->quad_mode ? QSPI_INST4_ADDR4_DATA4 : QSPI_INST1_ADDR1_DATA1;
	while (length) {
		cmd.address = address;
		if (length <= N25Q_PAGE_SIZE) {
			cmd.buf_len = length;
		} else {
			cmd.buf_len = N25Q_PAGE_SIZE;
		}
		/* PROGRAM commands are initiated by first executing the
		 * WRITE ENABLE command to set the write enable latch bit to 1.
		 */
		n25q256a_write_enable(me, cmd.inst_frame.bits.width, true);
		/* Send PROGRAM command */
		qspi_dma_serial_run_command(descr, &cmd);

		/* wait here to complete the page write.
		 * check the CS line status. if CS is low then wait for CS go to high
		 */
		wait_cs_is_low();

		while (!(n25q256a_read_reg(me, cmd.inst_frame.bits.width, N25Q_READ_FLAG_STATUS_REGISTER) & (1 << 7)))
			;
		cmd.tx_buf += cmd.buf_len;
		length -= cmd.buf_len;
		address += cmd.buf_len;
	}
	if (n25q->xip_mode) {
		n25q256a_xip_confirm(me, true);
	}
	return ERR_NONE;
}

int32_t n25q256a_erase(const struct spi_nor_flash *const me, uint32_t address, uint32_t length)
{
	struct n25q256a *           n25q  = (struct n25q256a *)me;
	struct qspi_dma_descriptor *descr = (struct qspi_dma_descriptor *)(me->io);
	struct _qspi_command        cmd   = {
        .inst_frame.bits.width    = n25q->quad_mode ? QSPI_INST4_ADDR4_DATA4 : QSPI_INST1_ADDR1_DATA1,
        .inst_frame.bits.inst_en  = 1,
        .inst_frame.bits.addr_en  = (length < N25Q_FLASH_SIZE) ? 1 : 0,
        .inst_frame.bits.tfr_type = QSPI_WRITE_ACCESS,
    };
	uint32_t temp_addr = address;
	uint32_t temp_len  = length;
	int32_t  rc        = ERR_NONE;

	if ((length % N25Q_SECTOR_SIZE) || (address % N25Q_SECTOR_SIZE)) {
		return ERR_INVALID_ARG;
	}

	if (length >= N25Q_FLASH_SIZE) {
		cmd.instruction = N25Q_BULK_ERASE;
		/* WRITE ENABLE command must be issued to
		 * set the write enable latch bit to 1 */
		n25q256a_write_enable(me, cmd.inst_frame.bits.width, true);
		/* Send specific erase command */
		qspi_dma_serial_run_command(descr, &cmd);
	} else {
		while (temp_len > 0) {
			if (((temp_addr % N25Q_BLOCK_SIZE) == 0) && (temp_len >= N25Q_BLOCK_SIZE)) {
				cmd.address     = temp_addr;
				cmd.instruction = N25Q_BLOCK_ERASE_64K;
				/* WRITE ENABLE command must be issued to
				 * set the write enable latch bit to 1 */
				n25q256a_write_enable(me, cmd.inst_frame.bits.width, true);
				/* Send specific erase command */
				qspi_dma_serial_run_command(descr, &cmd);
				temp_addr += N25Q_BLOCK_SIZE;
				temp_len -= N25Q_BLOCK_SIZE;
			} else if (temp_len >= N25Q_SECTOR_SIZE) {
				cmd.address     = temp_addr;
				cmd.instruction = N25Q_BLOCK_ERASE_4K;
				/* WRITE ENABLE command must be issued to
				 * set the write enable latch bit to 1 */
				n25q256a_write_enable(me, cmd.inst_frame.bits.width, true);
				/* Send specific erase command */
				qspi_dma_serial_run_command(descr, &cmd);
				temp_addr += N25Q_SECTOR_SIZE;
				temp_len -= N25Q_SECTOR_SIZE;
			} else {
				rc = ERR_INVALID_ARG;
				break;
			}
			/* When the operation is in progress,
			 * the write in progress bit is set.
			 * The write enable latch bit is cleared.
			 * The flag status register is polled for the operation status.
			 */
			while (!(n25q256a_read_reg(me, cmd.inst_frame.bits.width, N25Q_READ_FLAG_STATUS_REGISTER) & (1 << 7)))
				;
		}
	}
	/* The flag status register is polled for the final operation status. */
	while (!(n25q256a_read_reg(me, cmd.inst_frame.bits.width, N25Q_READ_FLAG_STATUS_REGISTER) & (1 << 7)))
		;
	if (n25q->xip_mode) {
		n25q256a_xip_confirm(me, true);
	}
	return rc;
}

int32_t n25q256a_enable_xip(const struct spi_nor_flash *const me)
{
	struct n25q256a *n25q  = (struct n25q256a *)me;
	uint8_t          width = n25q->quad_mode ? QSPI_INST4_ADDR4_DATA4 : QSPI_INST1_ADDR1_DATA1;
	if (!n25q->xip_mode) {
		uint8_t vcfg = n25q256a_read_reg(me, width, N25Q_READ_VOLATILE_CONFIG_REGISTER);
		if (vcfg & (1u << 3)) {
			vcfg &= ~(1u << 3);
			n25q256a_write_reg(me, width, N25Q_WRITE_VOLATILE_CONFIG_REGISTER, vcfg);
			while (!(n25q256a_read_reg(me, width, N25Q_READ_FLAG_STATUS_REGISTER) & (1 << 7)))
				;
		}
		n25q->xip_mode = 1;
		n25q256a_xip_confirm(me, true);
	}
	return ERR_NONE;
}

int32_t n25q256a_disable_xip(const struct spi_nor_flash *const me)
{
	struct n25q256a *           n25q  = (struct n25q256a *)me;
	struct qspi_dma_descriptor *descr = (struct qspi_dma_descriptor *)(me->io);
	uint8_t                     width = n25q->quad_mode ? QSPI_INST4_ADDR4_DATA4 : QSPI_INST1_ADDR1_DATA1;
	uint8_t                     vcfg;
	/* XIP is terminated by driving the XIP confirmation bit to 1.
	 * The device automatically resets volatile configuration register bit 3 to 1.
	 */
	n25q256a_xip_confirm(me, false);
	vcfg = n25q256a_read_reg(me, width, N25Q_READ_VOLATILE_CONFIG_REGISTER);
	if (!(vcfg & (1u << 3))) {
		if (n25q->pin_exit_xip) {
			/* Quad    : drive DQ0 = 1 with S# held LOW for  7 clock cycles
			 * Dual    : drive DQ0 = 1 with S# held LOW for 13 clock cycles
			 * Extended: drive DQ0 = 1 with S# held LOW for 25 clock cycles
			 */
			void *hw = descr->dev.prvt;
			qspi_dma_disable(descr);
			n25q->pin_exit_xip();

			qspi_dma_init(descr, hw);
			qspi_dma_enable(descr);
		}
	}
	n25q->xip_mode = 0;
	return ERR_NONE;
}
