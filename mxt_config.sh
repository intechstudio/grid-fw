#!/bin/sh

SRCDIR=esp32s3/components/grid_esp32_touch

cd "$SRCDIR" || exit 1

if ! xxd -i ./mxt144u_cfg.raw > ./mxt144u_cfg.h ; then
	exit 1
fi
