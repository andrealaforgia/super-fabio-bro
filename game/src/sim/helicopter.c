#include <math.h>

#include "world.h"

frect_t helicopter_rect(const helicopter_t* h) {
  frect_t r = {h->x - HELICOPTER_W * 0.5f, h->y - HELICOPTER_H, HELICOPTER_W, HELICOPTER_H};
  return r;
}

/* He has to come down on it: still on his way up, or walking in from the
 * side, he goes through it. */
static bool lands_on(const player_t* p, const helicopter_t* h) {
  frect_t box = helicopter_rect(h);
  return p->vy > 0.0f && p->y < box.y + box.h * 0.5f && frect_overlaps(player_rect(p), box);
}

static void board(world_t* world) {
  player_t* p = &world->player;
  world->helicopter.present = false;
  p->flying = true;
  p->fuel = HELICOPTER_FUEL_SECONDS;
  p->vy = 0.0f;
  p->grounded = false;
  p->platform = -1;
  world_emit(world, EVENT_HELICOPTER_BOARDS, p->x);
}

/* The parachute opens above the ground: he floats down under it. */
static void open_parachute(world_t* world) {
  player_t* p = &world->player;
  p->chute = true;
  p->chute_age = 0.0f;
  p->landing = 0.0f;
  p->vy = fminf(p->vy, PARACHUTE_FALL);
  world_emit(world, EVENT_PARACHUTE_OPENS, p->x);
}

void helicopter_bail_out(world_t* world) {
  player_t* p = &world->player;
  p->flying = false;
  p->fuel = 0.0f;
  p->vy = -120.0f; /* a little hop out of the seat */
  p->grounded = false;
  p->platform = -1;
  world_burst(world, p->x, p->y - 30.0f, 8, 230, 230, 230, 160.0f);
  world_emit(world, EVENT_JUMP, p->x);
  if (p->y < (float)(GROUND_ROW * TILE) - 40.0f) open_parachute(world);
}

/* Out of fuel: it blows up where it is, and he drops out of it, under a parachute if he is up high. */
static void explode(world_t* world) {
  player_t* p = &world->player;
  p->flying = false;
  p->fuel = 0.0f;
  if (p->y < (float)(GROUND_ROW * TILE) - 40.0f) open_parachute(world);
  world_add_effect(world, FX_HELICOPTER_BOOM, p->x, p->y, HELICOPTER_BOOM_SECONDS, p->facing);
  world_emit(world, EVENT_HELICOPTER_EXPLODES, p->x);
}

void helicopter_update(world_t* world, float dt) {
  player_t* p = &world->player;
  if (!p->alive) return;
  if (p->flying) {
    p->fuel -= dt;
    if (p->fuel <= 0.0f) explode(world);
  } else if (world->helicopter.present && lands_on(p, &world->helicopter)) {
    board(world);
  }
}
