idf.py -C "./grid_esp" build

mkdir -p ./binary

python3 ./grid_esp/tools/uf2conv.py -f ESP32S3 ./grid_esp/build/grid_fw.bin -b 0x0 -c -o ./binary/grid_fw.uf2
