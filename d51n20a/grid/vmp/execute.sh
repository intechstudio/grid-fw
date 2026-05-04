#!/bin/sh

DEVICE="/dev/ttyUSB1"

../../../common/dep/vmp/build/recv -i "${DEVICE}" | ./build/proc
