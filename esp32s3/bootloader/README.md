## Seting up the toolchain
- Install ESP-IDF on your computer. (esptool.py is the only tool needed)

## Install the test firmware
- Run burn_test_firmware.sh directly or the contents of it manually
- After the programming, press the reset button manually

## Test procedure for the user interface
- All LED on the board should briefly flash green and then turn blue after successful booting of the device
- Interacting with the control elements on the board should change the intensity on the corresponding LED.
- Rotating to the left increases intensity, rotating to the right decreeses the intensity.
- Pressing the rotary encoder produces full brightness on the corresponting LED.
- Pressing the side-mounted button cycles throught the 4 configuration banks of the device.
- Configuration banks are color coded: Blue (default), Orange, Green and Purple

## Test procedure for the USB connection and the bootloader
- Unplug power from the device
- Press and hold the side-mounted button and then plugh in usb cable for the device
- All LED on the board should briefly turn red and then turn green after successful enumeration of the USB driver
- On the host computer the device shows up as a removable mass storage device with the drive label of GRID-S3
- Check and verify that three files are available on the device called CURRENT.UF2, INDEX.HTM and INFO_UF2.TXT
- Repeat the test procedure with reversed orientation on the USB-C connector to verify the soldering of the USB connector
