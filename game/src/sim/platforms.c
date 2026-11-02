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

static bool over_plank(const player_t* p, const platform_def_t* def, const plank_t* plank) {
  float half = def->w * 0.5f;
  return p->x >= plank->x - half && p->x <= plank->x + half;
}

void platforms_carry(world_t* world, player_t* p) {
  if (p->platform < 0) return;
  const platform_def_t* def = &world->level.platforms[p->platform];
  const plank_t* plank = &world->planks[p->platform];
  /* He has been walking on the plank: it moved on, and he moves with it. */
  p->x += plank->dx;
  p->y = plank->top;
  if (!over_plank(p, def, plank)) p->platform = -1;
}

void platforms_land(world_t* world, player_t* p, float feet_before) {
  const level_t* level = &world->level;
  bool rising = p->vy < 0.0f;
  int found = -1;
  if (!rising) {
    for (int i = 0; i < level->platform_count && found < 0; i++) {
      const plank_t* plank = &world->planks[i];
      if (!over_plank(p, &level->platforms[i], plank)) continue;
      if (feet_before <= plank->top + LAND_TOLERANCE && p->y >= plank->top - 0.5f) found = i;
    }
  }
  if (found >= 0) {
    p->y = world->planks[found].top;
    p->vy = 0.0f;
    p->grounded = true;
  }
  p->platform = found;
}
