/**
 * @file geometry.h
 * @brief 2D vectors and axis-aligned rectangles. Rectangles are top-left
 * origin with y growing downwards, like the screen.
 */
#ifndef ENGINE_MATH_GEOMETRY_H_
#define ENGINE_MATH_GEOMETRY_H_

#include <stdbool.h>

typedef struct {
  float x, y;
} vec2_t;

typedef struct {
  float x, y, w, h;
} rect_t;

static inline float clampf(float value, float low, float high) {
  return value < low ? low : (value > high ? high : value);
}

static inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }

static inline float approachf(float value, float target, float step) {
  if (value < target) return value + step > target ? target : value + step;
  return value - step < target ? target : value - step;
}

static inline bool rect_overlaps(rect_t a, rect_t b) {
  return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h &&
         b.y < a.y + a.h;
}

static inline float rect_right(rect_t r) { return r.x + r.w; }
static inline float rect_bottom(rect_t r) { return r.y + r.h; }

#endif  // ENGINE_MATH_GEOMETRY_H_
