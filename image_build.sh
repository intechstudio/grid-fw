#!/bin/sh

# Generates the embedded test images used by the LCD image decoder
# (esp32s3/components/grid_esp32_lcd/grid_image.c). Run this before building
# any target that includes that component, same as lua_build.sh must run
# before building anything that includes the Lua sources. Output is
# build-only and must never be committed (see esp32s3/.gitignore).

set -e

LCD_DIR="esp32s3/components/grid_esp32_lcd"

python3 "$LCD_DIR/gen_test_images.py"

# generate_image_array.sh feeds its paths straight into `xxd -i`, which bakes
# the entire given path into the emitted C variable name (e.g. a/b/c.bmp ->
# a_b_c_bmp) — so it must run with cwd == $LCD_DIR, same as when invoked by
# hand, to reproduce the exact names grid_image.c declares via extern
# (generated_images_test_bmp, etc.). Running it with repo-root-relative
# paths instead would silently break the build with undefined references.
(
  cd "$LCD_DIR"
  ./generate_image_array.sh source_images/test.bmp generated_images/test.bmp generated_images/test_bmp.c
  ./generate_image_array.sh source_images/test.png generated_images/test.png generated_images/test_png.c
  ./generate_image_array.sh source_images/test.jpg generated_images/test.jpg generated_images/test_jpg.c
)
