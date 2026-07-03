#!/bin/sh

if ! xxd -i "$1" > ./mxt144u_cfg.h ; then
	exit 1
fi
