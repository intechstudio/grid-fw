======================
CRC Synchronous driver
======================

A CRC (Cyclic Redundancy Check) is a side application for detecting errors to
raw data. It take a data block of any length, and give out a fixed length check
value. CRC can not make correction when error detected.

In the CRC method, a fixed length check value (often called Checksum) is
appended to the transmit data block. The receiver can use same CRC function
to check whether the checksum is match the received data. If received checksum
not match the computation checksum, that mean something wrong when transfer the
data, the receiver can request the data to be send again.

The driver supports IEEE CRC32 polynomial and CCITT CRC16 polynomial.
The initial value used for the CRC calculation must be assigned. This value
is usually 0xFFFFFFFF(CRC32) or 0xFFFF(CRC16), but can be, for example, the
result of a previously computed CRC separate memory blocks.

Features
--------

* Initialization and de-initialization
* Enabling and Disabling
* CRC32(IEEE-802.3)
* CRC16(CCITT)

Applications
------------
* Calculate a checksum for data block to be sent and append it to the data.
  When data block with checksum received by the receiver, the receive checksum
  can be compared with new calculated checksum from the data block.

Dependencies
------------
* CRC capable hardware

Concurrency
-----------
N/A

Limitations
-----------
* CRC use hardware DSU engine only support CRC32(IEEE-802.3) reversed
  polynomial representation.

Known issues and workarounds
----------------------------
N/A

