#ifndef TESTS_TEST_WORLD_H_
#define TESTS_TEST_WORLD_H_

#include <assert.h>
#include <string.h>

#include "world.h"

/* An empty world with a flat floor and nothing generated: room to stage a
 * scene. The player stands at column 10; nothing spawns by itself. */
/* What the world announced: tests subscribe to every event and remember. */
static event_system_t test_events;
static bool test_event_seen[EVENT_KIND_COUNT];
static int test_event_total;

static inline void remember_event(const game_event_t* event, void* user) {
  (void)user;
  test_event_seen[event->type] = true;
  test_event_total++;
}

static inline event_system_ptr recording_events(void) {
  test_events = create_event_system();
  memset(test_event_seen, 0, sizeof(test_event_seen));
  test_event_total = 0;
  for (int kind = 0; kind < EVENT_KIND_COUNT; kind++)
    subscribe(&test_events, kind, remember_event, NULL);
  return &test_events;
}

static inline bool world_has_event(const world_t* world, event_kind_t kind) {
  (void)world;
  return test_event_seen[kind];
}

static inline int world_event_count(const world_t* world) {
  (void)world;
  return test_event_total;
}

static inline void world_clear_events(world_t* world) {
  (void)world;
  memset(test_event_seen, 0, sizeof(test_event_seen));
  test_event_total = 0;
}

static inline world_t* new_empty_world(void) {
  world_t* world = world_new(1, recording_events());
  assert(world);
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
  return world;
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
  size_t cursor = 0;
  return pool_next_active(&world->enemies, &cursor);
}

/* The enemy in pool slot `index`; the first one spawned is in slot 0. */
static inline enemy_t* enemy_at(world_t* world, size_t index) {
  return pool_get_at(&world->enemies, index);
}

#endif  // TESTS_TEST_WORLD_H_
