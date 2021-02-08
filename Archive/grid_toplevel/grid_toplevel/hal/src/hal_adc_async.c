/**
 * \file
 *
 * \brief ADC functionality implementation.
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

/**
 * \brief Indicates HAL being compiled. Must be defined before including.
 */
#define _COMPILING_HAL

#include "hal_adc_async.h"
#include <utils_assert.h>
#include <utils.h>
#include <hal_atomic.h>

/**
 * \brief Driver version
 */
#define DRIVER_VERSION 0x00000001u

static void adc_async_channel_conversion_done(struct _adc_async_device *device, const uint8_t channel,
                                              const uint16_t data);
static void adc_async_window_threshold_reached(struct _adc_async_device *device, const uint8_t channel);
static void adc_async_error_occured(struct _adc_async_device *device, const uint8_t channel);

/**
 * \brief Initialize ADC
 */
int32_t adc_async_init(struct adc_async_descriptor *const descr, void *const hw, uint8_t *channel_map,
                       uint8_t channel_max, uint8_t channel_amount, struct adc_async_channel_descriptor *const descr_ch,
                       void *const func)
{
	int32_t                   init_status;
	struct _adc_async_device *device;
	ASSERT(descr && hw && channel_map && channel_amount && descr_ch);
	ASSERT(channel_amount <= (channel_max + 1));

	device = &descr->device;
	for (uint8_t i = 0; i <= channel_max; i++) {
		channel_map[i] = 0xFF;
	}
	descr->channel_map    = channel_map;
	descr->channel_max    = channel_max;
	descr->channel_amount = channel_amount;
	descr->descr_ch       = descr_ch;
	init_status           = _adc_async_init(device, hw);
	if (init_status) {
		return init_status;
	}

	device->adc_async_ch_cb.convert_done = adc_async_channel_conversion_done;
	device->adc_async_cb.window_cb       = adc_async_window_threshold_reached;
	device->adc_async_cb.error_cb        = adc_async_error_occured;

	return ERR_NONE;
}

/**
 * \brief Deinitialize ADC
 */
int32_t adc_async_deinit(struct adc_async_descriptor *const descr)
{
	ASSERT(descr);
	_adc_async_deinit(&descr->device);

	return ERR_NONE;
}

int32_t adc_async_register_channel_buffer(struct adc_async_descriptor *const descr, const uint8_t channel,
                                          uint8_t *const convert_buffer, const uint16_t convert_buffer_length)
{
	uint8_t i, index = 0;

	ASSERT(descr && convert_buffer && convert_buffer_length);
	ASSERT(descr->channel_max >= channel);

	if (descr->channel_map[channel] != 0xFF) {
		return ERR_INVALID_ARG;
	}
	for (i = 0; i <= descr->channel_max; i++) {
		if (descr->channel_map[i] != 0xFF) {
			index++;
		}
	}

	if (index > descr->channel_amount) {
		return ERR_NO_RESOURCE;
	}
	if (ERR_NONE != ringbuffer_init(&descr->descr_ch[index].convert, convert_buffer, convert_buffer_length)) {
		return ERR_INVALID_ARG;
	}
	descr->channel_map[channel]            = index;
	descr->descr_ch[index].bytes_in_buffer = 0;

	return ERR_NONE;
}

/**
 * \brief Enable ADC
 */
int32_t adc_async_enable_channel(struct adc_async_descriptor *const descr, const uint8_t channel)
{
	ASSERT(descr);
	ASSERT(descr->channel_max >= channel);
	_adc_async_enable_channel(&descr->device, channel);

	return ERR_NONE;
}

/**
 * \brief Disable ADC
 */
int32_t adc_async_disable_channel(struct adc_async_descriptor *const descr, const uint8_t channel)
{
	ASSERT(descr);
	ASSERT(descr->channel_max >= channel);
	_adc_async_disable_channel(&descr->device, channel);

	return ERR_NONE;
}

/**
 * \brief Register ADC callback
 */
