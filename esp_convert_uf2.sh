
mkdir -p ./grid_esp/output

python3 ./grid_esp/tools/uf2conv.py -f ESP32S3 ./grid_esp/build/grid_fw.bin -b 0x0 -c -o ./grid_esp/output/grid_fw.uf2
