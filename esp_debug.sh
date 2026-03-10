SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
PORT=${PORT:-/dev/ttyUSB0}
OTA=${OTA:-yes}

esptool.py --chip esp32s3 -p $(ls $PORT | head -n 1) -b 2000000 --before=default_reset --after=no_reset write_flash --flash_mode dio --flash_size detect --flash_freq 80m 0x10000 $SCRIPT_DIR/grid_esp/build/grid_fw.bin

if [ "$OTA" = "yes" ]; then
	otatool.py -p $(ls $PORT | head -n 1) --esptool-args="after=no_reset" switch_ota_partition --slot 0
fi

idf.py -C "./grid_esp" -b 2000000 -p $(ls $PORT | head -n 1) monitor
