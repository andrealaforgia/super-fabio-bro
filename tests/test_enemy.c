#include <stdio.h>
#include <string.h>

#include "test_groups.h"
#include "test_support.h"
#include "test_world.h"

static world_t* fresh(void) {
  world_t* world = new_empty_world();
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
  world->book_reader_timer = 1e9f;
  world->monk_timer = 1e9f;
  world->chef_timer = 1e9f;
  world->gorilla_timer = 1e9f;
  world->player.invulnerable = 1000.0f;
  int spawned = 0;
  float last_spawn = -1.0f, shortest = 100.0f, longest = 0.0f;
  for (int i = 0; i < 60 * 60; i++) {
    step(world, NULL, 1.0f / 60.0f);
    size_t cursor = 0;
    for (enemy_t* e; (e = pool_next_active(&world->enemies, &cursor));) {
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
      pool_release_object(&world->enemies, e);
    }
  }
  CHECK(spawned >= 8);
  CHECK(shortest > 1.0f);
  CHECK(longest < 9.0f);
  world_free(world);
}

static void a_zombie_walks_towards_fabio_from_either_side(void) {
  world_t* world = fresh();
  spawn_zombie(world, world->player.x + 300.0f, world->player.y);
  spawn_zombie(world, world->player.x - 300.0f, world->player.y);
  world->player.invulnerable = 1000.0f;
  step(world, NULL, 1.0f);
  int closer = 0;
  size_t cursor = 0;
  for (const enemy_t* e; (e = pool_next_active(&world->enemies, &cursor));) {
    CHECK(fabsf(e->x - world->player.x) < 285.0f);
    closer++;
  }
  CHECK(closer == 2);
  world_free(world);
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
  world_free(world);
}

static void a_zombie_that_walks_into_a_pit_climbs_out_the_far_side(void) {
  world_t* world = fresh();
  world->player.invulnerable = 1000.0f;
  for (int col = 20; col < 24; col++)
    for (int row = GROUND_ROW; row < LEVEL_ROWS; row++)
      world->level.tiles[row][col] = TILE_EMPTY;
  spawn_zombie(world, 26 * TILE, world->player.y);
  enemy_at(world, 0)->speed = 90.0f;
  world->player.x = 10 * TILE;
  step(world, NULL, 6.0f);
  CHECK(world_count_enemies(world, ENEMY_ZOMBIE) == 1);
  CHECK(first_enemy(world)->x < 20 * TILE); /* out the far side, on towards Fabio */
  world_free(world);
}

static void zombies_far_behind_are_forgotten(void) {
  world_t* world = fresh();
  world->player.x = 120 * TILE;
  spawn_zombie(world, 2.0f * TILE, world->player.y);
  step(world, NULL, 0.1f);
  CHECK(world_count_enemies(world, ENEMY_ZOMBIE) == 0);
  world_free(world);
}

static void there_is_a_cap_on_how_many_zombies_chase_him(void) {
  world_t* world = fresh();
  world->spawning = true;
  world->bug_timer = 1e9f;
  world->book_reader_timer = 1e9f;
  world->monk_timer = 1e9f;
  world->chef_timer = 1e9f;
  world->gorilla_timer = 1e9f;
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
  world_free(world);
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
  world_free(world);
}

static void ground_bugs_walk_and_the_dark_beetle_hops(void) {
  world_t* world = fresh();
  world->player.invulnerable = 1000.0f;
  spawn_bug(world, BUG_DARK, world->player.x + 400.0f, world->player.y);
  float lowest = 1e9f;
  for (int i = 0; i < 300; i++) {
    step(world, NULL, 1.0f / 60.0f);
    if (enemy_at(world, 0)->y < lowest) lowest = enemy_at(world, 0)->y;
  }
  CHECK(enemy_at(world, 0)->x < world->player.x + 380.0f);
  CHECK(lowest < world->player.y - 10.0f);
  world_free(world);
}

static void bugs_come_with_the_zombies(void) {
  world_t* world = fresh();
  world->spawning = true;
  world->zombie_timer = 1e9f;
  world->bug_timer = 1.0f;
  world->book_reader_timer = 1e9f;
  world->monk_timer = 1e9f;
  world->chef_timer = 1e9f;
  world->gorilla_timer = 1e9f;
  world->player.invulnerable = 1e9f;
  int seen = 0;
  for (int i = 0; i < 60; i++) {
    step(world, NULL, 1.0f);
    seen += world_count_enemies(world, ENEMY_BUG) > 0;
  }
  CHECK(seen > 10);
  world_free(world);
}

