#ifndef USBDF_AUDIO_MIDI_H_
#define USBDF_AUDIO_MIDI_H_

#include "usbdc.h"



/**
 * \brief Initialize the USB Audio Midi Function Driver
 * \return Operation status.
 */
int32_t audiodf_midi_init(void);

/**
 * \brief Deinitialize the USB Audio Midi Function Driver
 * \return Operation status.
 */
int32_t audiodf_midi_deinit(void);

/**
 * \brief Check whether Audio Midi Function is enabled
 * \return Operation status.
 * \return true Audio Midi Function is enabled
 * \return false Audio Midi Function is disabled
 */
bool audiodf_midi_is_enabled(void);

/** AUDIO MIDI Class Callback Type */
enum audiodf_midi_cb_type { AUDIODF_MIDI_CB_READ, AUDIODF_MIDI_CB_WRITE, AUDIODF_MIDI_CB_INSTALLED};

int32_t audiodf_midi_write_status();
int32_t audiodf_midi_write(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3);
int32_t audiodf_midi_read(uint8_t *buf, uint32_t size);


int32_t audiodf_midi_register_callback(enum audiodf_midi_cb_type cb_type, FUNC_PTR func);

/**
 * \brief Return version
 */
uint32_t audiodf_midi_get_version(void);

#endif /* USBDF_AUDIO_MIDI_H_ */