int32_t adc_async_register_callback(struct adc_async_descriptor *const descr, const uint8_t channel,
                                    const enum adc_async_callback_type type, adc_async_cb_t cb)
{
	ASSERT(descr);
	ASSERT(descr->channel_max >= channel);

	uint8_t index = descr->channel_map[channel];
	switch (type) {
	case ADC_ASYNC_CONVERT_CB:
		descr->descr_ch[index].adc_async_ch_cb.convert_done = cb;
		break;
	case ADC_ASYNC_MONITOR_CB:
		descr->adc_async_cb.monitor = cb;
		break;
	case ADC_ASYNC_ERROR_CB:
		descr->adc_async_cb.error = cb;
		break;
	default:
		return ERR_INVALID_ARG;
	}
	_adc_async_set_irq_state(&descr->device, channel, (enum _adc_async_callback_type)type, cb != NULL);

	return ERR_NONE;
}

/*
 * \brief Read data from ADC
 */
int32_t adc_async_read_channel(struct adc_async_descriptor *const descr, const uint8_t channel, uint8_t *const buffer,
                               const uint16_t length)
{
	uint8_t  data_size, index;
	uint32_t num;
	uint16_t was_read = 0;

	ASSERT(descr && buffer && length);
	ASSERT(descr->channel_max >= channel);
	data_size = _adc_async_get_data_size(&descr->device);
	ASSERT(!(length % data_size));
	(void)data_size;

	index                                         = descr->channel_map[channel];
	struct adc_async_channel_descriptor *descr_ch = &descr->descr_ch[index];

	CRITICAL_SECTION_ENTER()
	num = ringbuffer_num(&descr_ch->convert);
	CRITICAL_SECTION_LEAVE()

	while ((was_read < num) && (was_read < length)) {
		ringbuffer_get(&descr_ch->convert, &buffer[was_read++]);
	}
	descr_ch->bytes_in_buffer -= was_read;

	return was_read;
}

/**
 * \brief Start conversion
 */
int32_t adc_async_start_conversion(struct adc_async_descriptor *const descr)
{
	ASSERT(descr);
	_adc_async_convert(&descr->device);
	return ERR_NONE;
}

/**
 * \brief Set ADC reference source
 */
int32_t adc_async_set_reference(struct adc_async_descriptor *const descr, const adc_reference_t reference)
{
	ASSERT(descr);
	_adc_async_set_reference_source(&descr->device, reference);
	return ERR_NONE;
}

/**
 * \brief Set ADC resolution
 */
int32_t adc_async_set_resolution(struct adc_async_descriptor *const descr, const adc_resolution_t resolution)
{
	ASSERT(descr);
	_adc_async_set_resolution(&descr->device, resolution);
	return ERR_NONE;
}

/**
 * \brief Set ADC input source for a channel
 */
int32_t adc_async_set_inputs(struct adc_async_descriptor *const descr, const adc_pos_input_t pos_input,
                             const adc_neg_input_t neg_input, const uint8_t channel)
{
	ASSERT(descr);
	ASSERT(descr->channel_max >= channel);
	_adc_async_set_inputs(&descr->device, pos_input, neg_input, channel);
	return ERR_NONE;
}

/**
 * \brief Set ADC thresholds
 */
int32_t adc_async_set_thresholds(struct adc_async_descriptor *const descr, const adc_threshold_t low_threshold,
                                 const adc_threshold_t up_threshold)
{
	ASSERT(descr);
	_adc_async_set_thresholds(&descr->device, low_threshold, up_threshold);
	return ERR_NONE;
}

/**
 * \brief Set ADC gain
 */
int32_t adc_async_set_channel_gain(struct adc_async_descriptor *const descr, const uint8_t channel,
                                   const adc_gain_t gain)
{
	ASSERT(descr);
	ASSERT(descr->channel_max >= channel);
	_adc_async_set_channel_gain(&descr->device, channel, gain);
	return ERR_NONE;
}

