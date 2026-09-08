#!/bin/bash

# Embeds an image file as a C byte array, same approach as remove_non_ascii.sh
# uses for fonts (xxd -i), minus the glyph-subsetting step.
# Example usage: ./generate_image_array.sh source_images/test.bmp generated_images/test.bmp generated_images/test_bmp.c
if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <input_image_path> <output_image_path> <output_source_path>"
    exit 1
fi

INPUT_PATH=$1
OUTPUT_PATH=$2
OUTPUT_SRC=$3

mkdir -p "$(dirname "$OUTPUT_PATH")" "$(dirname "$OUTPUT_SRC")"
cp "$INPUT_PATH" "$OUTPUT_PATH"
xxd -i "$OUTPUT_PATH" "$OUTPUT_SRC"
sed -i '1i\const \' "$OUTPUT_SRC"

echo "Embedded: $OUTPUT_SRC"
