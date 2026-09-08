import os

from PIL import Image, ImageDraw

W, H = 320, 240

OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "source_images")

img = Image.new("RGB", (W, H))
px = img.load()

# Horizontal color gradient background (R across x, B across y) so a decoder
# bug shows up as a visibly wrong gradient direction/banding.
for y in range(H):
    for x in range(W):
        r = int(255 * x / (W - 1))
        g = int(255 * y / (H - 1))
        b = int(255 * (1 - x / (W - 1)) * (1 - y / (H - 1)))
        px[x, y] = (r, g, b)

draw = ImageDraw.Draw(img)

# Pure-channel corner swatches: quick visual check that R/G/B/W map correctly
# (catches channel-order bugs, e.g. RGB vs BGR).
sw = 24
draw.rectangle([0, 0, sw, sw], fill=(255, 0, 0))
draw.rectangle([W - sw, 0, W, sw], fill=(0, 255, 0))
draw.rectangle([0, H - sw, sw, H], fill=(0, 0, 255))
draw.rectangle([W - sw, H - sw, W, H], fill=(255, 255, 255))

# Black/white checkerboard strip: exercises high-frequency detail, useful for
# spotting JPEG block artifacts / chroma subsampling and off-by-one blits.
cb_y0, cb_h, cell = H // 2 - 12, 24, 6
for cx in range(0, W, cell):
    color = (0, 0, 0) if (cx // cell) % 2 == 0 else (255, 255, 255)
    draw.rectangle([cx, cb_y0, cx + cell - 1, cb_y0 + cb_h - 1], fill=color)

# 1px border so cropping/off-by-one errors during blit are obvious.
draw.rectangle([0, 0, W - 1, H - 1], outline=(255, 255, 0))

os.makedirs(OUT_DIR, exist_ok=True)
img.save(os.path.join(OUT_DIR, "test.bmp"), format="BMP")
img.save(os.path.join(OUT_DIR, "test.png"), format="PNG")
img.save(os.path.join(OUT_DIR, "test.jpg"), format="JPEG", quality=90)

print("wrote test.bmp, test.png, test.jpg (%dx%d RGB) to %s" % (W, H, OUT_DIR))
