SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
SRC_DIR=esp32s3
PORT=${PORT:-/dev/ttyUSB0}
OTA=${OTA:-yes}

esptool --chip esp32s3 -p "$PORT" -b 2000000 --before=default-reset --after=no-reset write-flash --flash-mode dio --flash-size detect --flash-freq 80m 0x10000 $SCRIPT_DIR/"$SRC_DIR"/build/grid_"$SRC_DIR".bin --diff-with $SCRIPT_DIR/"$SRC_DIR"/build/old_grid_"$SRC_DIR".bin

if [ "$OTA" = "yes" ]; then
	otatool.py -p "$PORT" switch_ota_partition --slot 0
fi
