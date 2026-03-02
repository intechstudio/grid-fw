PORT=${PORT:-/dev/ttyUSB0}
kill $(lsof -t $PORT)
