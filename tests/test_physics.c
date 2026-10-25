#include <string.h>

#include "test_groups.h"
#include "test_support.h"
#include "test_world.h"

static void a_box_in_the_air_falls_and_lands_on_the_ground(void) {
  world_t* world = new_empty_world();
  float x = 300.0f, y = 100.0f, vx = 0.0f, vy = 0.0f;
  move_result_t result = {0};
  for (int i = 0; i < 240 && !result.grounded; i++) {
    vy += GRAVITY / 120.0f;
    result = physics_move(&world->level, &x, &y, &vx, &vy, 24.0f, 52.0f,
                          1.0f / 120.0f);
  }
  CHECK(result.grounded);
  CHECK_NEAR(y, GROUND_ROW * TILE, 0.01);
  CHECK_NEAR(vy, 0.0, 0.001);
  world_free(world);
}

static void a_wall_stops_a_box_flush_against_it(void) {
  world_t* world = new_empty_world();
  world->level.tiles[GROUND_ROW - 1][20] = TILE_STONE;
  float x = 500.0f, y = GROUND_ROW * TILE, vx = 300.0f, vy = 1.0f;
  move_result_t result = {0};
  for (int i = 0; i < 120; i++)
    result = physics_move(&world->level, &x, &y, &vx, &vy, 24.0f, 52.0f, 1.0f / 120.0f);
  CHECK(result.hit_right || vx == 0.0f);
  CHECK(x + 12.0f <= 20 * TILE + 0.01f);
  CHECK(x + 12.0f >= 20 * TILE - 1.0f);
  world_free(world);
}

static void a_box_rising_into_a_block_reports_the_block(void) {
  world_t* world = new_empty_world();
  world->level.tiles[GROUND_ROW - 4][12] = TILE_QBLOCK;
  float x = 12 * TILE + 16.0f, y = GROUND_ROW * TILE, vx = 0.0f, vy = -700.0f;
  move_result_t hit = {0};
  for (int i = 0; i < 120 && !hit.hit_ceiling; i++) {
    vy += GRAVITY / 120.0f;
    hit = physics_move(&world->level, &x, &y, &vx, &vy, 24.0f, 52.0f, 1.0f / 120.0f);
  }
  CHECK(hit.hit_ceiling);
  CHECK(hit.ceiling_col == 12 && hit.ceiling_row == GROUND_ROW - 4);
  CHECK(vy >= 0.0f);
  world_free(world);
}

static void the_map_edge_is_a_wall(void) {
  world_t* world = new_empty_world();
  float x = 40.0f, y = GROUND_ROW * TILE, vx = -500.0f, vy = 1.0f;
  for (int i = 0; i < 240; i++)
    physics_move(&world->level, &x, &y, &vx, &vy, 24.0f, 52.0f, 1.0f / 120.0f);
  CHECK(x >= 12.0f);
  world_free(world);
}

static void a_fast_fall_never_tunnels_through_the_floor(void) {
  world_t* world = new_empty_world();
  float x = 200.0f, y = 50.0f, vx = 0.0f, vy = MAX_FALL;
  for (int i = 0; i < 600; i++) {
    vy = MAX_FALL;
    physics_move(&world->level, &x, &y, &vx, &vy, 24.0f, 52.0f, 1.0f / 30.0f);
  }
  CHECK(y <= GROUND_ROW * TILE + 0.01f);
  world_free(world);
}

void run_physics_tests(void) {
  RUN(a_box_in_the_air_falls_and_lands_on_the_ground);
  RUN(a_wall_stops_a_box_flush_against_it);
  RUN(a_box_rising_into_a_block_reports_the_block);
  RUN(the_map_edge_is_a_wall);
  RUN(a_fast_fall_never_tunnels_through_the_floor);
}
