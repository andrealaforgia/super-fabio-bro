#include <math.h>

#include "world.h"

/* The city of Sulmona: the vases that fall off the balconies, and the aqueduct's water. */

bool water_running(const world_t* world) { return world->water_left > 0.0f; }

bool fabio_at_tap(const world_t* world) {
  const player_t* p = &world->player;
  const level_t* level = &world->level;
  return level->has_aqueduct && p->alive && !p->flying && p->y >= (float)(GROUND_ROW * TILE) - 80.0f &&
         fabsf(p->x - level->tap_x) <= TAP_RANGE;
}

/* Up is also jump on the keyboard, so he may already be leaving the ground when it is pressed: low down will do. */
/* Up, at the tap, turns the water on (with it already running, up does nothing more). */
static void tap(world_t* world, const controls_t* controls, float dt) {
  if (world->water_left > 0.0f) {
    world->water_left = fmaxf(0.0f, world->water_left - dt);
    return;
  }
  if ((controls_pressed(controls, BTN_UP) || controls_pressed(controls, BTN_MELEE)) && fabio_at_tap(world)) {
    world->water_left = WATER_SECONDS;
    world_emit(world, EVENT_WATER_ON, world->level.tap_x);
  }
}

/* Whatever stands under a curtain of water, from the walkway to the street, is washed away. */
static void water_washes(world_t* world) {
  const level_t* level = &world->level;
  float top = level->aqueduct_deck;
  float ground = (float)(GROUND_ROW * TILE);
  for (int i = 0; i < level->spout_count; i++) {
    frect_t curtain = {level->spout_x[i] - WATER_HALF_WIDTH, top, WATER_HALF_WIDTH * 2.0f, ground - top};
    size_t cursor = 0;
    for (enemy_t* e; (e = pool_next_active(&world->enemies, &cursor));) {
      if (!frect_overlaps(curtain, enemy_rect(e))) continue;
      world_burst(world, e->x, e->y - e->h * 0.5f, 10, 120, 190, 255, 220.0f);
      world_kill_enemy(world, e, true);
    }
  }
}

static void vase_lands(world_t* world, vase_t* v) {
  v->state = VASE_SMASHED;
  world_burst(world, v->x, v->y - 10.0f, 12, 205, 105, 60, 260.0f);
  world_burst(world, v->x, v->y - 14.0f, 5, 230, 40, 60, 200.0f); /* the red geraniums */
  world_emit(world, EVENT_VASE_SMASHES, v->x);
}

/* A falling vase hits the first enemy it meets, or the street. */
static void vase_falls(world_t* world, vase_t* v, float dt) {
  v->vy = fminf(v->vy + GRAVITY * dt, MAX_FALL);
  v->y += v->vy * dt;
  frect_t box = {v->x - VASE_W * 0.5f, v->y - VASE_H, VASE_W, VASE_H};
  size_t cursor = 0;
  for (enemy_t* e; (e = pool_next_active(&world->enemies, &cursor));) {
    if (!frect_overlaps(box, enemy_rect(e))) continue;
    world_kill_enemy(world, e, true);
    vase_lands(world, v);
    return;
  }
  if (v->y >= (float)(GROUND_ROW * TILE)) {
    v->y = (float)(GROUND_ROW * TILE);
    vase_lands(world, v);
  }
}

static void vases(world_t* world, float dt) {
  const level_t* level = &world->level;
  const player_t* p = &world->player;
  for (int i = 0; i < level->vase_count; i++) {
    vase_t* v = &world->vases[i];
    if (v->state == VASE_STANDING) {
      /* Walking by on its balcony, he knocks it off. */
      if (p->alive && p->platform == level->vase_plank[i] && fabsf(p->x - level->vase_x[i]) <= VASE_KNOCK_RANGE) {
        *v = (vase_t){VASE_FALLING, level->vase_x[i], level->vase_y[i], 0.0f};
      }
    } else if (v->state == VASE_FALLING) {
      vase_falls(world, v, dt);
    }
  }
}

/* The statue wakes when he comes up to it; the bull rears, then runs the other way from him and
 * tramples everything that stands in its path. */
static void bull(world_t* world, float dt) {
  const level_t* level = &world->level;
  const player_t* p = &world->player;
  bull_t* b = &world->bull;
  float ground = (float)(GROUND_ROW * TILE);
  if (!b->active) {
    if (level->has_bull) world->planks[level->bull_plank].off = false; /* the statue is back */
    if (b->spent && fabsf(p->x - level->bull_x) > BULL_REARM_DISTANCE) b->spent = false;
    if (!b->spent && p->alive && !level->cave && fabsf(p->x - level->bull_x) <= BULL_TRIGGER && p->y >= ground - 140.0f) {
      *b = (bull_t){true, true, level->bull_x, level->bull_x, 0.0f, p->facing > 0 ? -1 : 1};
      world_burst(world, b->x, ground - 40.0f, 18, 214, 170, 100, 300.0f);
      world_emit(world, EVENT_BULL_CHARGES, b->x);
    }
    return;
  }
  b->age += dt;
  if (level->has_bull) /* off its back, everyone */
    world->planks[level->bull_plank].off = b->age > BULL_REAR_SECONDS;
  if (level->cave) {
    b->active = false;
    return;
  }
  if (b->age > BULL_REAR_SECONDS) b->x += (float)b->dir * BULL_SPEED * dt;
  frect_t box = {b->x - BULL_W * 0.5f, ground - BULL_H, BULL_W, BULL_H};
  size_t cursor = 0;
  for (enemy_t* e; (e = pool_next_active(&world->enemies, &cursor));) {
    if (!frect_overlaps(box, enemy_rect(e))) continue;
    world_burst(world, e->x, e->y - e->h * 0.5f, 10, 255, 255, 255, 260.0f);
    world_kill_enemy(world, e, true);
  }
  if (b->age > BULL_REAR_SECONDS && (fabsf(b->x - b->start) > BULL_RUN_DISTANCE || b->x < 0.0f || b->x > level_pixel_width(level))) b->active = false;
}

void city_update(world_t* world, const controls_t* controls, float dt) {
  const level_t* level = &world->level;
  if (level->has_bull || world->bull.active) bull(world, dt);
  if (level->vase_count > 0) vases(world, dt);
  if (!level->has_aqueduct) return;
  tap(world, controls, dt);
  if (world->water_left > 0.0f) water_washes(world);
}