/**
 * \brief Set ADC conversion mode
 */
int32_t adc_async_set_conversion_mode(struct adc_async_descriptor *const descr, const enum adc_conversion_mode mode)
{
	ASSERT(descr);
	_adc_async_set_conversion_mode(&descr->device, mode);
	return ERR_NONE;
}

/**
 * \brief Set ADC differential mode
 */
int32_t adc_async_set_channel_differential_mode(struct adc_async_descriptor *const descr, const uint8_t channel,
                                                const enum adc_differential_mode mode)
{
	ASSERT(descr);
	ASSERT(descr->channel_max >= channel);
	_adc_async_set_channel_differential_mode(&descr->device, channel, mode);
	return ERR_NONE;
}

/**
 * \brief Set ADC window mode
 */
int32_t adc_async_set_window_mode(struct adc_async_descriptor *const descr, const adc_window_mode_t mode)
{
	ASSERT(descr);
	_adc_async_set_window_mode(&descr->device, mode);
	return ERR_NONE;
}

/**
 * \brief Retrieve threshold state
 */
int32_t adc_async_get_threshold_state(const struct adc_async_descriptor *const descr,
                                      adc_threshold_status_t *const            state)
{
	ASSERT(descr && state);
	_adc_async_get_threshold_state(&descr->device, state);
	return ERR_NONE;
}

/**
 * \brief Check if conversion is complete
 */
int32_t adc_async_is_channel_conversion_complete(const struct adc_async_descriptor *const descr, const uint8_t channel)
{
	ASSERT(descr);
	ASSERT(descr->channel_max >= channel);
	return _adc_async_is_channel_conversion_done(&descr->device, channel);
}

/**
 * \brief flush adc ringbuf
 */
int32_t adc_async_flush_rx_buffer(struct adc_async_descriptor *const descr, const uint8_t channel)
{
	ASSERT(descr);
	ASSERT(descr->channel_max >= channel);

	uint8_t                              index    = descr->channel_map[channel];
	struct adc_async_channel_descriptor *descr_ch = &descr->descr_ch[index];

	return ringbuffer_flush(&descr_ch->convert);
}

/**
 * \brief Retrieve the current driver version
 */
uint32_t adc_async_get_version(void)
{
	return DRIVER_VERSION;
}

/**
 * \internal Process conversion completion
 *
 * \param[in] device The pointer to ADC device structure
 * \param[in] data Converted data
 */
static void adc_async_channel_conversion_done(struct _adc_async_device *device, const uint8_t channel,
                                              const uint16_t data)
{
	struct adc_async_descriptor *const descr = CONTAINER_OF(device, struct adc_async_descriptor, device);

	uint8_t                              index    = descr->channel_map[channel];
	struct adc_async_channel_descriptor *descr_ch = &descr->descr_ch[index];

	ringbuffer_put(&descr_ch->convert, data);
	if (1 < _adc_async_get_data_size(&descr->device)) {
		ringbuffer_put(&descr_ch->convert, data >> 8);
		++descr_ch->bytes_in_buffer;
	}
	++descr_ch->bytes_in_buffer;

	if (descr_ch->adc_async_ch_cb.convert_done) {
		descr_ch->adc_async_ch_cb.convert_done(descr, channel);
	}
}

static void adc_async_window_threshold_reached(struct _adc_async_device *device, const uint8_t channel)
{
	struct adc_async_descriptor *const descr = CONTAINER_OF(device, struct adc_async_descriptor, device);

	if (descr->adc_async_cb.monitor) {
		descr->adc_async_cb.monitor(descr, channel);
	}
}

static void adc_async_error_occured(struct _adc_async_device *device, const uint8_t channel)
{
	struct adc_async_descriptor *const descr = CONTAINER_OF(device, struct adc_async_descriptor, device);

	if (descr->adc_async_cb.error) {
		descr->adc_async_cb.error(descr, channel);
	}
}

//@}
