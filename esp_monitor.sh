PORT=${PORT:-/dev/ttyUSB0}
idf.py -C esp32s3 -b 2000000 -p $PORT monitor
