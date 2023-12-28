# otatool.py -p /dev/ttyACM0 write_ota_partition --slot 0 --input build/grid_fw.bin
# otatool.py -p /dev/ttyACM0 switch_ota_partition --slot 0
esptool.py --chip esp32s3 -p $(ls /dev/ttyACM*) --before=default_reset --after=no_reset write_flash --flash_mode dio --flash_size detect --flash_freq 80m 0x10000 build/grid_fw.bin && otatool.py -p $(ls /dev/ttyACM*) switch_ota_partition --slot 0