/* A world where every kind is due over and over, with Fabio walking on so that
 * there is always fresh ground ahead to appear on. */
static world_t* everybody_due(void) {
  world_t* world = fresh();
  world->spawning = true;
  world->player.invulnerable = 1e9f;
  return world;
}

static void keep_everybody_due(world_t* world) {
  world->zombie_timer = fminf(world->zombie_timer, 0.0f);
  world->bug_timer = fminf(world->bug_timer, 0.0f);
  world->book_reader_timer = fminf(world->book_reader_timer, 0.0f);
  world->monk_timer = fminf(world->monk_timer, 0.0f);
  world->chef_timer = fminf(world->chef_timer, 0.0f);
  world->gorilla_timer = fminf(world->gorilla_timer, 0.0f);
}

static int everybody(const world_t* world) {
  return (int)pool_get_active_count(&world->enemies);
}

static void arrivals_come_one_at_a_time_well_apart(void) {
  world_t* world = everybody_due();
  float last = -100.0f, shortest = 100.0f;
  int arrived = 0, before = 0;
  for (int i = 0; i < 60 * 90; i++) {
    keep_everybody_due(world);
    step(world, NULL, 1.0f / 60.0f);
    int now = everybody(world);
    if (now > before) {
      CHECK(now == before + 1); /* never two at once */
      if (world->time - last < shortest) shortest = world->time - last;
      last = world->time;
      arrived++;
    }
    before = now;
    /* Fabio deals with them as they come, so the crowd never holds them back. */
    for (enemy_t* e; (e = first_enemy(world));) pool_release_object(&world->enemies, e);
    before = 0;
  }
  CHECK(arrived >= 12 && arrived <= 90.0f / ARRIVAL_GAP_END);
  CHECK(shortest >= ARRIVAL_GAP_END - 0.05f);
  world_free(world);
}

static void nobody_new_turns_up_while_fabio_is_crowded(void) {
  world_t* world = everybody_due();
  int most = 0;
  for (int i = 0; i < 120; i++) {
    keep_everybody_due(world);
    step(world, NULL, 1.0f);
    world->player.x += 20.0f; /* he dawdles along */
    world->camera_x += 20.0f;
    size_t cursor = 0;
    int near = 0;
    for (const enemy_t* e; (e = pool_next_active(&world->enemies, &cursor));)
      near += fabsf(e->x - world->player.x) <= world->view_w;
    if (near > most) most = near;
  }
  CHECK(most >= CROWD_START - 1);
  CHECK(most <= world_crowd_limit(world));
  world_free(world);
}

/* The book readers, monks, chefs and gorillas within a screen of Fabio. */
static int specials_near(const world_t* world) {
  int n = 0;
  size_t cursor = 0;
  for (const enemy_t* e; (e = pool_next_active(&world->enemies, &cursor));)
    n += e->kind != ENEMY_ZOMBIE && e->kind != ENEMY_BUG &&
         fabsf(e->x - world->player.x) <= world->view_w;
  return n;
}

static void the_odd_characters_come_far_apart_and_few_at_a_time(void) {
  world_t* world = everybody_due();
  world->zombie_timer = world->bug_timer = 1e9f;
  float last = -100.0f, shortest = 100.0f;
  int before = 0, most = 0, arrived = 0;
  for (int i = 0; i < 60 * 120; i++) {
    world->book_reader_timer = fminf(world->book_reader_timer, 0.0f);
    world->monk_timer = fminf(world->monk_timer, 0.0f);
    world->gorilla_timer = fminf(world->gorilla_timer, 0.0f);
    step(world, NULL, 1.0f / 60.0f);
    world->player.x += 0.5f;
    world->camera_x += 0.5f;
    int now = (int)pool_get_active_count(&world->enemies);
    if (now > before) { /* only odd characters are due: every arrival is one */
      if (world->time - last < shortest) shortest = world->time - last;
      last = world->time;
      arrived++;
    }
    if (specials_near(world) > most) most = specials_near(world);
    before = now;
  }
  CHECK(arrived >= 2);
  CHECK(shortest >= SPECIAL_GAP - 0.05f);
  CHECK(most <= SPECIALS_MAX);
  world_free(world);
}

