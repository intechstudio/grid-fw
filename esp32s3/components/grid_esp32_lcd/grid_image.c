#include "grid_image.h"

#ifndef __EMSCRIPTEN__
#include "esp_heap_caps.h"
#define STBI_MALLOC(sz) heap_caps_malloc(sz, MALLOC_CAP_SPIRAM)
#define STBI_REALLOC(p, newsz) heap_caps_realloc(p, newsz, MALLOC_CAP_SPIRAM)
#define STBI_FREE(p) heap_caps_free(p)
#endif

// Only the formats we actually embed need decoding, which keeps the
// generated decoder (and its flash footprint) limited to BMP/PNG/JPEG.
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_NO_TGA
#define STBI_NO_PSD
#define STBI_NO_GIF

#define STB_IMAGE_IMPLEMENTATION // force following include to generate implementation
#include "stb_image.h"

extern const unsigned char generated_images_test_bmp[];
extern unsigned int generated_images_test_bmp_len;

extern const unsigned char generated_images_test_png[];
extern unsigned int generated_images_test_png_len;

extern const unsigned char generated_images_test_jpg[];
extern unsigned int generated_images_test_jpg_len;

struct grid_image_table_entry {
  const unsigned char* data;
  const unsigned int* size;
};

static struct grid_image_table_entry grid_image_table[GRID_IMAGE_COUNT] = {
    {.data = generated_images_test_bmp, .size = &generated_images_test_bmp_len},
    {.data = generated_images_test_png, .size = &generated_images_test_png_len},
    {.data = generated_images_test_jpg, .size = &generated_images_test_jpg_len},
};

int grid_image_draw(struct grid_gui_model* gui, uint8_t image_id, uint16_t x, uint16_t y) {

  if (gui == NULL || image_id >= GRID_IMAGE_COUNT) {
    return 1;
  }

  struct grid_image_table_entry* entry = &grid_image_table[image_id];

  int w, h, channels;
  unsigned char* pixels = stbi_load_from_memory(entry->data, (int)*entry->size, &w, &h, &channels, STBI_rgb);

  if (pixels == NULL) {
    return 1;
  }

  // Clip against the destination bounds using wide (int) arithmetic before
  // any coordinate is narrowed to uint16_t, so a large x/y cannot wrap
  // around into the visible area instead of being clipped off-screen.
  int x0 = (int)x;
  int y0 = (int)y;
  int x1 = x0 + w;
  int y1 = y0 + h;

  int clip_x0 = x0 < 0 ? 0 : x0;
  int clip_y0 = y0 < 0 ? 0 : y0;
  int clip_x1 = x1 > (int)gui->width ? (int)gui->width : x1;
  int clip_y1 = y1 > (int)gui->height ? (int)gui->height : y1;

  for (int dy = clip_y0; dy < clip_y1; ++dy) {
    for (int dx = clip_x0; dx < clip_x1; ++dx) {

      const unsigned char* src = &pixels[((dy - y0) * w + (dx - x0)) * STBI_rgb];
      uint8_t* dst = gui->buffer + ((size_t)gui->height * dx + dy) * GRID_GUI_BYTES_PPX;

      // Decoded image pixels are always fully opaque, so write them
      // directly instead of going through the generic alpha-blending
      // pixel draw path used for shapes/text.
      dst[0] = src[0];
      dst[1] = src[1];
      dst[2] = src[2];
    }
  }

  stbi_image_free(pixels);

  return 0;
}
