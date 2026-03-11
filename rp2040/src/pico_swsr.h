#ifndef PICO_SWSR_H
#define PICO_SWSR_H

#include <stdbool.h>
#include <stdlib.h>

// Single writer, single reader ring buffer
struct pico_swsr_t {

  // Allocated element count
  int capacity;

  // Write index
  int write;

  // Read index
  int read;

  // Buffer contents
  char* data;
};

int pico_swsr_malloc(struct pico_swsr_t* swsr, int capacity);
void pico_swsr_free(struct pico_swsr_t* swsr);
void pico_swsr_init(struct pico_swsr_t* swsr);
bool pico_swsr_writable(struct pico_swsr_t* swsr);
bool pico_swsr_readable(struct pico_swsr_t* swsr);
void pico_swsr_write(struct pico_swsr_t* swsr, char c);
char pico_swsr_read(struct pico_swsr_t* swsr);

#endif /* PICO_SWSR_H */
