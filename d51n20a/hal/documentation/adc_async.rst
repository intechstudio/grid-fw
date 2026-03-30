=======================
ADC Asynchronous driver
=======================

An ADC (Analog-to-Digital Converter) converts analog signals to digital values.
A reference signal with a known voltage level is quantified into equally
sized chunks, each representing a digital value from 0 to the highest number
possible with the bit resolution supported by the ADC. The input voltage
measured by the ADC is compared against these chunks and the chunk with the
closest voltage level defines the digital value that can be used to represent
the analog input voltage level.

Usually an ADC can operate in either differential or single-ended mode.
In differential mode two signals (V+ and V-) are compared against each other
and the resulting digital value represents the relative voltage level between
V+ and V-. This means that if the input voltage level on V+ is lower than on
V- the digital value is negative, which also means that in differential
mode one bit is lost to the sign. In single-ended mode only V+ is compared
against the reference voltage, and the resulting digital value only can be
positive, but the full bit-range of the ADC can be used.

Usually multiple resolutions are supported by the ADC, lower resolution can
reduce the conversion time, but lose accuracy.

Some ADCs has a gain stage on the input lines which can be used to increase the
dynamic range. The default gain value is usually x1, which means that the
conversion range is from 0V to the reference voltage.
Applications can change the gain stage, to increase or reduce the conversion
range.

The window mode allows the conversion result to be compared to a set of
predefined threshold values. Applications can use callback function to monitor
if the conversion result exceeds predefined threshold value.

Usually multiple reference voltages are supported by the ADC, both internal and
external with difference voltage levels. The reference voltage have an impact
on the accuracy, and should be selected to cover the full range of the analog
input signal and never less than the expected maximum input voltage.

There are two conversion modes supported by ADC, single shot and free running.
In single shot mode the ADC only make one conversion when triggered by the
application, in free running mode it continues to make conversion from it
is triggered until it is stopped by the application. When window monitoring,
the ADC should be set to free running mode.

The ADC async driver use a channel map buffer to map the relation between the 
enabled channels number and the index of register channel descriptor. The index 
of channel map buffer is channel number, and the value of channel map buffer is
the index of register channel descriptor. For example, when register channel_1, 
channel_5, and channel_9 in sequence, the value of channel_map[1] is 0, the value 
of channel_map[5] is 1, the value of channel_map[9] is 2.

The ADC async driver use a ring buffer to store ADC sample data. When the ADC
raise the sample complete interrupt, a copy of the ADC sample register is stored
in the ring buffer at the next free location. This will happen regardless of if
the ADC is in one shot mode or in free running mode. When the ring buffer is
full, the next sample will overwrite the oldest sample in the ring buffer. The
size of the ring buffer is set by a macro in atmel_start.h called ADC_BUFFER_SIZE.

To read the samples from the ring buffer, the function adc_async_read is used.
This function reads the number of bytes asked for from the ring buffer, starting
from the oldest byte. If the number of bytes asked for are more than currently
available in the ring buffer, the number of available bytes are read. The
adc_async_read function will return the actual number of bytes read from the buffer
back to the caller. If the number of bytes asked for is less than the available
bytes in the ring buffer, the remaining bytes will be kept until a new call to
adc_async_read or it's overwritten because the ring buffer is full.

Note that the adc_async_read_channel function will always read bytes from the ring buffer,
and for samples > 8-bit the read length has to be power of two number.

Features
--------
* Initialization and de-initialization
* Single shot or free running conversion modes
* Start ADC conversion
* Callback on conversion done, error and monitor events
* Ring buffer

Applications
------------
* Measurement of internal sensor. E.g., MCU internal temperature sensor value.
* Measurement of external sensor. E.g., Temperature, humidity sensor value.
* Sampling and measurement of a signal. E.g., sinusoidal wave, square wave.

Dependencies
------------
* ADC hardware with result ready/conversion done, error and monitor interrupt.

Concurrency
-----------
N/A

Limitations
-----------
N/A

Knows issues and workarounds
----------------------------
N/A

