
mkdir ./output

python3 ./tools/uf2conv.py -f ESP32S3 ./build/grid_fw.bin -b 0x0 -c -o ./output/grid_fw.uf2


