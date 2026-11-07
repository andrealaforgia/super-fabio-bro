#include <math.h>
#include <string.h>

#include "test_groups.h"
#include "test_support.h"
#include "test_world.h"

static void dig_pit(world_t* world, int first, int width) {
  for (int col = first; col < first + width; col++)
    for (int row = GROUND_ROW; row < LEVEL_ROWS; row++) world->level.tiles[row][col] = TILE_EMPTY;
}

static int add_plank(world_t* world, platform_kind_t kind, float x, float y, float w, float range) {
  level_t* level = &world->level;
  int index = level->platform_count++;
  level->platforms[index] = (platform_def_t){kind, x, y, w, range, 4.0f, 1.5707963f};
  platforms_reset(world);
  return index;
}

static void every_level_has_planks_over_pits_and_lifts_beside_ledges(void) {
  int sliders = 0, lifts = 0;
  for (uint32_t seed = 1; seed <= 12; seed++) {
    level_t* level = calloc(1, sizeof(*level));
    level_generate(level, seed);
    for (int i = 0; i < level->platform_count; i++) {
      const platform_def_t* plank = &level->platforms[i];
      float left = plank->x - plank->w * 0.5f, right = plank->x + plank->w * 0.5f;
      if (plank->kind == PLATFORM_SLIDER) {
        sliders++;
        /* Wherever it swings it stays over the gap. */
        for (float x = left - plank->range + 1.0f; x < right + plank->range; x += TILE * 0.5f)
          CHECK(!tile_is_solid(level_tile(level, (int)(x / TILE), GROUND_ROW)));
      } else if (plank->kind == PLATFORM_LIFT) {
        lifts++;
        int ledge = (int)((right + 12.0f) / TILE);
        CHECK(level_tile(level, ledge, GROUND_ROW - 8) == TILE_GOLD);
        CHECK(plank->y - plank->range == (float)((GROUND_ROW - 8) * TILE));
      }
    }
    free(level);
  }
  CHECK(sliders >= 6);
  CHECK(lifts >= 3);
}

static void a_slider_swings_left_and_right_and_a_lift_up_and_down(void) {
  platform_def_t slider = {PLATFORM_SLIDER, 500.0f, 400.0f, 96.0f, 60.0f, 4.0f, 0.0f};
  platform_def_t lift = {PLATFORM_LIFT, 500.0f, 300.0f, 64.0f, 100.0f, 8.0f, 0.0f};
  float x0, y0, x1, y1;
  platform_position(&slider, 0.0f, &x0, &y0);
  platform_position(&slider, 1.0f, &x1, &y1);
  CHECK(x0 == 500.0f && x1 > 555.0f && y0 == 400.0f && y1 == 400.0f);
  platform_position(&slider, 3.0f, &x1, &y1);
  CHECK(x1 < 445.0f);
  platform_position(&lift, 2.0f, &x1, &y1);
  CHECK(x1 == 500.0f && y1 > 395.0f);
  platform_position(&lift, 6.0f, &x1, &y1);
  CHECK(y1 < 205.0f);
}

static void fabio_walks_onto_a_plank_over_a_pit_and_is_carried(void) {
  world_t* world = new_empty_world();
  dig_pit(world, 14, 6);
  /* A still plank, level with the ground and flush with the pit's near edge. */
  int index = add_plank(world, PLATFORM_SLIDER, 15.5f * TILE, (float)(GROUND_ROW * TILE), 96.0f, 0.0f);
  world->player.x = 12.0f * TILE;
  settle(world);
  buttons_t right = BTN_RIGHT;
  step(world, &right, 0.5f);
  CHECK(world->player.platform == index);
  CHECK(world->player.grounded && world->player.alive);
  CHECK(world->player.hp == PLAYER_HP);
  CHECK(world->player.y == world->planks[index].top);
  world_free(world);
}

static void standing_on_a_swinging_slider_he_goes_wherever_it_goes(void) {
  world_t* world = new_empty_world();
  dig_pit(world, 14, 8);
  int index = add_plank(world, PLATFORM_SLIDER, 18.0f * TILE, (float)(GROUND_ROW * TILE), 96.0f, 60.0f);
  world->player.x = world->planks[index].x;
  world->player.y = world->planks[index].top - 10.0f;
  world->player.grounded = false;
  step(world, NULL, 0.3f);
  CHECK(world->player.platform == index);
  float before_plank = world->planks[index].x, before_fabio = world->player.x;
  step(world, NULL, 0.5f);
  CHECK(world->player.platform == index);
  CHECK(fabsf((world->player.x - before_fabio) - (world->planks[index].x - before_plank)) < 0.5f);
  CHECK(fabsf(world->planks[index].x - before_plank) > 10.0f);
  CHECK(world->player.hp == PLAYER_HP);
  world_free(world);
}

