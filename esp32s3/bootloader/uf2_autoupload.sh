#!/bin/bash


# Get the current username
USERNAME=$(whoami)

# Directory to check
CHECK_DIR="/media/${USERNAME}/GRID-S3"

# Source file to copy
SOURCE_FILE="./grid_esp32_release_2024-04-23-1133.uf2"

while true; do
    # Check if the directory exists
    if [ -d "$CHECK_DIR" ]; then
        # Check if the directory is writable
        if [ -w "$CHECK_DIR" ]; then
            # Copy the file to the directory
            echo "Upload Started"
            cp "$SOURCE_FILE" "$CHECK_DIR"
        fi
    fi

    # Wait before the next check
    sleep 1
done