xxd -i ../grid_pico/build/main/main.bin > ./main/pico_firmware.h
sed -i '1i\const \\' ./main/pico_firmware.h
