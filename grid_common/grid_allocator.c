

#include "grid_allocator.h"
#include <stdio.h>
#include <stdlib.h>
#define malloc malloc
#define free free

// Function pointers to the original malloc and free functions

// Counters for allocations and total allocated memory size
static size_t allocation_count = 0;
static size_t total_allocated_size = 0;

// Function to print memory statistics
void print_memory_stats(char* file, int line) {
  printf("Memory statistics: %s:%d\n", file, line);
  printf("Total allocations: %d\n", allocation_count);
  printf("Total allocated memory size: %d bytes\n", total_allocated_size);
}

// Custom malloc function
void* my_malloc(size_t size, char* file, int line) {
  // Call the original malloc function
  void* ptr = malloc(size);

  print_memory_stats(file, line);

  if (ptr != NULL) {
    // Update counters
    allocation_count++;
    total_allocated_size += size;
  }
  return ptr;
}

// Custom free function
void my_free(void* ptr) {
  // Call the original free function
  free(ptr);
  // Update counters
  if (ptr != NULL) {
    allocation_count--;
    // You can't determine the size of the allocated memory
    // without additional tracking mechanism
  }
}
