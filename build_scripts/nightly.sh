set -e  #error van akkor kilepo

(cd ./../grid_make/gcc && make)


python3 ./macro2json.py ./../grid_make/grid/grid_protocol.h ./../../grid-protocol/grid_protocol_nightly.json


python3 ./uf2conv.py -c -b 0x4000 -o ./../binary/grid_nightly.uf2 ./../grid_make/gcc/AtmelStart.bin


cp ./../binary/grid_nightly.uf2 /media/$(whoami)/GRID
