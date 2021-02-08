==============================
I2C Master asynchronous driver
==============================

I2C (Inter-Integrated Circuit) is a two wire serial interface usually used
for on-board low-speed bi-directional communication between controllers and
peripherals. The master device is responsible for initiating and controlling
all transfers on the I2C bus. Only one master device can be active on the I2C
bus at the time, but the master role can be transferred between devices on the
same I2C bus. I2C uses only two bidirectional open-drain lines, usually
designated SDA (Serial Data Line) and SCL (Serial Clock Line), with pull up
resistors.

The stop condition is automatically controlled by the driver if the I/O write and
read functions are used, but can be manually controlled by using the
i2c_m_async_transfer function.

Often a master accesses different information in the slave by accessing
different registers in the slave. This is done by first sending a message to
the target slave containing the register address, followed by a repeated start
condition (no stop condition between) ending with transferring register data.
This scheme is supported by the i2c_m_async_cmd_write and i2c_m_async_cmd_read
function, but limited to 8-bit register addresses.

Transfer callbacks are executed at the end of a full transfer, that is when
a complete message with address is either sent or read from the slave. When the
i2c_m_async_transfer function is used the tx and rx callbacks are triggered
regardless of if a stop condition is generated at the end of the message.

The tx and rx callbacks are reused for the cmd functions and are triggered at
the end of a full register write or read, that is after the register address has
been written to the slave and data has been transferred to or from the master.

The error callback is executed as soon as an error is detected, the error
situation can both be detected while processing an interrupt or detected by
the hardware which may trigger a special error interrupt. In situations where
errors are detected by software, there will be a slight delay from the error
occurs until the error callback is triggered due to software processing.

I2C Modes (standard mode/fastmode+/highspeed mode) can only be selected in
Atmel Start. If the SCL frequency (baudrate) has changed run-time, make sure to
stick within the SCL clock frequency range supported by the selected mode.
The requested SCL clock frequency is not validated by the
i2c_m_async_set_baudrate function against the selected I2C mode.

Features
--------

	* I2C Master support
	* Initialization and de-initialization
	* Enabling and disabling
	* Run-time bus speed configuration
	* Write and read I2C messages
	* Slave register access functions (limited to 8-bit address)
	* Manual or automatic stop condition generation
	* 10- and 7- bit addressing
	* I2C Modes supported
	       +----------------------+-------------------+
	       |* Standard/Fast mode  | (SCL: 1 - 400kHz) |
	       +----------------------+-------------------+
	       |* Fastmode+           | (SCL: 1 - 1000kHz)|
	       +----------------------+-------------------+
	       |* Highspeed mode      | (SCL: 1 - 3400kHz)|
	       +----------------------+-------------------+
	* Callback on buffer transfer complete and error events

Applications
------------

* Transfer data to and from one or multiple I2C slaves like I2C connected sensors, data storage or other I2C capable peripherals
* Data communication between micro controllers
* Controlling displays

Dependencies
------------

* I2C Master capable hardware with char transfer complete and error interrupt

Concurrency
-----------

N/A

Limitations
-----------

General
^^^^^^^

	* System Management Bus (SMBus) not supported
	* Power Management Bus (PMBus) not supported
	* Buffer content should not be changed before transfer is complete
	* Due to software processing some errors may have a detection delay.

Clock considerations
^^^^^^^^^^^^^^^^^^^^

The register value for the requested I2C speed is calculated and placed in the
correct register, but not validated if it works correctly with the
clock/prescaler settings used for the module. To validate the I2C speed
setting use the formula found in the configuration file for the module.
Selectable speed is automatically limited within the speed range defined by
the I2C mode selected.

Known issues and workarounds
----------------------------

N/A