static void a_plank_can_be_jumped_up_through_and_stood_on(void) {
  world_t* world = new_empty_world();
  dig_pit(world, 8, 6);
  int index = add_plank(world, PLATFORM_LIFT, 10.0f * TILE, 300.0f, 96.0f, 0.0f);
  world->player.x = 10.0f * TILE;
  world->player.y = (float)(GROUND_ROW * TILE) - 20.0f;
  world->player.vy = -700.0f; /* from below, going up through it */
  world->player.grounded = false;
  buttons_t held_jump = BTN_JUMP;
  step(world, &held_jump, 0.25f);
  CHECK(world->player.platform == -1);
  CHECK(world->player.y < 300.0f);
  step(world, NULL, 1.0f);
  CHECK(world->player.platform == index); /* and he comes down on top of it */
  CHECK(world->player.y == 300.0f);
  world_free(world);
}

static void a_lift_carries_fabio_up_and_he_can_still_jump_off(void) {
  world_t* world = new_empty_world();
  int index = add_plank(world, PLATFORM_LIFT, 10.0f * TILE, 300.0f, 64.0f, 100.0f); /* starts low */
  world->player.x = 10.0f * TILE;
  world->player.y = world->planks[index].top - 40.0f;
  world->player.grounded = false;
  step(world, NULL, 0.4f);
  CHECK(world->player.platform == index);
  float low = world->player.y;
  step(world, NULL, 1.0f);
  CHECK(world->player.y < low - 30.0f && world->player.platform == index);
  buttons_t jump = BTN_JUMP;
  step(world, &jump, 0.1f);
  CHECK(world->player.platform == -1);
  CHECK(world->player.vy < 0.0f || world->player.y < world->planks[index].top);
  world_free(world);
}

static void stepping_off_the_edge_lets_him_fall_in_the_pit(void) {
  world_t* world = new_empty_world();
  dig_pit(world, 8, 8);
  add_plank(world, PLATFORM_SLIDER, 12.0f * TILE, (float)(GROUND_ROW * TILE), 96.0f, 0.0f);
  world->player.x = 12.0f * TILE;
  world->player.y = (float)(GROUND_ROW * TILE);
  world->player.platform = 0;
  step(world, NULL, 0.2f);
  CHECK(world->player.platform == 0);
  buttons_t right = BTN_RIGHT;
  step(world, &right, 0.6f); /* well past the plank's end, and over the pit */
  CHECK(world->player.platform == -1);
  CHECK(world->player.y > (float)(GROUND_ROW * TILE) || world->player.hp < PLAYER_HP);
  world_free(world);
}

static bool on_the_grass(const enemy_t* e) { return e->y <= (float)(GROUND_ROW * TILE) + 1.0f; }

static void walkers_that_fall_into_a_pit_leap_out_of_it(void) {
  int kinds[3] = {0, 1, 2};
  for (int k = 0; k < 3; k++) {
    for (int side = 0; side < 2; side++) {
      world_t* world = new_empty_world();
      dig_pit(world, 20, 3);
      world->player.invulnerable = 1e9f;
      world->player.x = side ? 5.0f * TILE : 40.0f * TILE; /* far off, either way */
      float x = 21.5f * TILE;
      if (kinds[k] == 0) spawn_zombie(world, x, level_height() - 12.0f);
      else if (kinds[k] == 1) spawn_monk(world, MONK_SKELETON, x, level_height() - 12.0f);
      else spawn_bug(world, BUG_BEETLE, x, level_height() - 12.0f);
      step(world, NULL, 2.5f);
      CHECK(world_count_enemies(world, ENEMY_ZOMBIE) + world_count_enemies(world, ENEMY_MONK) +
                world_count_enemies(world, ENEMY_BUG) == 1);
      CHECK(on_the_grass(enemy_at(world, 0)));
      world_free(world);
    }
  }
}

