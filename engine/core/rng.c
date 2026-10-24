#include "rng.h"

void rng_seed(rng_t* rng, uint32_t seed) {
  rng->state = seed ? seed : 0x9e3779b9u;
  for (int i = 0; i < 4; i++) rng_next(rng);
}

uint32_t rng_next(rng_t* rng) {
  uint32_t x = rng->state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  rng->state = x;
  return x;
}

float rng_float(rng_t* rng) {
  return (float)(rng_next(rng) >> 8) / 16777216.0f;
}

int rng_range(rng_t* rng, int low, int high) {
  if (high <= low) return low;
  return low + (int)(rng_next(rng) % (uint32_t)(high - low + 1));
}

int rng_chance(rng_t* rng, float probability) {
  return rng_float(rng) < probability;
}
