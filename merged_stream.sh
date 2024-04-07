#!/bin/bash

# Define the baud rate
BAUD_RATE=2000000

# Define ANSI color escape codes
COLOR_0="\033[0;31m"    # Red color
COLOR_1="\033[0;32m"  # Green color
COLOR_2="\033[0;34m"   # Blue color
COLOR_3="\033[0;35m"   # Magenta color
COLOR_NONE="\033[0m"     # Reset color

# Function to generate microsecond resolution timestamp
timestamp() {
    local nanoseconds=$(date '+%N' | sed 's/^0*//')  # Remove leading zeros
    printf "%s.%06d" "$(date '+%T')" "$nanoseconds"
}

timestamp_ms() {
    local milliseconds=$(date '+%3N' | sed 's/^0*//')  # Remove leading zeros
    printf "%s.%03d" "$(date '+%T')" "$milliseconds"
}

# Function to append timestamp to received message
append_timestamp() {
    while read -r line; do
        printf "%s %s\n" "$(timestamp_ms)" "$line"
    done
}

# Replace /dev/ttyUSB0, /dev/ttyUSB1, etc. with your actual serial port names
socat -u /dev/ttyUSB0,raw,b${BAUD_RATE},echo=0 - | sed "s/^/$(printf ${COLOR_0})Port 1: $(printf ${COLOR_NONE})/" &
socat -u /dev/ttyUSB1,raw,b${BAUD_RATE},echo=0 - | sed "s/^/$(printf ${COLOR_1})Port 2: $(printf ${COLOR_NONE})/" &
socat -u /dev/ttyUSB2,raw,b${BAUD_RATE},echo=0 - | sed "s/^/$(printf ${COLOR_2})Port 3: $(printf ${COLOR_NONE})/" &
socat -u /dev/ttyUSB3,raw,b${BAUD_RATE},echo=0 - | sed "s/^/$(printf ${COLOR_3})Port 4: $(printf ${COLOR_NONE})/" &

# You can add more ports as needed

# Wait for user to interrupt script to close socat processes
echo "Press Ctrl+C to stop merging serial streams..."
trap "pkill -f 'socat -u'; exit" INT
wait
