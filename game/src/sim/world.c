#include "world.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

void world_emit(world_t* world, event_kind_t kind, float x) {
  if (!world->events) return;
  game_event_t event = {(int)kind, &x, sizeof(x)};
  publish(world->events, &event);
}

int world_count_enemies(const world_t* world, enemy_kind_t kind) {
  int count = 0;
  size_t cursor = 0;
  for (const enemy_t* e; (e = pool_next_active(&world->enemies, &cursor));)
    if (e->kind == kind) count++;
  return count;
}

float world_progress(const world_t* world) {
  float span = world->level.goal_x - world->level.spawn_x;
  return clampf((world->player.x - world->level.spawn_x) / span, 0.0f, 1.0f);
}

static bool make_pools(world_t* world) {
  world->enemies = create_object_pool(sizeof(enemy_t), MAX_ENEMIES);
  world->pickups = create_object_pool(sizeof(pickup_t), MAX_PICKUPS);
  world->bullets = create_object_pool(sizeof(bullet_t), MAX_BULLETS);
  world->rays = create_object_pool(sizeof(ray_t), MAX_RAYS);
  world->effects = create_object_pool(sizeof(effect_t), MAX_EFFECTS);
  world->particles = create_object_pool(sizeof(particle_t), MAX_PARTICLES);
  world->bumps = create_object_pool(sizeof(bump_t), MAX_BUMPS);
  return world->enemies.objects && world->pickups.objects &&
         world->bullets.objects && world->rays.objects &&
         world->effects.objects && world->particles.objects &&
         world->bumps.objects;
}

static object_pool_t* pools_of(world_t* world, int index) {
  object_pool_t* all[] = {&world->enemies, &world->pickups, &world->bullets,
                          &world->rays,    &world->effects, &world->particles,
                          &world->bumps};
  return all[index];
}

void world_restart(world_t* world, uint32_t seed) {
  world_t fresh;  /* everything but the pools and the events starts from zero */
  memset(&fresh, 0, sizeof(fresh));
  for (int i = 0; i < 7; i++) {
    *pools_of(&fresh, i) = *pools_of(world, i);
    pool_reset(pools_of(&fresh, i));
  }
  fresh.events = world->events;
  memcpy(world, &fresh, sizeof(*world));
  level_generate(&world->level, seed);
  rng_seed(&world->rng, seed ^ 0x5bd1e995u);
  player_reset(&world->player, &world->level);
  world->state = STATE_PLAYING;
  world->view_w = 960.0f;
  world->spawning = true;
  world->zombie_timer = 5.0f;
  world->bug_timer = 9.0f;
  world->book_reader_timer = 8.0f;
  world->camera_x = 0.0f;
}

world_t* world_new(uint32_t seed, event_system_ptr events) {
  world_t* world = calloc(1, sizeof(*world));
  if (!world) return NULL;
  if (!make_pools(world)) {
    world_free(world);
    return NULL;
  }
  world->events = events;
  world_restart(world, seed);
  return world;
}

void world_free(world_t* world) {
  if (!world) return;
  for (int i = 0; i < 7; i++) pool_destroy(pools_of(world, i));
  free(world);
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
