mkdir -p ./grid_pico/build

rm -r ./grid_pico/build/main/main.*
rm ./grid_esp/main/pico_firmware.h

cmake -S "./grid_pico" -B "./grid_pico/build"
make -C "./grid_pico/build"
xxd -i ./grid_pico/build/main/main.bin > ./grid_pico/build/main/pico_firmware.h
sed -i '1i\const \\' ./grid_pico/build/main/pico_firmware.h
sed -i 's/__grid_pico_build_main_main_bin/pico_firmware/g' ./grid_pico/build/main/pico_firmware.h
cp ./grid_pico/build/main/pico_firmware.h ./grid_esp/main/pico_firmware.h
