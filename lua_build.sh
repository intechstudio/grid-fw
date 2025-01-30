#!/bin/bash

# Check if the directory is provided as an argument
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <directory>"
    exit 1
fi

# Get the directory from the argument
directory="$1"

# Check if the provided argument is a directory
if [ ! -d "$directory" ]; then
    echo "Error: $directory is not a valid directory."
    exit 1
fi

# Loop through all .lua files in the specified directory
for lua_file in "$directory"/*.lua; do
    # Check if there are no .lua files
    if [ ! -e "$lua_file" ]; then
        echo "No .lua files found in the directory."
        exit 0
    fi

    # Get the base name of the file (without the extension)
    base_name=$(basename "$lua_file" .lua)

    # Create the corresponding .h file
    header_file="$directory/$base_name.h"

    # Start writing to the header file
    {
        echo "#ifndef ${base_name^^}_H"
        echo "#define ${base_name^^}_H"
        echo ""
        echo "const char* ${base_name}_lua = "

        # Use xxd to convert the .lua file to a C string literal
        xxd -i "$lua_file" | sed 's/unsigned char/const char/' >> "$header_file"

        # Close the header guard
        echo ""
        echo "#endif // ${base_name^^}_H"
    } > "$header_file"

    echo "Converted $lua_file to $header_file"
done
