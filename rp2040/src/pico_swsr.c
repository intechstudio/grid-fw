#include "pico_swsr.h"

int pico_swsr_malloc(struct pico_swsr_t* swsr, int capacity) {

  if (capacity < 1) {
    return 1;
  }

  char* data = malloc(capacity);

  if (!data) {
    return 1;
  }

  swsr->capacity = capacity;
  swsr->data = data;

  pico_swsr_init(swsr);

  return 0;
}

void pico_swsr_free(struct pico_swsr_t* swsr) {

  free(swsr->data);

  swsr->data = NULL;
}

void pico_swsr_init(struct pico_swsr_t* swsr) {

  swsr->write = 0;
  swsr->read = swsr->capacity - 1;
}

bool pico_swsr_writable(struct pico_swsr_t* swsr) { return (swsr->write + 1) % swsr->capacity != swsr->read; }

bool pico_swsr_readable(struct pico_swsr_t* swsr) { return (swsr->read + 1) % swsr->capacity != swsr->write; }

void pico_swsr_write(struct pico_swsr_t* swsr, char c) {

  swsr->data[swsr->write] = c;
  swsr->write = (swsr->write + 1) % swsr->capacity;
}

char pico_swsr_read(struct pico_swsr_t* swsr) {

  swsr->read = (swsr->read + 1) % swsr->capacity;
  return swsr->data[swsr->read];
}
