#!/bin/sh

mkdir rp2040/build

rm rp2040/build/main/main.*

cmake -S rp2040 -B rp2040/build

if [ $? -ne 0 ] ; then
	exit 1
fi

make -C rp2040/build

if [ $? -ne 0 ] ; then
	exit 1
fi

xxd -i rp2040/build/main/main.bin > rp2040/build/main/pico_firmware.h

if [ $? -ne 0 ] ; then
	exit 1
fi

sed -i '1i\const \\' rp2040/build/main/pico_firmware.h

sed -i 's/rp2040_build_main_main_bin/pico_firmware/g' rp2040/build/main/pico_firmware.h
