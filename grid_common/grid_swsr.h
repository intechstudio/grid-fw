#ifndef GRID_SWSR_H
#define GRID_SWSR_H

#include <stdbool.h>
#include <stddef.h>

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

// Unknown writer, single reader ring buffer
struct grid_uwsr_t {

  // Allocated element count
  int capacity;

  // Read index
  int read;

  // Buffer contents
  char* data;
};

int grid_uwsr_malloc(struct grid_uwsr_t* uwsr, int capacity);
void grid_uwsr_free(struct grid_uwsr_t* uwsr);
void grid_uwsr_init(struct grid_uwsr_t* uwsr);
int grid_uwsr_cspn(struct grid_uwsr_t* uwsr, char reject);
bool grid_uwsr_readable(struct grid_uwsr_t* uwsr, int size);
void grid_uwsr_read(struct grid_uwsr_t* uwsr, void* dest, int size);

#endif /* GRID_SWSR_H */