static void the_kind_that_has_waited_longest_goes_first(void) {
  world_t* world = everybody_due();
  world->zombie_timer = -1.0f;
  world->bug_timer = -3.0f; /* overdue the longest */
  world->book_reader_timer = world->monk_timer = world->chef_timer = world->gorilla_timer = 1e9f;
  step(world, NULL, 0.05f);
  CHECK(world_count_enemies(world, ENEMY_BUG) == 1);
  CHECK(world_count_enemies(world, ENEMY_ZOMBIE) == 0);
  world_free(world);
}

/* Whether every column under the whole width of e was solid ground at its feet
 * where it arrived, `dt` ago (walkers have taken their first step since). */
static bool fully_supported(const world_t* world, const enemy_t* e, float dt) {
  int row = (int)(e->y / TILE);
  float x = e->x - e->vx * dt;
  for (int col = (int)floorf((x - e->w * 0.5f) / TILE);
       col <= (int)floorf((x + e->w * 0.5f) / TILE); col++)
    if (!tile_is_solid(level_tile(&world->level, col, row))) return false;
  return true;
}

static void everybody_arrives_with_ground_under_all_of_him(void) {
  world_t* world = everybody_due();
  /* Ground in runs of one to three tiles with one-tile gaps between them. */
  for (int col = 14; col < LEVEL_COLS; col++)
    if (col % 4 == 0 || (col % 11 == 5)) {
      for (int row = GROUND_ROW; row < LEVEL_ROWS; row++) world->level.tiles[row][col] = TILE_EMPTY;
    }
  bool was_active[MAX_ENEMIES] = {false};
  int checked = 0;
  for (int i = 0; i < 60 * 240; i++) {
    keep_everybody_due(world);
    world->arrival_wait = fminf(world->arrival_wait, 0.0f);
    step(world, NULL, 1.0f / 60.0f);
    for (size_t slot = 0; slot < MAX_ENEMIES; slot++) {
      bool active = pool_is_active(&world->enemies, slot);
      const enemy_t* e = pool_get_at(&world->enemies, slot);
      bool flier = e->kind == ENEMY_BUG && (e->variant == BUG_FLY || e->variant == BUG_BEE);
      bool from_a_pit = e->y > (float)(GROUND_ROW * TILE);
      if (active && !was_active[slot] && !flier && !from_a_pit) {
        if (!fully_supported(world, e, 1.0f / 60.0f)) fprintf(stderr, "kind %d at %.0f, w %.0f\n", e->kind, e->x, e->w);
        CHECK(fully_supported(world, e, 1.0f / 60.0f));
        checked++;
      }
      was_active[slot] = active;
    }
    /* Clear the way so that more keep coming, and walk on. */
    for (enemy_t* e; (e = first_enemy(world));) pool_release_object(&world->enemies, e);
    memset(was_active, 0, sizeof(was_active));
    world->player.x += 1.5f;
    world->camera_x += 1.5f;
  }
  CHECK(checked >= 40);
  world_free(world);
}

void run_enemy_tests(void) {
  RUN(arrivals_come_one_at_a_time_well_apart);
  RUN(nobody_new_turns_up_while_fabio_is_crowded);
  RUN(the_odd_characters_come_far_apart_and_few_at_a_time);
  RUN(the_kind_that_has_waited_longest_goes_first);
  RUN(everybody_arrives_with_ground_under_all_of_him);
  RUN(zombies_appear_at_regular_intervals_off_screen);
  RUN(a_zombie_walks_towards_fabio_from_either_side);
  RUN(a_zombie_hops_over_a_block_in_its_way);
  RUN(a_zombie_that_walks_into_a_pit_climbs_out_the_far_side);
  RUN(zombies_far_behind_are_forgotten);
  RUN(there_is_a_cap_on_how_many_zombies_chase_him);
  RUN(a_flying_bug_closes_in_and_hovers_near_fabio);
  RUN(ground_bugs_walk_and_the_dark_beetle_hops);
  RUN(bugs_come_with_the_zombies);
}
