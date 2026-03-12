/**
 * \file
 *
 * \brief SPI related functionality declaration.
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

#ifndef _HAL_SPI_M_ASYNC_H_INCLUDED
#define _HAL_SPI_M_ASYNC_H_INCLUDED

#include <hal_io.h>
#include <hpl_spi_m_async.h>

/**
 * \addtogroup doc_driver_hal_spi_master_async
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** \brief SPI status
 *
 *  Status descriptor holds the current status of transfer.
 *
 *  \c txcnt and \c rxcnt are always the status of progress in current TX/RX
 *  transfer buffer.
 *
 *  For R/W/Transfer, simply check \c SPI_M_ASYNC_STATUS_BUSY to know that the
 *  transfer is in progress, check \c SPI_M_ASYNC_STATUS_TX_DONE and
 *  \c SPI_M_ASYNC_STATUS_RX_DONE to know that TX or RX is completed (since TX
 *  and RX happen in different clock edge the time stamp of completion is
 *  different), check \c SPI_M_ASYNC_STATUS_COMPLETE to confirm that CS has been
 *  deactivate.
 */
struct spi_m_async_status {
	/** Status flags */
	uint32_t flags;
	/** Number of characters transmitted */
	uint32_t xfercnt;
};
/** SPI is busy (read/write/transfer, with CS activated) */
#define SPI_M_ASYNC_STATUS_BUSY 0x0010
/** SPI finished transmit buffer */
#define SPI_M_ASYNC_STATUS_TX_DONE 0x0020
/** SPI finished receive buffer */
#define SPI_M_ASYNC_STATUS_RX_DONE 0x0040
/** SPI finished everything including CS deactivate */
#define SPI_M_ASYNC_STATUS_COMPLETE 0x0080
#define SPI_M_ASYNC_STATUS_ERR_MASK 0x000F
#define SPI_M_ASYNC_STATUS_ERR_POS 0
#define SPI_M_ASYNC_STATUS_ERR_OVRF ((-ERR_OVERFLOW) << SPI_M_ASYNC_STATUS_ERR_POS)
#define SPI_M_ASYNC_STATUS_ERR_ABORT ((-ERR_ABORTED) << SPI_M_ASYNC_STATUS_ERR_POS)
#define SPI_M_ASYNC_STATUS_ERR_EXTRACT(st) (((st) >> SPI_M_ASYNC_STATUS_ERR_POS) & SPI_M_ASYNC_STATUS_ERR_MASK)

/* Forward declaration of spi_descriptor. */
struct spi_m_async_descriptor;

/** The callback types */
enum spi_m_async_cb_type {
	/** Callback type for read/write/transfer buffer done,
	 *  see \ref spi_m_async_cb_xfer_t. */
	SPI_M_ASYNC_CB_XFER,
	/** Callback type for CS deactivate, error, or abort,
	 *  see \ref spi_m_async_cb_error_t. */
	SPI_M_ASYNC_CB_ERROR,
	SPI_M_ASYNC_CB_N
};

/** \brief Prototype of callback on SPI transfer errors
 *
 *  Invoked on transfer errors
 *  invoke \ref spi_get_status.
 */
typedef void (*spi_m_async_cb_error_t)(struct spi_m_async_descriptor *, const int32_t status);

/** \brief Prototype of callback on SPI read/write/transfer buffer completion
 *
 *  Invoked on transfer completion, which means the transfer buffer has been
 *  completed, including all TX/RX data (TX and RX happen in different clock
 *  edges, but the callback is invoked after all TX and RX have been done).
 */
typedef void (*spi_m_async_cb_xfer_t)(struct spi_m_async_descriptor *);

/** \brief SPI HAL callbacks
 *
 */
struct spi_m_callbacks {
	/** Callback invoked when the buffer read/write/transfer done. */
	spi_m_async_cb_xfer_t cb_xfer;
	/** Callback invoked when the CS deactivates, goes wrong, or aborts. */
	spi_m_async_cb_error_t cb_error;
};

/** \brief SPI HAL driver struct for asynchronous access
 */
