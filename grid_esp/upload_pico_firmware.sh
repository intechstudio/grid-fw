esptool.py --chip esp32s3 -p /dev/ttyACM0 --before=default_reset \
--after=no_reset write_flash --flash_mode dio --flash_size detect --flash_freq 80m \
0x450000 pico_main.bin \