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

void city_update(world_t* world, const controls_t* controls, float dt) {
  const level_t* level = &world->level;
  if (level->vase_count > 0) vases(world, dt);
  if (!level->has_aqueduct) return;
  tap(world, controls, dt);
  if (world->water_left > 0.0f) water_washes(world);
}
