#!/bin/sh
# Stream RTT (printf-over-SWD) from the RP2350 to this terminal.
# Requires the firmware built with pico_enable_stdio_rtt(main 1).

PORT=9090

# Prefer the RP2350-specific OpenOCD (RPi fork). In the Docker image this is
# `openocd-rp2350` (the Espressif `openocd` is a separate install); on the host
# fall back to plain `openocd`. Override with the OPENOCD env var.
OPENOCD="${OPENOCD:-$(command -v openocd-rp2350 || echo openocd)}"

# Clear OPENOCD_SCRIPTS (set by esp-idf for the Espressif OpenOCD) so the RP2350
# fork uses its OWN bundled scripts, not the Espressif rp2350.cfg (which expects
# an `rp2xxx` flash driver this fork doesn't have). No-op on the host.
# Start OpenOCD: attach, locate the RTT control block in SRAM, serve channel 0.
env -u OPENOCD_SCRIPTS "$OPENOCD" -f rp2350/rp2350-openocd.cfg \
	-c "init" \
	-c "rtt setup 0x20000000 0x80000 \"SEGGER RTT\"" \
	-c "rtt start" \
	-c "rtt server start $PORT 0" &
OCD_PID=$!
trap 'kill $OCD_PID 2>/dev/null' EXIT INT TERM

sleep 1
echo "--- RTT channel 0 on localhost:$PORT (Ctrl-C to quit) ---"

# Bridge the RTT TCP channel to this terminal with whatever client is present:
# nc on the host, socat in the Docker image, telnet as a last resort.
if command -v nc >/dev/null 2>&1; then
	nc localhost "$PORT"
elif command -v socat >/dev/null 2>&1; then
	socat - "TCP:localhost:$PORT"
else
	telnet localhost "$PORT"
fi