static void a_zombie_that_walks_into_a_pit_climbs_out_again(void) {
  world_t* world = new_empty_world();
  dig_pit(world, 20, 4);
  world->player.invulnerable = 1e9f;
  world->player.x = 30.0f * TILE;
  spawn_zombie(world, 17.0f * TILE, (float)(GROUND_ROW * TILE));
  enemy_at(world, 0)->speed = 60.0f;
  bool went_down = false;
  for (int i = 0; i < 600; i++) {
    step(world, NULL, 0.02f);
    went_down = went_down || enemy_at(world, 0)->y > PIT_LEAP_LINE;
  }
  CHECK(went_down);
  CHECK(world_count_enemies(world, ENEMY_ZOMBIE) == 1);
  CHECK(enemy_at(world, 0)->x > 24.0f * TILE); /* and on across to the far side */
  world_free(world);
}

static void zombies_sometimes_jump_out_of_a_pit_ahead_of_fabio(void) {
  world_t* world = new_empty_world();
  dig_pit(world, 18, 3);
  world->spawning = true;
  world->player.x = 10.0f * TILE;
  world->player.invulnerable = 1e9f;
  world->view_w = 960.0f;
  world->camera_x = world->player.x - 400.0f;
  world->bug_timer = world->monk_timer = world->book_reader_timer = world->chef_timer = 1e9f;
  int from_the_pit = 0;
  for (int i = 0; i < 80; i++) {
    world->zombie_timer = 0.01f;
    world->arrival_wait = 0.0f; /* no waiting for a turn: this is about where they come from */
    step(world, NULL, 0.02f);
    size_t cursor = 0;
    for (const enemy_t* e; (e = pool_next_active(&world->enemies, &cursor));)
      if (e->y > PIT_LEAP_LINE && e->kind == ENEMY_ZOMBIE) from_the_pit++;
    if (world_count_enemies(world, ENEMY_ZOMBIE) >= 5)
      for (enemy_t* e; (e = first_enemy(world));) pool_release_object(&world->enemies, e);
  }
  CHECK(from_the_pit > 0);
  world_free(world);
}

static void a_walker_climbs_out_of_a_pit_whose_far_wall_is_four_tiles_high(void) {
  world_t* world = new_empty_world();
  dig_pit(world, 20, 3);
  for (int h = 1; h <= 4; h++) world->level.tiles[GROUND_ROW - h][23] = TILE_STONE; /* stairs up */
  world->player.invulnerable = 1e9f;
  world->player.x = 40.0f * TILE; /* it wants to go that way */
  spawn_zombie(world, 21.5f * TILE, level_height() - 12.0f);
  step(world, NULL, 3.0f);
  CHECK(world_count_enemies(world, ENEMY_ZOMBIE) == 1);
  CHECK(first_enemy(world)->y <= (float)((GROUND_ROW - 4) * TILE) + 1.0f || first_enemy(world)->x > 24.0f * TILE);
  world_free(world);
}

static void the_book_reader_the_monk_and_the_chef_leap_out_too_and_then_stand_still(void) {
  for (int who = 0; who < 3; who++) {
    world_t* world = new_empty_world();
    dig_pit(world, 20, 3);
    world->player.invulnerable = 1e9f;
    world->player.x = 5.0f * TILE;
    float x = 21.5f * TILE, y = level_height() - 12.0f;
    if (who == 0) spawn_book_reader(world, x, y);
    else if (who == 1) spawn_monk(world, MONK_HOODED, x, y);
    else spawn_chef(world, x, y);
    step(world, NULL, 2.5f);
    CHECK(first_enemy(world) != NULL);
    CHECK(on_the_grass(first_enemy(world)));
    float landed = first_enemy(world)->x;
    step(world, NULL, 1.5f);
    CHECK(first_enemy(world)->x == landed);
    world_free(world);
  }
}

void run_platform_tests(void) {
  RUN(a_walker_climbs_out_of_a_pit_whose_far_wall_is_four_tiles_high);
  RUN(the_book_reader_the_monk_and_the_chef_leap_out_too_and_then_stand_still);
  RUN(every_level_has_planks_over_pits_and_lifts_beside_ledges);
  RUN(a_slider_swings_left_and_right_and_a_lift_up_and_down);
  RUN(fabio_walks_onto_a_plank_over_a_pit_and_is_carried);
  RUN(standing_on_a_swinging_slider_he_goes_wherever_it_goes);
  RUN(a_plank_can_be_jumped_up_through_and_stood_on);
  RUN(a_lift_carries_fabio_up_and_he_can_still_jump_off);
  RUN(stepping_off_the_edge_lets_him_fall_in_the_pit);
  RUN(walkers_that_fall_into_a_pit_leap_out_of_it);
  RUN(a_zombie_that_walks_into_a_pit_climbs_out_again);
  RUN(zombies_sometimes_jump_out_of_a_pit_ahead_of_fabio);
}
