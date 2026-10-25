#ifndef TESTS_TEST_WORLD_H_
#define TESTS_TEST_WORLD_H_

#include <string.h>

#include "world.h"

/* An empty world with a flat floor and nothing generated: room to stage a
 * scene. The player stands at column 10; nothing spawns by itself. */
static inline void empty_world(world_t* world) {
  world_init(world, 1);
  memset(world->level.tiles, 0, sizeof(world->level.tiles));
  memset(world->level.content, 0, sizeof(world->level.content));
  world->level.item_count = 0;
  world->level.decor_count = 0;
  for (int col = 0; col < LEVEL_COLS; col++) {
    world->level.tiles[GROUND_ROW][col] = TILE_GRASS;
    for (int row = GROUND_ROW + 1; row < LEVEL_ROWS; row++)
      world->level.tiles[row][col] = TILE_ROCK;
  }
  world->level.goal_x = level_width() - 200.0f;
  world->spawning = false;
  world->player.x = 10.0f * TILE;
  world->player.y = (float)(GROUND_ROW * TILE);
  world->camera_x = world->player.x - 400.0f;
}

static inline void step(world_t* world, const buttons_t* held, float seconds) {
  static controls_t controls;
  const float dt = 1.0f / 120.0f;
  const buttons_t none = 0;
  if (!held) held = &none;
  for (float t = 0.0f; t < seconds - 1e-6f; t += dt) {
    controls_update(&controls, *held);
    world_update(world, &controls, dt);
  }
}

static inline void settle(world_t* world) { step(world, NULL, 0.3f); }

static inline enemy_t* first_enemy(world_t* world) {
  for (int i = 0; i < MAX_ENEMIES; i++)
    if (world->enemies[i].alive) return &world->enemies[i];
  return NULL;
}

#endif  // TESTS_TEST_WORLD_H_
