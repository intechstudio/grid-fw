#!/bin/sh

SRC_DIR=esp32s3

NEW_BIN="$SRC_DIR"/build/grid_"$SRC_DIR".bin
OLD_BIN="$SRC_DIR"/build/old_grid_"$SRC_DIR".bin

if [ -f "$NEW_BIN" ] ; then
  cp "$NEW_BIN" "$OLD_BIN"
else
  truncate --size=0 "$OLD_BIN"
fi

if ! idf.py -C "$SRC_DIR" build ; then
	exit 1
fi

python3 tools/uf2conv/uf2conv.py -f ESP32S3 "$SRC_DIR"/build/grid_"$SRC_DIR".bin -b 0x0 -c -o "$SRC_DIR"/build/grid_"$SRC_DIR".uf2
