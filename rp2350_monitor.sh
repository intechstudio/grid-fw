#!/bin/sh
# Stream RTT (printf-over-SWD) from the RP2350 to this terminal.
# Requires the firmware built with pico_enable_stdio_rtt(main 1).

PORT=9090

# Start OpenOCD: attach, locate the RTT control block in SRAM, serve channel 0.
openocd -f rp2350/rp2350-openocd.cfg \
	-c "init" \
	-c "rtt setup 0x20000000 0x80000 \"SEGGER RTT\"" \
	-c "rtt start" \
	-c "rtt server start $PORT 0" &
OCD_PID=$!
trap 'kill $OCD_PID 2>/dev/null' EXIT INT TERM

sleep 1
echo "--- RTT channel 0 on localhost:$PORT (Ctrl-C to quit) ---"
nc localhost $PORT
