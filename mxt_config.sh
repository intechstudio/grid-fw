#!/bin/sh

SRCDIR=./common/src/mxt
DESTDIR=./common/build/mxt

mkdir -p "$DESTDIR"

if ! xxd -i "$SRCDIR"/mxt144u_cfg.raw > "$DESTDIR"/mxt144u_cfg.h ; then
	exit 1
fi

sed -i "1i\const \\" "$DESTDIR"/mxt144u_cfg.h
