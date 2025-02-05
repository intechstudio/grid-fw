#!/bin/sh

DEVICE="/dev/ttyUSB1"

../../../../vmp/build/recv -i "${DEVICE}" | ./build/proc
