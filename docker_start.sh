#!/bin/bash

# Check if Docker is installed
if command -v docker &> /dev/null; then
    CONTAINER_TOOL="docker"
# Check if Podman is installed
elif command -v podman &> /dev/null; then
    CONTAINER_TOOL="podman"
else
    echo "Neither Docker nor Podman found. Please install one of them to proceed."
    exit 1
fi

# Check if an argument is provided
if [ -n "$1" ]; then
    $CONTAINER_TOOL run --privileged -it -v /dev:/dev -v $PWD:/project -w /project/ idf-pico-merged sh -c "./build_firmware.sh"
else
    $CONTAINER_TOOL run --privileged -it -v /dev:/dev -v $PWD:/project -w /project/ idf-pico-merged
fi
