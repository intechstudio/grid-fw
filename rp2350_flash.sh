#!/bin/sh

openocd -f rp2350/rp2350-openocd.cfg -c "program rp2350/build/main/main.elf verify reset exit"
