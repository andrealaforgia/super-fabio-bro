#include <math.h>

#include "world.h"

void world_fire_force_burst(world_t* world) {
  player_t* p = &world->player;
  force_burst_t* burst = pool_acquire(&world->force_bursts, NULL);
  if (!burst) return;
  *burst = (force_burst_t){p->x + (float)p->facing * 36.0f, p->y - 32.0f, (float)p->facing * FORCE_BURST_SPEED, 0.0f};
  p->shoot_pose = 0.3f;
  world_emit(world, EVENT_FORCE_BURST, burst->x);
}

float greeting_alpha(float left) {
  float age = GREETING_SECONDS - left;
  if (left <= 0.0f || age < 0.0f) return 0.0f;
  if (age < GREETING_FADE_IN) return age / GREETING_FADE_IN;
  if (left < GREETING_FADE_OUT) return left / GREETING_FADE_OUT;
  return 1.0f;
}

/* Everything it passes through is destroyed, and it flies on; a wall stops it. */
void force_bursts_update(world_t* world, float dt) {
  size_t cursor = 0;
  for (force_burst_t* b; (b = pool_next_active(&world->force_bursts, &cursor));) {
    b->age += dt;
    b->x += b->vx * dt;
    if (b->age > FORCE_BURST_LIFE || level_solid_at(&world->level, b->x, b->y)) {
      world_burst(world, b->x, b->y, 6, 90, 255, 120, 180.0f);
      pool_release_object(&world->force_bursts, b);
      continue;
    }
    frect_t box = {b->x - FORCE_BURST_W * 0.5f, b->y - FORCE_BURST_H * 0.5f, FORCE_BURST_W, FORCE_BURST_H};
    size_t target_cursor = 0;
    for (enemy_t* e; (e = pool_next_active(&world->enemies, &target_cursor));) {
      if (!frect_overlaps(box, enemy_rect(e))) continue;
      world_add_effect(world, FX_FORCE_HIT, e->x, e->y - e->h * 0.5f, 0.4f, 1);
      world_kill_enemy(world, e, true);
    }
  }
}
