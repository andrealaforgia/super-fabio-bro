#include <math.h>

#include "world.h"

#define EPSILON 0.001f

static int cell(float v) { return (int)floorf(v / TILE); }

static bool column_blocked(const level_t* level, int col, float top,
                           float bottom) {
  for (int row = cell(top); row <= cell(bottom - EPSILON); row++)
    if (tile_is_solid(level_tile(level, col, row))) return true;
  return false;
}

static bool row_blocked(const level_t* level, int row, float left,
                        float right) {
  for (int col = cell(left); col <= cell(right - EPSILON); col++)
    if (tile_is_solid(level_tile(level, col, row))) return true;
  return false;
}

move_result_t physics_move(const level_t* level, float* x, float* y,
                           float* vx, float* vy, float w, float h, float dt) {
  move_result_t result = {false, false, false, false, 0, 0};
  float half = w * 0.5f;

  *x += *vx * dt;
  float top = *y - h, bottom = *y;
  if (*vx > 0.0f) {
    int col = cell(*x + half - EPSILON);
    if (column_blocked(level, col, top + EPSILON, bottom)) {
      *x = col * TILE - half - EPSILON;
      *vx = 0.0f;
      result.hit_right = true;
    }
  } else if (*vx < 0.0f) {
    int col = cell(*x - half);
    if (column_blocked(level, col, top + EPSILON, bottom)) {
      *x = (col + 1) * TILE + half + EPSILON;
      *vx = 0.0f;
      result.hit_left = true;
    }
  }

  *y += *vy * dt;
  float left = *x - half, right = *x + half;
  if (*vy > 0.0f) {
    int row = cell(*y - EPSILON);
    if (row_blocked(level, row, left + EPSILON, right - EPSILON)) {
      *y = (float)(row * TILE);
      *vy = 0.0f;
      result.grounded = true;
    }
  } else if (*vy < 0.0f) {
    int row = cell(*y - h);
    if (row_blocked(level, row, left + EPSILON, right - EPSILON)) {
      *y = (float)((row + 1) * TILE) + h + EPSILON;
      *vy = 0.0f;
      result.hit_ceiling = true;
      int centre = cell(*x);
      result.ceiling_row = row;
      result.ceiling_col = centre;
      if (!tile_is_solid(level_tile(level, centre, row))) {
        int a = cell(left + EPSILON), b = cell(right - EPSILON);
        result.ceiling_col =
            tile_is_solid(level_tile(level, a, row)) ? a : b;
      }
    }
  } else {
    /* Resting: still grounded if the floor is right below. */
    int row = cell(*y + EPSILON * 4);
    if (row_blocked(level, row, left + EPSILON, right - EPSILON))
      result.grounded = true;
  }
  return result;
}
