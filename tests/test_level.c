#include <string.h>

#include "level.h"
#include "test_groups.h"
#include "test_support.h"

static level_t level;

static bool has_ground(int col) {
  return tile_is_solid(level_tile(&level, col, GROUND_ROW));
}

static void generation_is_deterministic(void) {
  static level_t other;
  level_generate(&level, 1234);
  level_generate(&other, 1234);
  CHECK(memcmp(&level, &other, sizeof(level)) == 0);
  level_generate(&other, 4321);
  CHECK(memcmp(&level, &other, sizeof(level)) != 0);
}

static void the_player_starts_on_solid_ground_with_room_to_stand(void) {
  level_generate(&level, 1);
  int col = (int)(level.spawn_x / TILE);
  CHECK(has_ground(col));
  CHECK(!tile_is_solid(level_tile(&level, col, GROUND_ROW - 1)));
  CHECK(!tile_is_solid(level_tile(&level, col, GROUND_ROW - 2)));
  CHECK_NEAR(level.spawn_y, GROUND_ROW * TILE, 0.01);
}

static void the_edges_are_walls(void) {
  level_generate(&level, 1);
  CHECK(level_solid_at(&level, -5.0f, 100.0f));
  CHECK(level_solid_at(&level, level_width() + 5.0f, 100.0f));
  CHECK(!level_solid_at(&level, 100.0f, -50.0f));
  CHECK(!level_solid_at(&level, 100.0f, level_height() + 50.0f));
}

static void no_pit_is_wider_than_a_running_jump_clears(void) {
  for (uint32_t seed = 1; seed <= 20; seed++) {
    level_generate(&level, seed);
    int run = 0;
    for (int col = 0; col < LEVEL_COLS; col++) {
      run = has_ground(col) ? 0 : run + 1;
      CHECK(run <= 6);
    }
  }
}

static void hazards_are_separated_by_room_to_land(void) {
  for (uint32_t seed = 1; seed <= 20; seed++) {
    level_generate(&level, seed);
    int last_pit_end = -100;
    for (int col = 1; col < LEVEL_COLS; col++) {
      if (!has_ground(col - 1) && has_ground(col)) last_pit_end = col;
      if (has_ground(col - 1) && !has_ground(col))
        CHECK(col - last_pit_end >= 6);
    }
  }
}

static void the_level_ends_in_a_castle_on_flat_ground(void) {
  level_generate(&level, 1);
  CHECK(level.castle_x > 0.0f && level.castle_x < level_width());
  CHECK(level.goal_x == level.castle_x);
  int col = (int)(level.castle_x / TILE);
  CHECK(has_ground(col));
  CHECK(!tile_is_solid(level_tile(&level, col, GROUND_ROW - 1)));
}

static void the_first_blocks_hold_the_pistol_and_a_potion(void) {
  level_generate(&level, 99);
  bool pistol = false, potion = false;
  for (int col = 0; col < 30; col++) {
    if (level_tile(&level, col, GROUND_ROW - 4) == TILE_QBLOCK) {
      if (level.content[GROUND_ROW - 4][col] == ITEM_PISTOL) pistol = true;
      if (level.content[GROUND_ROW - 4][col] == ITEM_POTION) potion = true;
    }
  }
  CHECK(pistol && potion);
}

static void potions_turn_up_all_along_the_level(void) {
  for (uint32_t seed = 1; seed <= 10; seed++) {
    level_generate(&level, seed);
    int last = 0;
    for (int col = 0; col < (int)(level.goal_x / TILE) - 8; col++) {
      for (int row = 0; row < LEVEL_ROWS; row++)
        if (level_tile(&level, col, row) == TILE_QBLOCK &&
            level.content[row][col] == ITEM_POTION)
          last = col;
      for (int i = 0; i < level.item_count; i++)
        if (level.items[i].kind == ITEM_POTION && (int)(level.items[i].x / TILE) == col)
          last = col;
      CHECK(col - last < 100);
    }
  }
}

static void every_question_block_says_what_is_inside(void) {
  level_generate(&level, 5);
  int count = 0;
  for (int row = 0; row < LEVEL_ROWS; row++)
    for (int col = 0; col < LEVEL_COLS; col++)
      if (level.tiles[row][col] == TILE_QBLOCK) {
        count++;
        CHECK(level.content[row][col] != ITEM_NONE);
      }
  CHECK(count > 10);
}

static void floating_blocks_can_be_jumped_onto(void) {
  /* A jump reaches about 4.7 tiles; nothing floats higher than a platform
   * reachable from another platform. */
  level_generate(&level, 3);
  for (int col = 0; col < LEVEL_COLS; col++)
    for (int row = 0; row < GROUND_ROW - 4; row++)
      if (level.tiles[row][col] == TILE_BRICK) CHECK(row >= GROUND_ROW - 8);
}

static void items_and_decor_sit_inside_the_world(void) {
  level_generate(&level, 8);
  CHECK(level.item_count > 20 && level.decor_count > 20);
  for (int i = 0; i < level.item_count; i++) {
    CHECK(level.items[i].x > 0 && level.items[i].x < level_width());
    CHECK(level.items[i].y > 0 && level.items[i].y < level_height());
  }
  for (int i = 0; i < level.decor_count; i++)
    CHECK(has_ground((int)(level.decor[i].x / TILE)));
}

static void pipes_stand_on_the_ground_in_pairs(void) {
  int pipes = 0;
  for (uint32_t seed = 1; seed <= 5; seed++) {
    level_generate(&level, seed);
    for (int col = 0; col < LEVEL_COLS - 1; col++)
      for (int row = 0; row < GROUND_ROW; row++)
        if (level_tile(&level, col, row) == TILE_PIPE_TOP_L) {
          pipes++;
          CHECK(level_tile(&level, col + 1, row) == TILE_PIPE_TOP_R);
          CHECK(has_ground(col) && has_ground(col + 1));
          CHECK(level_tile(&level, col, GROUND_ROW - 1) != TILE_EMPTY);
        }
  }
  CHECK(pipes > 8);
}

void run_level_tests(void) {
  RUN(generation_is_deterministic);
  RUN(the_player_starts_on_solid_ground_with_room_to_stand);
  RUN(the_edges_are_walls);
  RUN(no_pit_is_wider_than_a_running_jump_clears);
  RUN(hazards_are_separated_by_room_to_land);
  RUN(the_level_ends_in_a_castle_on_flat_ground);
  RUN(the_first_blocks_hold_the_pistol_and_a_potion);
  RUN(potions_turn_up_all_along_the_level);
  RUN(every_question_block_says_what_is_inside);
  RUN(floating_blocks_can_be_jumped_onto);
  RUN(items_and_decor_sit_inside_the_world);
  RUN(pipes_stand_on_the_ground_in_pairs);
}
