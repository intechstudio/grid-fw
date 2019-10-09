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

/**
 * \brief Return version
 */
uint32_t audiodf_midi_get_version(void);

#endif /* USBDF_AUDIO_MIDI_H_ */