struct spi_m_async_descriptor {
	struct _spi_m_async_hpl_interface *func;
	/** Pointer to the SPI device instance */
	struct _spi_m_async_dev dev;
	/** I/O read/write */
	struct io_descriptor io;

	/** SPI transfer status */
	uint8_t stat;

	/** Callbacks for asynchronous transfer */
	struct spi_m_callbacks callbacks;
	/** Transfer information copy, for R/W/Transfer */
	struct spi_xfer xfer;
	/** Character count in current transfer */
	uint32_t xfercnt;
};

/** \brief Set the SPI HAL instance function pointer for HPL APIs.
 *
 *  Set SPI HAL instance function pointer for HPL APIs.
 *
 *  \param[in] spi Pointer to the HAL SPI instance.
 *  \param[in] func Pointer to the HPL api structure.
 *
 */
void spi_m_async_set_func_ptr(struct spi_m_async_descriptor *spi, void *const func);

/** \brief Initialize the SPI HAL instance and hardware for callback mode
 *
 *  Initialize SPI HAL with interrupt mode (uses callbacks).
 *
 *  \param[in] spi Pointer to the HAL SPI instance.
 *  \param[in] hw Pointer to the hardware base.
 *
 *  \return Operation status.
 *  \retval ERR_NONE Success.
 *  \retval ERR_INVALID_DATA Error, initialized.
 */
int32_t spi_m_async_init(struct spi_m_async_descriptor *spi, void *const hw);

/** \brief Deinitialize the SPI HAL instance
 *
 *  Abort transfer, disable and reset SPI, de-init software.
 *
 *  \param[in] spi Pointer to the HAL SPI instance.
 *
 *  \return Operation status.
 *  \retval ERR_NONE Success.
 *  \retval <0 Error code.
 */
void spi_m_async_deinit(struct spi_m_async_descriptor *spi);

/** \brief Enable SPI
 *
 *  \param[in] spi Pointer to the HAL SPI instance.
 *
 *  \return Operation status.
 *  \retval ERR_NONE Success.
 *  \retval <0 Error code.
 */
void spi_m_async_enable(struct spi_m_async_descriptor *spi);

/** \brief Disable the SPI and abort any pending transfer in progress
 *
 * If there is any pending transfer, the complete callback is invoked
 * with the \c ERR_ABORTED status.
 *
 *  \param[in] spi Pointer to the HAL SPI instance.
 *
 *  \return Operation status.
 *  \retval ERR_NONE Success.
 *  \retval <0 Error code.
 */
void spi_m_async_disable(struct spi_m_async_descriptor *spi);

/** \brief Set SPI baudrate
 *
 *  Works if the SPI is initialized as master.
 *  In the function a sanity check is used to confirm it's called in the correct mode.
 *
 *  \param[in] spi Pointer to the HAL SPI instance.
 *  \param[in] baud_val The target baudrate value
 *                  (see "baudrate calculation" for calculating the value).
 *
 *  \return Operation status.
 *  \retval ERR_NONE Success.
 *  \retval ERR_BUSY Busy.
 */
int32_t spi_m_async_set_baudrate(struct spi_m_async_descriptor *spi, const uint32_t baud_val);

/** \brief Set SPI mode
 *
 *  Set the SPI transfer mode (\ref spi_transfer_mode),
 *  which controls the clock polarity and clock phase:
 *  - Mode 0: leading edge is rising edge, data sample on leading edge.
 *  - Mode 1: leading edge is rising edge, data sample on trailing edge.
 *  - Mode 2: leading edge is falling edge, data sample on leading edge.
 *  - Mode 3: leading edge is falling edge, data sample on trailing edge.
 *
 *  \param[in] spi Pointer to the HAL SPI instance.
 *  \param[in] mode The mode (\ref spi_transfer_mode).
 *
 *  \return Operation status.
 *  \retval ERR_NONE Success.
 *  \retval ERR_BUSY Busy, CS activated.
 */
int32_t spi_m_async_set_mode(struct spi_m_async_descriptor *spi, const enum spi_transfer_mode mode);

