#ifndef GRID_SWSR_H
#define GRID_SWSR_H

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// Single writer, single reader ring buffer
struct grid_swsr_t {

  // Allocated element count
  int capacity;

  // Write index
  int write;

  // Read index
  int read;

  // Buffer contents
  char* data;
};

int grid_swsr_malloc(struct grid_swsr_t* swsr, int capacity);
void grid_swsr_free(struct grid_swsr_t* swsr);
void grid_swsr_init(struct grid_swsr_t* swsr);
size_t grid_swsr_size(struct grid_swsr_t* swsr);
bool grid_swsr_writable(struct grid_swsr_t* swsr, int size);
bool grid_swsr_readable(struct grid_swsr_t* swsr, int size);
void grid_swsr_write(struct grid_swsr_t* swsr, void* src, int size);
void grid_swsr_read(struct grid_swsr_t* swsr, void* dest, int size);
void grid_swsr_copy(struct grid_swsr_t* src, struct grid_swsr_t* dest, size_t size);
int grid_swsr_cspn(struct grid_swsr_t* swsr, char reject);

#endif /* GRID_SWSR_H */
