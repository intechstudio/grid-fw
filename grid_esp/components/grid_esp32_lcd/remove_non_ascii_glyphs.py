import subprocess
import sys

# Define the range of ASCII character codes
ASCII_RANGE = range(0x00, 0x80)

# Check for command-line arguments
if len(sys.argv) != 3:
    print("Usage: python script.py <input_font_path> <output_font_path>")
    sys.exit(1)

input_path = sys.argv[1]
output_path = sys.argv[2]

# FontForge script as a string
fontforge_script = f"""
import fontforge

font = fontforge.open("{input_path}")
ASCII_RANGE = range(0x00, 0x80)

for glyph in font.glyphs():
    if glyph.unicode == -1 or glyph.unicode not in ASCII_RANGE:
        font.removeGlyph(glyph)
    else:
        # Simplify the glyph to reduce size
        glyph.simplify()
        glyph.round()

font.generate("{output_path}")
font.close()
"""

# Call FontForge with the script
try:
    subprocess.run(["fontforge", "-lang=py", "-c", fontforge_script], check=True)
    print("Non-ASCII glyphs removed. Saved to:", output_path)
except subprocess.CalledProcessError as e:
    print("Error running FontForge:", e)
    sys.exit(1)
