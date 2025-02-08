#!/bin/bash

# Get the directory from the argument
directory="grid_common/lua_src/"

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
        echo "#ifndef GRID_LUA_SRC_${base_name^^}_H"
        echo "#define GRID_LUA_SRC_${base_name^^}_H"
        echo ""
    } > "$header_file"
        # Use xxd to convert the .lua file to a C string literal and add a terminating zero byte
        (xxd -i "$lua_file") >> "$header_file"
    {
        # Close the header guard
        echo ""
        echo "#endif // GRID_LUA_SRC_${base_name^^}_H"
    } >> "$header_file"


    # Change variable type to const char
    sed -i 's/unsigned char/const char/' "$header_file"

    # Change variable name
    sed -i "s/grid_common_lua_src__${base_name}/grid_lua_src_${base_name}/" "$header_file"

    # Add terminating zero byte
    sed -i "s/};/, 0x00};/" "$header_file"

    # Remove newline before the terminating zero byte
    sed -i ':a;N;$!ba;s/\n,/,/g' "$header_file"

    # Add +1 to the thength integer
    sed -i ':a;N;$!ba;s/;\n\n/+1;\n\n/g' "$header_file"

    echo "Converted $lua_file to $header_file"
done
