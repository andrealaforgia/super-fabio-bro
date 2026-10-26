#include <string.h>

#include "test_groups.h"
#include "test_support.h"
#include "test_world.h"

static world_t* fresh(void) {
  world_t* world = new_empty_world();
  settle(world);
  world->player.invulnerable = 1000.0f;
  return world;
}

static int live_popups(const world_t* world) {
  return (int)pool_get_active_count(&world->popups);
}

static popup_t* first_popup(world_t* world) {
  size_t cursor = 0;
  return pool_next_active(&world->popups, &cursor);
}

static void a_kill_shows_its_points_where_the_enemy_died(void) {
  world_t* world = fresh();
  spawn_zombie(world, world->player.x + 100.0f, world->player.y);
  float x = enemy_at(world, 0)->x;
  world_kill_enemy(world, enemy_at(world, 0), false);
  CHECK(live_popups(world) == 1);
  CHECK(first_popup(world)->points == 200);
  CHECK(first_popup(world)->x == x);
  world_free(world);
}

static void each_kind_of_kill_shows_what_it_scored(void) {
  world_t* world = fresh();
  spawn_bug(world, BUG_BEETLE, world->player.x + 100.0f, world->player.y);
  world_kill_enemy(world, enemy_at(world, 0), false);
  CHECK(first_popup(world)->points == 100);
  pool_reset(&world->popups);
  spawn_book_reader(world, world->player.x + 100.0f, world->player.y);
  world_kill_enemy(world, first_enemy(world), false);
  CHECK(first_popup(world)->points == 500);
  world_free(world);
}

static void the_points_drift_upwards_then_are_gone(void) {
  world_t* world = fresh();
  spawn_zombie(world, world->player.x + 100.0f, world->player.y);
  world_kill_enemy(world, enemy_at(world, 0), false);
  float start = first_popup(world)->y;
  step(world, NULL, 0.4f);
  CHECK(live_popups(world) == 1);
  CHECK(first_popup(world)->y < start - 20.0f);
  step(world, NULL, 2.0f);
  CHECK(live_popups(world) == 0);
  world_free(world);
}

static void picking_up_a_coin_shows_its_points_too(void) {
  world_t* world = fresh();
  world->level.items[0] = (placement_t){ITEM_COIN, world->player.x, world->player.y - 20.0f};
  world->level.item_count = 1;
  step(world, NULL, 0.1f);
  CHECK(world->coins == 1);
  CHECK(live_popups(world) == 1);
  CHECK(first_popup(world)->points == 100);
  world_free(world);
}

static void points_are_not_shown_twice_for_one_award(void) {
  world_t* world = fresh();
  spawn_zombie(world, world->player.x + 100.0f, world->player.y);
  world_kill_enemy(world, enemy_at(world, 0), false);
  world_kill_enemy(world, enemy_at(world, 0), false);
  CHECK(live_popups(world) == 1);
  CHECK(world->score == 200);
  world_free(world);
}

static void a_restart_clears_the_popups(void) {
  world_t* world = fresh();
  spawn_zombie(world, world->player.x + 100.0f, world->player.y);
  world_kill_enemy(world, enemy_at(world, 0), false);
  world_restart(world, 2);
  CHECK(live_popups(world) == 0);
  world_free(world);
}

void run_score_popup_tests(void) {
  RUN(a_kill_shows_its_points_where_the_enemy_died);
  RUN(each_kind_of_kill_shows_what_it_scored);
  RUN(the_points_drift_upwards_then_are_gone);
  RUN(picking_up_a_coin_shows_its_points_too);
  RUN(points_are_not_shown_twice_for_one_award);
  RUN(a_restart_clears_the_popups);
}
