#include <math.h>

#include "world.h"

#define LAND_TOLERANCE 6.0f

static void place(world_t* world, bool moving) {
  const level_t* level = &world->level;
  for (int i = 0; i < level->platform_count; i++) {
    plank_t* plank = &world->planks[i];
    float x, top;
    platform_position(&level->platforms[i], world->time, &x, &top);
    plank->dx = moving ? x - plank->x : 0.0f;
    plank->dy = moving ? top - plank->top : 0.0f;
    plank->x = x;
    plank->top = top;
  }
}

void platforms_reset(world_t* world) { place(world, false); }

void platforms_update(world_t* world) { place(world, true); }

static bool over_plank(float x, const platform_def_t* def, const plank_t* plank) {
  float half = def->w * 0.5f;
  return x >= plank->x - half && x <= plank->x + half;
}

void platforms_carry_body(world_t* world, float* x, float* y, int* platform) {
  if (*platform < 0) return;
  const platform_def_t* def = &world->level.platforms[*platform];
  const plank_t* plank = &world->planks[*platform];
  if (plank->off) { /* it has gone from under him */
    *platform = -1;
    return;
  }
  /* Whoever has been walking on the plank goes where it goes. */
  *x += plank->dx;
  *y = plank->top;
  if (!over_plank(*x, def, plank)) *platform = -1;
}

void platforms_land_body(world_t* world, float* x, float* y, float* vy, bool* grounded, int* platform, float feet_before) {
  const level_t* level = &world->level;
  int found = -1;
  if (*vy >= 0.0f) {
    for (int i = 0; i < level->platform_count && found < 0; i++) {
      const plank_t* plank = &world->planks[i];
      if (plank->off || !over_plank(*x, &level->platforms[i], plank)) continue;
      if (feet_before <= plank->top + LAND_TOLERANCE && *y >= plank->top - 0.5f) found = i;
    }
  }
  if (found >= 0) {
    *y = world->planks[found].top;
    *vy = 0.0f;
    *grounded = true;
  }
  *platform = found;
}

void platforms_carry(world_t* world, player_t* p) { platforms_carry_body(world, &p->x, &p->y, &p->platform); }

void platforms_land(world_t* world, player_t* p, float feet_before) {
  platforms_land_body(world, &p->x, &p->y, &p->vy, &p->grounded, &p->platform, feet_before);
}
