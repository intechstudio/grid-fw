clear
idf.py build
parttool.py --port $(ls /dev/ttyACM*) write_partition --partition-name=ota_0 --input "build/grid_fw.bin"
otatool.py -p $(ls /dev/ttyACM*) switch_ota_partition --slot 0