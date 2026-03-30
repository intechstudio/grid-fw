#include "grid_allocator.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "grid_platform.h"

size_t try_calloc(size_t blocks, size_t blksize) {

  // Allocating zero bytes is, in principle, always possible
  if (!blocks) {
    return 0;
  }

  void* test = calloc(blocks, blksize);

  if (!test) {
    return 0;
  }

  free(test);
  return blocks;
}

void find_max_allocatable(size_t blksize) {

  size_t blocks = 0;
  int success = 1;

  while (success) {

    success = try_calloc(blocks, blksize) == blocks;

    grid_platform_printf("blksize: %u, blocks: %u, total: %u success: %d\n", blksize, blocks, blksize * blocks, success);

    ++blocks;
  }
}
