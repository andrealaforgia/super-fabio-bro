#include <math.h>

#include "world.h"

#define ITEM_REACH 18.0f
#define MAX_POTIONS 5
#define BUMP_TIME 0.18f

static void collect(world_t* world, item_kind_t kind, float x, float y) {
  player_t* p = &world->player;
  switch (kind) {
    case ITEM_COIN:
      world->coins++;
      world_award(world, 100, x, y);
      world_emit(world, EVENT_COIN, x);
      return;
    case ITEM_POTION:
      if (p->potions < MAX_POTIONS) p->potions++;
      world_award(world, 250, x, y);
      break;
    case ITEM_PISTOL:
      p->ammo += PISTOL_AMMO;
      world_award(world, 250, x, y);
      break;
    case ITEM_MUSHROOM:
      if (p->hp < PLAYER_HP) {
        p->hp++;
        world_emit(world, EVENT_HEAL, x);
      }
      world_award(world, 500, x, y);
      break;
    default:
      return;
  }
  world_emit(world, EVENT_PICKUP, x);
}

void world_spawn_pickup(world_t* world, item_kind_t kind, float x, float y) {
  pickup_t* pickup = pool_acquire(&world->pickups, NULL);
  if (pickup) *pickup = (pickup_t){kind, x, y, 0.0f, -300.0f, 0.0f};
}

void world_bump_block(world_t* world, int col, int row) {
  tile_t tile = level_tile(&world->level, col, row);
  if (tile != TILE_QBLOCK && tile != TILE_BRICK) return;
  bump_t* bump = pool_acquire(&world->bumps, NULL);
  if (bump) *bump = (bump_t){col, row, 0.001f};
  float cx = (float)(col * TILE + TILE / 2);
  world_emit(world, EVENT_BUMP, cx);
  if (tile != TILE_QBLOCK) return;

  item_kind_t what = (item_kind_t)world->level.content[row][col];
  world->level.tiles[row][col] = TILE_USED;
  world_emit(world, EVENT_BLOCK_OPENS, cx);
  float top = (float)(row * TILE);
  if (what == ITEM_COIN || what == ITEM_NONE) {
    collect(world, ITEM_COIN, cx, top - 8.0f);
    world_burst(world, cx, top - 8.0f, 8, 255, 210, 40, 220.0f);
  } else {
    world_spawn_pickup(world, what, cx, top - 14.0f);
  }
}

void items_update(world_t* world, float dt) {
  player_t* p = &world->player;
  size_t bump_cursor = 0;
  for (bump_t* bump; (bump = pool_next_active(&world->bumps, &bump_cursor));) {
    bump->age += dt;
    if (bump->age >= BUMP_TIME) pool_release_object(&world->bumps, bump);
  }
  if (!p->alive) return;
  frect_t body = player_rect(p);
  const level_t* level = &world->level;

  for (int i = 0; i < level->item_count; i++) {
    if (world->item_taken[i]) continue;
    const placement_t* item = &level->items[i];
    frect_t box = {item->x - ITEM_REACH, item->y - ITEM_REACH, ITEM_REACH * 2.0f,
                  ITEM_REACH * 2.0f};
    if (frect_overlaps(body, box)) {
      world->item_taken[i] = true;
      collect(world, item->kind, item->x, item->y);
    }
  }

  size_t pickup_cursor = 0;
  for (pickup_t* pickup;
       (pickup = pool_next_active(&world->pickups, &pickup_cursor));) {
    pickup->age += dt;
    float w = 24.0f, h = 24.0f;
    float feet = pickup->y + h * 0.5f;
    pickup->vy = fminf(pickup->vy + GRAVITY * 0.6f * dt, MAX_FALL);
    physics_move(level, &pickup->x, &feet, &pickup->vx, &pickup->vy, w, h, dt);
    pickup->y = feet - h * 0.5f;
    if (pickup->y > level_height() + 100.0f) {
      pool_release_object(&world->pickups, pickup);
      continue;
    }
    frect_t box = {pickup->x - ITEM_REACH, pickup->y - ITEM_REACH,
                  ITEM_REACH * 2.0f, ITEM_REACH * 2.0f};
    if (pickup->age > 0.25f && frect_overlaps(body, box)) {
      pool_release_object(&world->pickups, pickup);
      collect(world, pickup->kind, pickup->x, pickup->y);
    }
  }
}
