#!/bin/sh

SRC_DIR=esp32s3

if ! idf.py -C "$SRC_DIR" build ; then
	exit 1
fi

python3 tools/uf2conv/uf2conv.py -f ESP32S3 "$SRC_DIR"/build/grid_fw.bin -b 0x0 -c -o "$SRC_DIR"/build/grid_fw.uf2
