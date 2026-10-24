#include <string.h>

#include "test_groups.h"
#include "test_support.h"
#include "test_world.h"

static world_t* fresh(void) {
  world_t* world = malloc(sizeof(*world));
  empty_world(world);
  settle(world);
  return world;
}

static bool on_screen(const world_t* world, float x) {
  return x >= world->camera_x && x <= world->camera_x + world->view_w;
}

static void zombies_appear_at_regular_intervals_off_screen(void) {
  world_t* world = fresh();
  world->spawning = true;
  world->zombie_timer = 2.0f;
  world->bug_timer = 1000.0f;
  world->player.invulnerable = 1000.0f;
  int spawned = 0;
  float last_spawn = -1.0f, shortest = 100.0f, longest = 0.0f;
  for (int i = 0; i < 60 * 60; i++) {
    step(world, NULL, 1.0f / 60.0f);
    for (int k = 0; k < MAX_ENEMIES; k++) {
      enemy_t* e = &world->enemies[k];
      if (!e->alive) continue;
      /* Each zombie is seen the moment it appears, then cleared away. */
      CHECK(!on_screen(world, e->x));
      CHECK(e->facing == (e->x > world->player.x ? -1 : 1));
      if (last_spawn >= 0.0f) {
        float gap = world->time - last_spawn;
        if (gap < shortest) shortest = gap;
        if (gap > longest) longest = gap;
      }
      last_spawn = world->time;
      spawned++;
      e->alive = false;
    }
  }
  CHECK(spawned >= 8);
  CHECK(shortest > 1.0f);
  CHECK(longest < 9.0f);
  free(world);
}

static void a_zombie_walks_towards_fabio_from_either_side(void) {
  world_t* world = fresh();
  spawn_zombie(world, world->player.x + 300.0f, world->player.y);
  spawn_zombie(world, world->player.x - 300.0f, world->player.y);
  world->player.invulnerable = 1000.0f;
  step(world, NULL, 1.0f);
  int closer = 0;
  for (int i = 0; i < MAX_ENEMIES; i++) {
    const enemy_t* e = &world->enemies[i];
    if (!e->alive) continue;
    CHECK(fabsf(e->x - world->player.x) < 285.0f);
    closer++;
  }
  CHECK(closer == 2);
  free(world);
}

static void a_zombie_hops_over_a_block_in_its_way(void) {
  world_t* world = fresh();
  world->player.invulnerable = 1000.0f;
  world->level.tiles[GROUND_ROW - 1][20] = TILE_STONE;
  spawn_zombie(world, 25 * TILE, world->player.y);
  world->player.x = 10 * TILE;
  step(world, NULL, 6.0f);
  CHECK(first_enemy(world) != NULL);
  CHECK(first_enemy(world)->x < 20 * TILE);
  free(world);
}

static void a_zombie_that_walks_into_a_pit_is_gone(void) {
  world_t* world = fresh();
  world->player.invulnerable = 1000.0f;
  for (int col = 20; col < 24; col++)
    for (int row = GROUND_ROW; row < LEVEL_ROWS; row++)
      world->level.tiles[row][col] = TILE_EMPTY;
  spawn_zombie(world, 26 * TILE, world->player.y);
  world->enemies[0].speed = 90.0f;
  world->player.x = 10 * TILE;
  step(world, NULL, 6.0f);
  CHECK(world_count_enemies(world, ENEMY_ZOMBIE) == 0);
  free(world);
}

static void zombies_far_behind_are_forgotten(void) {
  world_t* world = fresh();
  world->player.x = 120 * TILE;
  spawn_zombie(world, 2.0f * TILE, world->player.y);
  step(world, NULL, 0.1f);
  CHECK(world_count_enemies(world, ENEMY_ZOMBIE) == 0);
  free(world);
}

static void there_is_a_cap_on_how_many_zombies_chase_him(void) {
  world_t* world = fresh();
  world->spawning = true;
  world->bug_timer = 1e9f;
  world->player.invulnerable = 1e9f;
  world->player.x = 40 * TILE;
  world->camera_x = world->player.x - 400.0f;
  int most = 0;
  for (int i = 0; i < 120; i++) {
    step(world, NULL, 1.0f);
    int n = world_count_enemies(world, ENEMY_ZOMBIE);
    if (n > most) most = n;
  }
  CHECK(most <= 5 + 6);
  CHECK(most >= 3);
  free(world);
}

static void a_flying_bug_closes_in_and_hovers_near_fabio(void) {
  world_t* world = fresh();
  world->player.invulnerable = 1000.0f;
  spawn_bug(world, BUG_FLY, world->player.x + 500.0f, world->player.y - 200.0f);
  step(world, NULL, 6.0f);
  const enemy_t* e = first_enemy(world);
  CHECK(e != NULL);
  CHECK(fabsf(e->x - world->player.x) < 120.0f);
  CHECK(e->y > world->player.y - 140.0f && e->y < world->player.y + 10.0f);
  free(world);
}

static void ground_bugs_walk_and_the_dark_beetle_hops(void) {
  world_t* world = fresh();
  world->player.invulnerable = 1000.0f;
  spawn_bug(world, BUG_DARK, world->player.x + 400.0f, world->player.y);
  float lowest = 1e9f;
  for (int i = 0; i < 300; i++) {
    step(world, NULL, 1.0f / 60.0f);
    if (world->enemies[0].y < lowest) lowest = world->enemies[0].y;
  }
  CHECK(world->enemies[0].x < world->player.x + 380.0f);
  CHECK(lowest < world->player.y - 10.0f);
  free(world);
}

static void bugs_come_with_the_zombies(void) {
  world_t* world = fresh();
  world->spawning = true;
  world->zombie_timer = 1e9f;
  world->bug_timer = 1.0f;
  world->player.invulnerable = 1e9f;
  int seen = 0;
  for (int i = 0; i < 60; i++) {
    step(world, NULL, 1.0f);
    seen += world_count_enemies(world, ENEMY_BUG) > 0;
  }
  CHECK(seen > 10);
  free(world);
}

void run_enemy_tests(void) {
  RUN(zombies_appear_at_regular_intervals_off_screen);
  RUN(a_zombie_walks_towards_fabio_from_either_side);
  RUN(a_zombie_hops_over_a_block_in_its_way);
  RUN(a_zombie_that_walks_into_a_pit_is_gone);
  RUN(zombies_far_behind_are_forgotten);
  RUN(there_is_a_cap_on_how_many_zombies_chase_him);
  RUN(a_flying_bug_closes_in_and_hovers_near_fabio);
  RUN(ground_bugs_walk_and_the_dark_beetle_hops);
  RUN(bugs_come_with_the_zombies);
}
