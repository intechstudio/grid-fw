#include "grid_swsr.h"

int grid_swsr_malloc(struct grid_swsr_t* swsr, int capacity) {

  if (capacity < 1) {
    return 1;
  }

  // Allocate an extra two bytes, as the read and write heads will
  // always point to empty locations within the allocated space
  capacity += 2;

  char* data = malloc(capacity);

  if (!data) {
    return 1;
  }

  swsr->capacity = capacity;
  swsr->data = data;

  grid_swsr_init(swsr);

  return 0;
}

void grid_swsr_free(struct grid_swsr_t* swsr) {

  free(swsr->data);

  swsr->data = NULL;
}

void grid_swsr_init(struct grid_swsr_t* swsr) {

  swsr->write = 0;
  swsr->read = swsr->capacity - 1;
}

size_t grid_swsr_size(struct grid_swsr_t* swsr) {

  if (swsr->write < swsr->read) {
    return swsr->capacity - 1 - (swsr->read - swsr->write);
  } else {
    return swsr->write - swsr->read - 1;
  }
}

bool grid_swsr_in_range_excl(struct grid_swsr_t* swsr, int a, int b, int x) {

  int min = 0;
  int max = swsr->capacity;

  assert(min < max);
  assert(min <= a && a < max);
  assert(min <= b && b < max);
  assert(min <= x && x < max);

  if (a <= b) {
    return a <= x && x < b;
  } else {
    return (min <= x && x < b) || (a <= x && x < max);
  }
}

bool grid_swsr_writable(struct grid_swsr_t* swsr, int size) {

  if (size > swsr->capacity - 2) {
    return false;
  }

  int next_write = (swsr->write + size) % swsr->capacity;
  return grid_swsr_in_range_excl(swsr, swsr->write, swsr->read, next_write);
}

bool grid_swsr_readable(struct grid_swsr_t* swsr, int size) {

  if (size > swsr->capacity - 2) {
    return false;
  }

  int next_read = (swsr->read + size) % swsr->capacity;
  return grid_swsr_in_range_excl(swsr, swsr->read, swsr->write, next_read);
}

void grid_swsr_write(struct grid_swsr_t* swsr, void* src, int size) {

  assert(grid_swsr_writable(swsr, size));

  int until_capa = swsr->capacity - swsr->write;
  bool wraps = size > until_capa;

  int starts[2] = {swsr->write, 0};
  int lengths[2] = {
      wraps ? until_capa : size,
      wraps ? size - until_capa : 0,
  };

  memcpy(&swsr->data[starts[0]], (char*)src, lengths[0]);
  memcpy(&swsr->data[starts[1]], &((char*)src)[lengths[0]], lengths[1]);

  swsr->write = (swsr->write + size) % swsr->capacity;
}

void grid_swsr_read(struct grid_swsr_t* swsr, void* dest, int size) {

  assert(grid_swsr_readable(swsr, size));

  int first = (swsr->read + 1) % swsr->capacity;

  int until_capa = swsr->capacity - first;
  bool wraps = size > until_capa;

  int starts[2] = {first, 0};
  int lengths[2] = {
      wraps ? until_capa : size,
      wraps ? size - until_capa : 0,
  };

  memcpy((char*)dest, &swsr->data[starts[0]], lengths[0]);
  memcpy(&((char*)dest)[lengths[0]], &swsr->data[starts[1]], lengths[1]);

  swsr->read = (swsr->read + size) % swsr->capacity;
}
