The Quad SPI DMA Driver
=================================

The Quad SPI Interface (QSPI) is a synchronous serial data link that provides
communication with external devices in master mode.

The QSPI DMA driver uses DMA system to transfer data between QSPI Memory region
and external device. User must configure DMAC system driver accordingly. Callback 
function is called when all the data is transferred or transfer error occurred, 
if it is registered via qspi_dma_register_callback() function.


Features
--------

* Initialization/de-initialization
* Enabling/disabling
* Register callback function
* Execute command in Serial Memory Mode

Applications
------------

They are commonly used in an application for using serial flash memory operating
in single-bit SPI, Dual SPI and Quad SPI. 

Dependencies
------------

Serial NOR flash with Multiple I/O hardware

Concurrency
-----------

N/A

Limitations
-----------

N.A

Known issues and workarounds
----------------------------

N/A