/** \brief Set SPI transfer character size in number of bits
 *
 *  The character size (\ref spi_char_size) influence the way the data is
 *  sent/received.
 *  For char size <= 8-bit, data is stored byte by byte.
 *  For char size between 9-bit ~ 16-bit, data is stored in 2-byte length.
 *  Note that the default and recommended char size is 8-bit since it's
 *  supported by all system.
 *
 *  \param[in] spi Pointer to the HAL SPI instance.
 *  \param[in] char_size The char size (\ref spi_char_size).
 *
 *  \return Operation status.
 *  \retval ERR_NONE Success.
 *  \retval ERR_BUSY Busy, CS activated.
 *  \retval ERR_INVALID_ARG The char size is not supported.
 */
int32_t spi_m_async_set_char_size(struct spi_m_async_descriptor *spi, const enum spi_char_size char_size);

/** \brief Set SPI transfer data order
 *
 *  \param[in] spi Pointer to the HAL SPI instance.
 *  \param[in] dord The data order: send LSB/MSB first.
 *
 *  \return Operation status.
 *  \retval ERR_NONE Success.
 *  \retval ERR_BUSY Busy, CS activated.
 *  \retval ERR_INVALID The data order is not supported.
 */
int32_t spi_m_async_set_data_order(struct spi_m_async_descriptor *spi, const enum spi_data_order dord);

/** \brief Perform the SPI data transfer (TX and RX) asynchronously
 *
 *  Log the TX and RX buffers and transfer them in the background. It never blocks.
 *
 *  \param[in] spi Pointer to the HAL SPI instance.
 *  \param[in] txbuf Pointer to the transfer information (\ref spi_transfer).
 *  \param[out] rxbuf Pointer to the receiver information (\ref spi_receive).
 *  \param[in] length SPI transfer data length.
 *
 *  \return Operation status.
 *  \retval ERR_NONE Success.
 *  \retval ERR_BUSY Busy.
 */
int32_t spi_m_async_transfer(struct spi_m_async_descriptor *spi, uint8_t const *txbuf, uint8_t *const rxbuf,
                             const uint16_t length);

/** \brief Get the SPI transfer status
 *
 *  Get transfer status, transfer counts in a structured way.
 *
 *  \param[in] spi Pointer to the HAL SPI instance.
 *  \param[out] stat Pointer to the detailed status descriptor, set to NULL
 *                to not return details.
 *
 *  \return Status.
 *  \retval ERR_NONE  Not busy.
 *  \retval ERR_BUSY  Busy.
 */
int32_t spi_m_async_get_status(struct spi_m_async_descriptor *spi, struct spi_m_async_status *stat);

/** \brief Register a function as SPI transfer completion callback
 *
 *  Register callback function specified by its \c type.
 *  - SPI_CB_COMPLETE: set the function that will be called on the SPI transfer
 *    completion including deactivating the CS.
 *  - SPI_CB_XFER: set the function that will be called on the SPI buffer transfer
 *    completion.
 *  Register NULL function to not use the callback.
 *
 *  \param[in] spi Pointer to the HAL SPI instance.
 *  \param[in] type Callback type (\ref spi_m_async_cb_type).
 *  \param[in] func Pointer to callback function.
 */
void spi_m_async_register_callback(struct spi_m_async_descriptor *spi, const enum spi_m_async_cb_type type,
                                   FUNC_PTR func);

/**
 * \brief Return I/O descriptor for this SPI instance
 *
 * This function will return an I/O instance for this SPI driver instance
 *
 * \param[in] spi An SPI master descriptor, which is used to communicate through
 *                SPI
 * \param[in, out] io A pointer to an I/O descriptor pointer type
 *
 * \retval ERR_NONE
 */
int32_t spi_m_async_get_io_descriptor(struct spi_m_async_descriptor *const spi, struct io_descriptor **io);

/** \brief Retrieve the current driver version
 *
 *  \return Current driver version.
 */
uint32_t spi_m_async_get_version(void);

#ifdef __cplusplus
}
#endif
/**@}*/
#endif /* ifndef _HAL_SPI_M_ASYNC_H_INCLUDED */
