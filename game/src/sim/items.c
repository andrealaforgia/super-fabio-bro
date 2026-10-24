#include <math.h>

#include "world.h"

#define ITEM_REACH 18.0f
#define MAX_POTIONS 5
#define BUMP_TIME 0.18f

static void collect(world_t* world, item_kind_t kind, float x) {
  player_t* p = &world->player;
  switch (kind) {
    case ITEM_COIN:
      world->coins++;
      world->score += 100;
      world_emit(world, EVENT_COIN, x);
      return;
    case ITEM_POTION:
      if (p->potions < MAX_POTIONS) p->potions++;
      world->score += 250;
      break;
    case ITEM_PISTOL:
      p->has_gun = true;
      world->score += 250;
      break;
    case ITEM_MUSHROOM:
      if (p->hp < PLAYER_HP) {
        p->hp++;
        world_emit(world, EVENT_HEAL, x);
      }
      world->score += 500;
      break;
    default:
      return;
  }
  world_emit(world, EVENT_PICKUP, x);
}

void world_spawn_pickup(world_t* world, item_kind_t kind, float x, float y) {
  for (int i = 0; i < MAX_PICKUPS; i++) {
    pickup_t* pickup = &world->pickups[i];
    if (pickup->alive) continue;
    *pickup = (pickup_t){true, kind, x, y, 0.0f, -300.0f, 0.0f};
    return;
  }
}

void world_bump_block(world_t* world, int col, int row) {
  tile_t tile = level_tile(&world->level, col, row);
  if (tile != TILE_QBLOCK && tile != TILE_BRICK) return;
  for (int i = 0; i < MAX_BUMPS; i++) {
    if (world->bumps[i].age > 0.0f && world->bumps[i].age < BUMP_TIME) continue;
    world->bumps[i] = (bump_t){col, row, 0.001f};
    break;
  }
  float cx = (float)(col * TILE + TILE / 2);
  world_emit(world, EVENT_BUMP, cx);
  if (tile != TILE_QBLOCK) return;

  item_kind_t what = (item_kind_t)world->level.content[row][col];
  world->level.tiles[row][col] = TILE_USED;
  world_emit(world, EVENT_BLOCK_OPENS, cx);
  float top = (float)(row * TILE);
  if (what == ITEM_COIN || what == ITEM_NONE) {
    collect(world, ITEM_COIN, cx);
    world_burst(world, cx, top - 8.0f, 8, 255, 210, 40, 220.0f);
  } else {
    world_spawn_pickup(world, what, cx, top - 14.0f);
  }
}

void items_update(world_t* world, float dt) {
  player_t* p = &world->player;
  for (int i = 0; i < MAX_BUMPS; i++) {
    bump_t* bump = &world->bumps[i];
    if (bump->age > 0.0f) {
      bump->age += dt;
      if (bump->age >= BUMP_TIME) bump->age = 0.0f;
    }
  }
  if (!p->alive) return;
  rect_t body = player_rect(p);
  const level_t* level = &world->level;

  for (int i = 0; i < level->item_count; i++) {
    if (world->item_taken[i]) continue;
    const placement_t* item = &level->items[i];
    rect_t box = {item->x - ITEM_REACH, item->y - ITEM_REACH, ITEM_REACH * 2.0f,
                  ITEM_REACH * 2.0f};
    if (rect_overlaps(body, box)) {
      world->item_taken[i] = true;
      collect(world, item->kind, item->x);
    }
  }

  for (int i = 0; i < MAX_PICKUPS; i++) {
    pickup_t* pickup = &world->pickups[i];
    if (!pickup->alive) continue;
    pickup->age += dt;
    float w = 24.0f, h = 24.0f;
    float feet = pickup->y + h * 0.5f;
    pickup->vy = fminf(pickup->vy + GRAVITY * 0.6f * dt, MAX_FALL);
    physics_move(level, &pickup->x, &feet, &pickup->vx, &pickup->vy, w, h, dt);
    pickup->y = feet - h * 0.5f;
    if (pickup->y > level_height() + 100.0f) {
      pickup->alive = false;
      continue;
    }
    rect_t box = {pickup->x - ITEM_REACH, pickup->y - ITEM_REACH,
                  ITEM_REACH * 2.0f, ITEM_REACH * 2.0f};
    if (pickup->age > 0.25f && rect_overlaps(body, box)) {
      pickup->alive = false;
      collect(world, pickup->kind, pickup->x);
    }
  }
}
