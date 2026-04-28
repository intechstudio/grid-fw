SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
SRC_DIR=esp32s3
PORT=${PORT:-/dev/ttyUSB0}
OTA=${OTA:-yes}

NEW_BIN=$SCRIPT_DIR/"$SRC_DIR"/build/grid_"$SRC_DIR".bin
OLD_BIN=$SCRIPT_DIR/"$SRC_DIR"/build/old_grid_"$SRC_DIR".bin
DIFF_ARG=""
if [ -f "$OLD_BIN" ]; then
	DIFF_ARG="--diff-with $OLD_BIN"
fi

esptool --chip esp32s3 -p "$PORT" -b 2000000 --before=default-reset --after=no-reset write-flash --flash-mode dio --flash-size detect --flash-freq 80m 0x10000 "$NEW_BIN" $DIFF_ARG

cp "$NEW_BIN" "$OLD_BIN"

if [ "$OTA" = "yes" ]; then
	otatool.py -p "$PORT" switch_ota_partition --slot 0
fi
