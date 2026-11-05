#include <math.h>

#include "world.h"

#define DEG (3.14159265f / 180.0f)

static void muzzle_of(const world_t* world, float* x, float* y) {
  const player_t* p = &world->player;
  *x = p->x + (float)p->facing * 40.0f;
  *y = p->y - 30.0f;
}

void world_fire_bubbles(world_t* world) {
  player_t* p = &world->player;
  float x, y;
  muzzle_of(world, &x, &y);
  for (int i = 0; i < BUBBLES_PER_SHOT; i++) {
    bubble_t* bubble = pool_acquire(&world->bubbles, NULL);
    if (!bubble) break;
    float angle = (rng_float(&world->rng) * 2.0f - 1.0f) * BUBBLE_SPREAD * DEG;
    float speed = BUBBLE_SPEED * (0.8f + 0.4f * rng_float(&world->rng));
    *bubble = (bubble_t){x, y, (float)p->facing * cosf(angle) * speed, sinf(angle) * speed - 25.0f,
                         0.0f, BUBBLE_R0, -1, 0, 0.0f};
  }
  p->shoot_pose = 0.2f;
  world_emit(world, EVENT_BUBBLE_SHOT, x);
}

static float enclosing_radius(const enemy_t* e) { return hypotf(e->w, e->h) * 0.5f + 10.0f; }

bool enemy_in_bubble(const world_t* world, const enemy_t* enemy) {
  int slot = (int)pool_index_of(&world->enemies, enemy);
  size_t cursor = 0;
  for (const bubble_t* b; (b = pool_next_active(&world->bubbles, &cursor));)
    if (b->victim == slot) return true;
  return false;
}

static void pop_spray(world_t* world, const bubble_t* b) {
  world_burst(world, b->x, b->y, 10, 190, 225, 255, 180.0f);
  world_burst(world, b->x, b->y, 6, 255, 255, 255, 120.0f);
}

/* A flying bubble swells; the first enemy it touches is wrapped, and a wall or
 * old age bursts it. */
static void fly(world_t* world, bubble_t* b, float dt) {
  b->age += dt;
  b->r = BUBBLE_R0 + (BUBBLE_R_MAX - BUBBLE_R0) * fminf(b->age / BUBBLE_GROW_SECONDS, 1.0f);
  b->vx *= expf(-0.7f * dt);
  b->vy -= 18.0f * dt; /* a soap bubble drifts up */
  b->x += b->vx * dt;
  b->y += b->vy * dt;
  if (b->age >= BUBBLE_LIFE || level_solid_at(&world->level, b->x, b->y)) {
    pop_spray(world, b);
    pool_release_object(&world->bubbles, b);
    return;
  }
  float reach = b->r * 0.85f;
  frect_t box = {b->x - reach, b->y - reach, reach * 2.0f, reach * 2.0f};
  size_t cursor = 0;
  for (enemy_t* e; (e = pool_next_active(&world->enemies, &cursor));) {
    if (enemy_in_bubble(world, e) || !frect_overlaps(box, enemy_rect(e))) continue;
    b->victim = (int)pool_index_of(&world->enemies, e);
    b->victim_kind = (int)e->kind;
    b->wrapped = 0.0f;
    world_emit(world, EVENT_BUBBLE_WRAPS, e->x);
    return;
  }
}

/* A wrapped bubble closes round the enemy, carries him up, then bursts and he with it. */
static void hold(world_t* world, bubble_t* b, float dt) {
  enemy_t* e = pool_is_active(&world->enemies, (size_t)b->victim) ? pool_get_at(&world->enemies, (size_t)b->victim)
                                                                    : NULL;
  if (!e || (int)e->kind != b->victim_kind) { /* he died some other way: nothing left to hold */
    pool_release_object(&world->bubbles, b);
    return;
  }
  b->wrapped += dt;
  if (b->wrapped >= BUBBLE_WRAP_SECONDS) {
    pop_spray(world, b);
    world_emit(world, EVENT_BUBBLE_POPS, b->x);
    pool_release_object(&world->bubbles, b);
    world_kill_enemy(world, e, false);
    return;
  }
  e->y -= BUBBLE_LIFT * dt;
  float wobble = 1.0f + 0.05f * sinf(b->wrapped * 14.0f);
  float target = enclosing_radius(e) * wobble;
  b->r += (target - b->r) * fminf(1.0f, 12.0f * dt);
  b->x = e->x;
  b->y = e->y - e->h * 0.5f;
}

void bubbles_update(world_t* world, float dt) {
  size_t cursor = 0;
  for (bubble_t* b; (b = pool_next_active(&world->bubbles, &cursor));) {
    if (b->victim < 0) fly(world, b, dt);
    else hold(world, b, dt);
  }
}
