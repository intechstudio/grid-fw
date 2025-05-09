#ifndef GRID_MATH_H
#define GRID_MATH_H

#include <stdint.h>

uint32_t clampu32(uint32_t x, uint32_t a, uint32_t b);
int32_t clampi32(int32_t x, int32_t a, int32_t b);
double clampf64(double x, double a, double b);

#endif /* GRID_MATH_H */
