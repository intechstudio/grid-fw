#include "grid_math.h"

uint32_t clampu32(uint32_t x, uint32_t a, uint32_t b) {

  const uint32_t t = x < a ? a : x;
  return t > b ? b : t;
}

int32_t clampi32(int32_t x, int32_t a, int32_t b) {

  const int32_t t = x < a ? a : x;
  return t > b ? b : t;
}

double clampf64(double x, double a, double b) {

  const double t = x < a ? a : x;
  return t > b ? b : t;
}

int32_t mirrori32(int32_t x, int32_t a, int32_t b) {

  const uint32_t range = b - a;
  return (range - (x - a)) + a;
}
