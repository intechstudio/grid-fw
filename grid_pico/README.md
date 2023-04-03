## Flash Programming Using PICO as PICOPROBE

- Start openocd in a new terminal
```sudo openocd -f interface/picoprobe.cfg -f target/rp2040.cfg -s tcl```
- Start telnet in a new terminal
```telnet localhost 4444```
- Uload the firmware to SRAM
```
reset
halt
load_image /home/suku/Documents/grid-fw/grid_pico/build/main/main.bin 0x20000000
verify_image /home/suku/Documents/grid-fw/grid_pico/build/main/main.bin 0x20000000
resume 0x20000000
```
