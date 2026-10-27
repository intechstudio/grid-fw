#!/bin/sh

PORT=9090

# Prefer the fork of openocd specific to RPi Pico boards by default
OPENOCD="${OPENOCD:-$(command -v openocd-rp2350 || echo openocd)}"

# Enable job control so that all processes run in a separate process group
set -m

# Clear $OPENOCD_SCRIPTS to make the RPi fork use its own scripts instead of
# the scripts set by the ESP fork due to it needing a missing flash driver.
env -u OPENOCD_SCRIPTS "$OPENOCD" -f rp2350/rp2350-openocd.cfg \
	-c "init" \
	-c "rtt setup 0x20000000 0x1000 \"SEGGER RTT\"" \
	-c "rtt start" \
	-c "rtt server start $PORT 0" &

OCD_PID=$!
trap 'kill -TERM -$OCD_PID' EXIT INT TERM

sleep 1

socat - "TCP:localhost:$PORT"
