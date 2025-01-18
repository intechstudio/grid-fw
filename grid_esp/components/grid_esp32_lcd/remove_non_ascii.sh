#!/bin/bash

# Check if the correct number of arguments is provided
echo "Prerequisites: sudo apt install fontforge python3-fontforge"
if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <input_font_path> <output_font_path> <output_header_path>"
    exit 1
fi

INPUT_PATH=$1
OUTPUT_PATH=$2

# Run the FontForge Python script
fontforge -script remove_non_ascii_glyphs.py "$INPUT_PATH" "$OUTPUT_PATH"

# Check if the operation was successful
if [ $? -eq 0 ]; then
    echo "Non-ASCII glyphs removed. Saved to: $OUTPUT_PATH"
    xxd -i $2 $3
else
    echo "An error occurred while processing the font."
    exit 1
fi
