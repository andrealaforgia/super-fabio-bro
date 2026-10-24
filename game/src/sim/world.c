#include "world.h"

#include <math.h>
#include <string.h>

void world_emit(world_t* world, event_kind_t kind, float x) {
  if (world->event_count >= MAX_EVENTS) return;
  world->events[world->event_count++] = (event_t){kind, x};
}

void world_clear_events(world_t* world) { world->event_count = 0; }

bool world_has_event(const world_t* world, event_kind_t kind) {
  for (int i = 0; i < world->event_count; i++)
    if (world->events[i].kind == kind) return true;
  return false;
}

int world_count_enemies(const world_t* world, enemy_kind_t kind) {
  int count = 0;
  for (int i = 0; i < MAX_ENEMIES; i++)
    if (world->enemies[i].alive && world->enemies[i].kind == kind) count++;
  return count;
}

float world_progress(const world_t* world) {
  float span = world->level.goal_x - world->level.spawn_x;
  return clampf((world->player.x - world->level.spawn_x) / span, 0.0f, 1.0f);
}

void world_init(world_t* world, uint32_t seed) {
  memset(world, 0, sizeof(*world));
  level_generate(&world->level, seed);
  rng_seed(&world->rng, seed ^ 0x5bd1e995u);
  player_reset(&world->player, &world->level);
  world->state = STATE_PLAYING;
  world->view_w = 960.0f;
  world->spawning = true;
  world->zombie_timer = 5.0f;
  world->bug_timer = 9.0f;
  world->camera_x = 0.0f;
}

static void follow_camera(world_t* world, float dt) {
  const player_t* p = &world->player;
  float target = p->x - world->view_w * 0.42f + (float)p->facing * world->view_w * 0.06f;
  target = clampf(target, 0.0f, fmaxf(0.0f, level_width() - world->view_w));
  float blend = 1.0f - expf(-6.0f * dt);
  world->camera_x += (target - world->camera_x) * blend;
}

void world_update(world_t* world, const controls_t* controls, float dt) {
  world->time += dt;
  world->state_time += dt;
  if (world->state == STATE_GAME_OVER || world->state == STATE_WON) {
    combat_update(world, dt);
    follow_camera(world, dt);
    return;
  }

  player_update(world, controls, dt);
  if (world->state == STATE_PLAYING) {
    spawner_update(world, dt);
    enemies_update(world, dt);
  }
  combat_update(world, dt);
  items_update(world, dt);
  follow_camera(world, dt);

  if (world->state == STATE_DYING && world->state_time > 1.8f) {
    world->state = STATE_GAME_OVER;
    world->state_time = 0.0f;
  }
  if (world->state == STATE_PLAYING &&
      world->player.x >= world->level.goal_x) {
    world->state = STATE_WON;
    world->state_time = 0.0f;
    world->score += world->player.hp * 1000;
    world_emit(world, EVENT_WIN, world->player.x);
  }
}
