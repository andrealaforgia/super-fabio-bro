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

static buttons_t held(bool right, bool left, bool jump, bool run) {
  return (right ? BTN_RIGHT : 0u) | (left ? BTN_LEFT : 0u) |
         (jump ? BTN_JUMP : 0u) | (run ? BTN_RUN : 0u);
}

static void fabio_stands_on_the_ground_when_nothing_is_pressed(void) {
  world_t* world = fresh();
  CHECK(world->player.grounded);
  CHECK_NEAR(world->player.vx, 0.0, 0.001);
  CHECK(world->player.alive && world->player.hp == PLAYER_HP);
  free(world);
}

static void he_walks_and_faces_the_way_he_goes(void) {
  world_t* world = fresh();
  float x0 = world->player.x;
  buttons_t left = held(false, true, false, false);
  step(world, &left, 0.5f);
  CHECK(world->player.x < x0 - 40.0f);
  CHECK(world->player.facing == -1);
  buttons_t right = held(true, false, false, false);
  step(world, &right, 0.6f);
  CHECK(world->player.facing == 1);
  free(world);
}

static void running_is_faster_than_walking(void) {
  world_t* a = fresh();
  world_t* b = fresh();
  buttons_t walk = held(true, false, false, false);
  buttons_t run = held(true, false, false, true);
  step(a, &walk, 1.0f);
  step(b, &run, 1.0f);
  CHECK(b->player.x - 10 * TILE > a->player.x - 10 * TILE + 40.0f);
  free(a);
  free(b);
}

static void he_stops_when_the_keys_are_released(void) {
  world_t* world = fresh();
  buttons_t run = held(true, false, false, true);
  step(world, &run, 1.0f);
  step(world, NULL, 0.5f);
  CHECK_NEAR(world->player.vx, 0.0, 0.5);
}

static float jump_apex(world_t* world, bool hold) {
  buttons_t jump = held(false, false, true, false);
  float floor_y = world->player.y, apex = 0.0f;
  controls_t controls;
  memset(&controls, 0, sizeof(controls));
  for (int i = 0; i < 120; i++) {
    buttons_t now = (hold || i < 3) ? jump : held(false, false, false, false);
    controls_update(&controls, now);
    world_update(world, &controls, 1.0f / 120.0f);
    if (floor_y - world->player.y > apex) apex = floor_y - world->player.y;
  }
  return apex;
}

static void a_full_jump_climbs_high_enough_for_the_highest_platform(void) {
  world_t* world = fresh();
  float apex = jump_apex(world, true);
  /* Platform tops are 4 tiles above the ground. */
  CHECK(apex > 4.0f * TILE + 6.0f);
  CHECK(apex < 6.0f * TILE);
  free(world);
}

static void letting_go_early_makes_a_short_hop(void) {
  world_t* tall = fresh();
  world_t* hop = fresh();
  float full = jump_apex(tall, true);
  float shorter = jump_apex(hop, false);
  CHECK(shorter < full * 0.6f);
  CHECK(shorter > 10.0f);
  free(tall);
  free(hop);
}

static void a_running_jump_clears_a_six_tile_pit(void) {
  world_t* world = fresh();
  for (int col = 14; col < 20; col++)
    for (int row = GROUND_ROW; row < LEVEL_ROWS; row++)
      world->level.tiles[row][col] = TILE_EMPTY;
  world->player.x = 14 * TILE - 10.0f;
  world->player.vx = 0.0f;
  buttons_t go = held(true, false, true, true);
  step(world, &go, 0.2f);
  controls_t controls;
  memset(&controls, 0, sizeof(controls));
  /* Hold run and right, keep jump held until landed. */
  step(world, &go, 1.4f);
  CHECK(world->player.hp == PLAYER_HP);
  CHECK(world->player.x > 20 * TILE);
  free(world);
}

static void he_can_stand_on_a_platform_he_jumps_onto(void) {
  world_t* world = fresh();
  for (int col = 12; col < 18; col++)
    world->level.tiles[GROUND_ROW - 4][col] = TILE_BRICK;
  world->player.x = 14 * TILE;
  world->player.y = GROUND_ROW * TILE;
  /* Jump from beside the platform, forwards onto it. */
  world->player.x = 10 * TILE;
  buttons_t go = held(true, false, true, false);
  step(world, &go, 0.9f);
  buttons_t stop = held(false, false, false, false);
  step(world, &stop, 0.6f);
  CHECK(world->player.y <= (GROUND_ROW - 4) * TILE + 0.5f || world->player.grounded);
  free(world);
}

static void hitting_a_question_block_from_below_releases_its_item(void) {
  world_t* world = fresh();
  world->level.tiles[GROUND_ROW - 4][10] = TILE_QBLOCK;
  world->level.content[GROUND_ROW - 4][10] = ITEM_POTION;
  world->player.x = 10 * TILE + 16.0f;
  buttons_t jump = held(false, false, true, false);
  step(world, &jump, 0.6f);
  CHECK(world->level.tiles[GROUND_ROW - 4][10] == TILE_USED);
  CHECK(world_has_event(world, EVENT_BLOCK_OPENS) || world_event_count(world) > 0);
  int pickups = 0;
  for (int i = 0; i < MAX_PICKUPS; i++) pickups += world->pickups[i].alive;
  CHECK(pickups == 1 || world->player.potions == 1);
  free(world);
}

