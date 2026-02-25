PORT=${PORT:-/dev/ttyUSB0}
idf.py -C "./grid_esp" -b 2000000 -p $PORT monitor
