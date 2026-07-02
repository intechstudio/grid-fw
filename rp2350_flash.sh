#!/bin/sh

# Prefer the RP2350-specific OpenOCD (RPi fork). In the Docker image this is
# `openocd-rp2350` (the Espressif `openocd` is a separate install); on the host
# fall back to plain `openocd`. Override with the OPENOCD env var.
OPENOCD="${OPENOCD:-$(command -v openocd-rp2350 || echo openocd)}"

# Clear OPENOCD_SCRIPTS (set by esp-idf for the Espressif OpenOCD) so the RP2350
# fork uses its OWN bundled scripts, not the Espressif rp2350.cfg (which expects
# an `rp2xxx` flash driver this fork doesn't have). No-op on the host.
env -u OPENOCD_SCRIPTS "$OPENOCD" -f rp2350/rp2350-openocd.cfg -c "program rp2350/build/main/main.elf verify reset exit"