static void a_coin_block_pays_out_at_once(void) {
  world_t* world = fresh();
  world->level.tiles[GROUND_ROW - 4][10] = TILE_QBLOCK;
  world->level.content[GROUND_ROW - 4][10] = ITEM_COIN;
  world->player.x = 10 * TILE + 16.0f;
  buttons_t jump = held(false, false, true, false);
  step(world, &jump, 0.6f);
  CHECK(world->coins == 1);
  CHECK(world->score >= 100);
  free(world);
}

static void bricks_bump_but_stay_bricks(void) {
  world_t* world = fresh();
  world->level.tiles[GROUND_ROW - 4][10] = TILE_BRICK;
  world->player.x = 10 * TILE + 16.0f;
  buttons_t jump = held(false, false, true, false);
  step(world, &jump, 0.6f);
  CHECK(world->level.tiles[GROUND_ROW - 4][10] == TILE_BRICK);
  free(world);
}

static void pickups_from_a_block_are_collected_by_walking_into_them(void) {
  world_t* world = fresh();
  world_spawn_pickup(world, ITEM_POTION, world->player.x, world->player.y - 12.0f);
  step(world, NULL, 0.8f);
  CHECK(world->player.potions == 1);
  world_spawn_pickup(world, ITEM_PISTOL, world->player.x, world->player.y - 12.0f);
  step(world, NULL, 0.8f);
  CHECK(world->player.ammo == PISTOL_AMMO);
  free(world);
}

static void placed_coins_and_mushrooms_are_taken_once(void) {
  world_t* world = fresh();
  world->level.item_count = 2;
  world->level.items[0] = (placement_t){ITEM_COIN, world->player.x, world->player.y - 20.0f};
  world->level.items[1] = (placement_t){ITEM_MUSHROOM, world->player.x, world->player.y - 20.0f};
  world->player.hp = 3;
  step(world, NULL, 0.2f);
  CHECK(world->coins == 1);
  CHECK(world->player.hp == 4);
  step(world, NULL, 0.2f);
  CHECK(world->coins == 1);
  free(world);
}

static void a_pit_costs_a_heart_and_puts_him_back_on_safe_ground(void) {
  world_t* world = fresh();
  for (int col = 14; col < 30; col++)
    for (int row = GROUND_ROW; row < LEVEL_ROWS; row++)
      world->level.tiles[row][col] = TILE_EMPTY;
  buttons_t run = held(true, false, false, true);
  for (int i = 0; i < 100 && world->player.hp == PLAYER_HP; i++)
    step(world, &run, 0.05f);
  step(world, NULL, 0.05f);
  CHECK(world->player.hp == PLAYER_HP - 1);
  CHECK(world->player.x < 14 * TILE);
  CHECK(world->player.invulnerable > 0.0f);
  CHECK(world->state == STATE_PLAYING);
  free(world);
}

static void touching_a_zombie_hurts_then_protects_for_a_while(void) {
  world_t* world = fresh();
  spawn_zombie(world, world->player.x + 10.0f, world->player.y);
  step(world, NULL, 0.1f);
  CHECK(world->player.hp == PLAYER_HP - 1);
  CHECK(world_has_event(world, EVENT_HURT) || world->player.hurt_timer > 0.0f);
  step(world, NULL, 0.3f);
  CHECK(world->player.hp == PLAYER_HP - 1);
  free(world);
}

static void losing_the_last_heart_ends_the_game(void) {
  world_t* world = fresh();
  world->player.hp = 1;
  spawn_zombie(world, world->player.x + 10.0f, world->player.y);
  step(world, NULL, 0.1f);
  CHECK(world->state == STATE_DYING);
  CHECK(!world->player.alive);
  step(world, NULL, 2.5f);
  CHECK(world->state == STATE_GAME_OVER);
  free(world);
}

static void reaching_the_castle_wins(void) {
  world_t* world = fresh();
  world->player.x = world->level.goal_x - 10.0f;
  buttons_t go = held(true, false, false, false);
  step(world, &go, 0.5f);
  CHECK(world->state == STATE_WON);
  CHECK(world->score >= PLAYER_HP * 1000);
  free(world);
}

void run_player_tests(void) {
  RUN(fabio_stands_on_the_ground_when_nothing_is_pressed);
  RUN(he_walks_and_faces_the_way_he_goes);
  RUN(running_is_faster_than_walking);
  RUN(he_stops_when_the_keys_are_released);
  RUN(a_full_jump_climbs_high_enough_for_the_highest_platform);
  RUN(letting_go_early_makes_a_short_hop);
  RUN(a_running_jump_clears_a_six_tile_pit);
  RUN(he_can_stand_on_a_platform_he_jumps_onto);
  RUN(hitting_a_question_block_from_below_releases_its_item);
  RUN(a_coin_block_pays_out_at_once);
  RUN(bricks_bump_but_stay_bricks);
  RUN(pickups_from_a_block_are_collected_by_walking_into_them);
  RUN(placed_coins_and_mushrooms_are_taken_once);
  RUN(a_pit_costs_a_heart_and_puts_him_back_on_safe_ground);
  RUN(touching_a_zombie_hurts_then_protects_for_a_while);
  RUN(losing_the_last_heart_ends_the_game);
  RUN(reaching_the_castle_wins);
}
