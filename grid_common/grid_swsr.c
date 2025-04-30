#include "grid_swsr.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

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

  if (size < 0 || size > swsr->capacity - 2) {
    return false;
  }

  int next_write = (swsr->write + size) % swsr->capacity;
  return grid_swsr_in_range_excl(swsr, swsr->write, swsr->read, next_write);
}

bool grid_swsr_readable(struct grid_swsr_t* swsr, int size) {

  if (size < 0 || size > swsr->capacity - 2) {
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

  if (!dest) {
    swsr->read = (swsr->read + size) % swsr->capacity;
    return;
  }

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

void grid_swsr_copy(struct grid_swsr_t* src, struct grid_swsr_t* dest, size_t size) {

  assert(grid_swsr_readable(src, size));
  assert(grid_swsr_writable(dest, size));

  struct grid_swsr_t* swsr = src;

  int first = (swsr->read + 1) % swsr->capacity;

  int until_capa = swsr->capacity - first;
  bool wraps = size > until_capa;

  int starts[2] = {first, 0};
  int lengths[2] = {
      wraps ? until_capa : size,
      wraps ? size - until_capa : 0,
  };

  grid_swsr_write(dest, &swsr->data[starts[0]], lengths[0]);
  grid_swsr_write(dest, &swsr->data[starts[1]], lengths[1]);
}

int grid_swsr_cspn(struct grid_swsr_t* swsr, char reject) {

  int i = 0;

  int first = (swsr->read + 1) % swsr->capacity;
  int j = first + 0;

  int write = swsr->write;
  while (j != write && swsr->data[j] != reject) {
    j = ((++i) + first) % swsr->capacity;
  }

  bool found = j != write && swsr->data[j] == reject;

  return found ? i : -1;
}

int grid_uwsr_malloc(struct grid_uwsr_t* uwsr, int capacity, char reject) {

  if (capacity < 1) {
    return 1;
  }

  char* data = malloc(capacity);

  if (!data) {
    return 1;
  }

  uwsr->capacity = capacity;
  uwsr->data = data;

  grid_uwsr_init(uwsr, reject);

  return 0;
}

void grid_uwsr_free(struct grid_uwsr_t* uwsr) {

  free(uwsr->data);

  uwsr->data = NULL;
}

void grid_uwsr_init(struct grid_uwsr_t* uwsr, char reject) {

  assert(reject != 0);

  uwsr->read = uwsr->capacity - 1;
  uwsr->seek = 0;
  uwsr->reject = reject;

  memset(uwsr->data, 0, uwsr->capacity);
}

int grid_uwsr_cspn(struct grid_uwsr_t* uwsr) {

  int first = uwsr->read + 1;
  int j = (first + uwsr->seek) % uwsr->capacity;

  int read = uwsr->read;
  while (j != read && uwsr->data[j] != uwsr->reject && uwsr->data[j]) {
    j = (first + (++uwsr->seek)) % uwsr->capacity;
  }

  bool found = j != read && uwsr->data[j] == uwsr->reject;

  return found ? uwsr->seek : -1;
}

bool grid_uwsr_readable(struct grid_uwsr_t* uwsr, int size) {

  if (size < 0 || size > uwsr->capacity) {
    return false;
  }

  return true;
}

void grid_uwsr_read(struct grid_uwsr_t* uwsr, void* dest, int size) {

  assert(grid_uwsr_readable(uwsr, size));

  int first = (uwsr->read + 1) % uwsr->capacity;

  int until_capa = uwsr->capacity - first;
  bool wraps = size > until_capa;

  int starts[2] = {first, 0};
  int lengths[2] = {
      wraps ? until_capa : size,
      wraps ? size - until_capa : 0,
  };

  if (dest) {
    memcpy((char*)dest, &uwsr->data[starts[0]], lengths[0]);
    memcpy(&((char*)dest)[lengths[0]], &uwsr->data[starts[1]], lengths[1]);
  }

  memset(&uwsr->data[starts[0]], 0, lengths[0]);
  memset(&uwsr->data[starts[1]], 0, lengths[1]);

  uwsr->read = (uwsr->read + size) % uwsr->capacity;
  uwsr->seek = 0;
}

bool grid_uwsr_overflow(struct grid_uwsr_t* uwsr) { return uwsr->data[uwsr->read] != 0; }
