#ifndef GRID_ALLOCATOR_H_INCLUDED
#define GRID_ALLOCATOR_H_INCLUDED

#pragma once

#include <stdlib.h>

#define malloc(x) my_malloc(x, __FILE__, __LINE__)
#define free(x) my_free(x)

void* my_malloc(size_t size, char* file, int line);
void my_free(void* ptr);

#endif
