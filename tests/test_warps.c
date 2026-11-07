#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "pose.h"
#include "sfx.h"
#include "test_groups.h"
#include "test_support.h"
#include "test_world.h"

#define GROUND ((float)(GROUND_ROW * TILE))

static level_t* level_for(uint32_t seed) {
  level_t* level = calloc(1, sizeof(*level));
  level_generate(level, seed);
  return level;
}

static void stand_on_pipe(world_t* world, int pipe) {
  player_t* p = &world->player;
  p->x = (float)((world->level.warp_col[pipe] + 1) * TILE);
  p->y = (float)(world->level.warp_row[pipe] * TILE);
  p->vx = p->vy = 0.0f;
  p->grounded = true;
  world->camera_x = p->x - 300.0f;
  step(world, NULL, 0.05f); /* keys let go, and he settles on the cap */
}

static void every_level_has_warp_pipes_the_first_just_past_the_start(void) {
  for (uint32_t seed = 1; seed <= 30; seed++) {
    level_t* level = level_for(seed);
    CHECK(level->warp_count >= 2 && level->warp_count <= MAX_WARPS);
    CHECK((float)(level->warp_col[0] * TILE) < level->spawn_x + 30.0f * TILE);
    for (int i = 0; i < level->warp_count; i++) {
      int col = level->warp_col[i], row = level->warp_row[i];
      CHECK(level_tile(level, col, row) == TILE_PIPE_TOP_L && level_tile(level, col + 1, row) == TILE_PIPE_TOP_R);
      CHECK(tile_is_solid(level_tile(level, col, GROUND_ROW)));
    }
    free(level);
  }
}

static void the_level_runs_forest_then_desert_then_snow(void) {
  level_t* level = level_for(4);
  CHECK(level_biome_at(level, 10) == BIOME_FOREST);
  CHECK(level_biome_at(level, DESERT_COL - 1) == BIOME_FOREST && level_biome_at(level, DESERT_COL) == BIOME_DESERT);
  CHECK(level_biome_at(level, SNOW_COL - 1) == BIOME_DESERT && level_biome_at(level, SNOW_COL) == BIOME_SNOW);
  CHECK(level_biome_at(level, CITY_COL - 1) == BIOME_SNOW && level_biome_at(level, 500) == BIOME_CITY);
  free(level);
}

static void each_zone_has_its_own_decorations(void) {
  for (uint32_t seed = 1; seed <= 12; seed++) {
    level_t* level = level_for(seed);
    int desert = 0, snow = 0;
    for (int i = 0; i < level->decor_count; i++) {
      const decor_t* d = &level->decor[i];
      decor_kind_t k = d->kind;
      biome_t zone = level_biome_at(level, (int)(d->x / TILE));
      if (zone == BIOME_FOREST) CHECK(k < DECOR_PROCEDURAL);
      if (zone == BIOME_DESERT) {
        CHECK(k == DECOR_CACTUS || k == DECOR_DEAD_TREE || k == DECOR_BOULDER || k == DECOR_SKULL);
        desert++;
      }
      if (zone == BIOME_SNOW) {
        CHECK(k == DECOR_SNOW_PINE || k == DECOR_ICE || k == DECOR_FROST_BUSH || k == DECOR_SNOWMAN);
        snow++;
      }
    }
    CHECK(desert > 4 && snow > 4);
    free(level);
  }
}

static void the_cave_is_a_closed_treasure_room_with_a_guardian_and_a_pipe_at_each_end(void) {
  level_t* a = calloc(1, sizeof(*a));
  level_t* b = calloc(1, sizeof(*b));
  level_generate_cave(a, 7, 1);
  level_generate_cave(b, 7, 1);
  CHECK(memcmp(a->tiles, b->tiles, sizeof(a->tiles)) == 0 && a->guardian == b->guardian); /* always the same */
  CHECK(a->cave && a->cols == CAVE_COLS && level_biome_at(a, 20) == BIOME_CAVE);
  CHECK(a->warp_count == 2 && a->warp_col[0] < 8 && a->warp_col[1] > CAVE_COLS - 10);
  CHECK(a->guardian == 1 || a->guardian == 2);
  CHECK(a->goal_x > 1.0e6f); /* nothing wins in a cave */
  for (int col = 0; col < CAVE_COLS; col++) CHECK(tile_is_solid(level_tile(a, col, 0)) && tile_is_solid(level_tile(a, col, GROUND_ROW)));
  for (int row = 0; row < GROUND_ROW; row++) CHECK(tile_is_solid(level_tile(a, 1, row)) && tile_is_solid(level_tile(a, CAVE_COLS, row)));
  int blocks = 0, coins = 0, weapons = 0;
  for (int row = 0; row < LEVEL_ROWS; row++)
    for (int col = 0; col < CAVE_COLS; col++)
      if (level_tile(a, col, row) == TILE_QBLOCK) {
        blocks++;
        item_kind_t what = (item_kind_t)a->content[row][col];
        weapons += what == ITEM_LIGHTSABER || what == ITEM_BUBBLE_GUN;
      }
  for (int i = 0; i < a->item_count; i++) coins += a->items[i].kind == ITEM_COIN;
  CHECK(blocks == 3 && weapons >= 1 && coins >= 30);
  for (int i = 0; i < a->decor_count; i++)
    if (a->decor[i].kind == DECOR_STALACTITE) CHECK(a->decor[i].y == (float)(3 * TILE));
  free(a);
  free(b);
}

static void pressing_down_on_a_pipe_cap_sinks_him_into_a_cave_and_the_pipe_there_brings_him_back(void) {
  world_t* world = world_new(5, recording_events());
  world->spawning = false;
  stand_on_pipe(world, 0);
  step(world, NULL, 0.2f);
  CHECK(world->warp.phase == WARP_NONE && !world->level.cave); /* standing on it is not enough */
  step(world, &(buttons_t){BTN_DOWN}, 0.02f);
  CHECK(world->warp.phase == WARP_SINKING);
  float x = world->player.x;
  step(world, NULL, 0.2f);
  CHECK(world->player.x == x && !world->level.cave);
  step(world, NULL, WARP_SINK_SECONDS);
  CHECK(world->level.cave && world->warp.phase == WARP_RISING);
  CHECK(world->player.x == (float)((world->level.warp_col[0] + 1) * TILE));
  CHECK(world_count_enemies(world, ENEMY_GORILLA) + world_count_enemies(world, ENEMY_SLOP) == 1);
  CHECK(world->warp_used[0] && !world->spawning);
  step(world, NULL, WARP_RISE_SECONDS + 0.1f);
  CHECK(world->warp.phase == WARP_NONE && world->player.hp == PLAYER_HP);

  /* ... and the same going back up, out of the pipe he went down. */
  stand_on_pipe(world, 1);
  step(world, &(buttons_t){BTN_DOWN}, 0.02f);
  step(world, NULL, WARP_SINK_SECONDS + WARP_RISE_SECONDS + 0.2f);
  CHECK(!world->level.cave && world->warp.phase == WARP_NONE);
  CHECK(world->player.x == (float)((world->level.warp_col[0] + 1) * TILE));
  CHECK(world->player.y == (float)(world->level.warp_row[0] * TILE));
  CHECK(world_has_event(world, EVENT_WARP));
  /* The pipe has been used: it does nothing a second time. */
  step(world, &(buttons_t){0}, 0.02f);
  step(world, &(buttons_t){BTN_DOWN}, 0.1f);
  CHECK(world->warp.phase == WARP_NONE && !world->level.cave);
  world_free(world);
}

static void the_main_level_is_exactly_as_he_left_it(void) {
  world_t* world = world_new(5, NULL);
  world->spawning = false;
  world->item_taken[0] = true;
  world->score = 1234;
  int drums = world->drum_count;
  float helicopter_x = world->helicopter.x;
  uint8_t before[LEVEL_ROWS][LEVEL_COLS];
  memcpy(before, world->level.tiles, sizeof(before));
  stand_on_pipe(world, 0);
  step(world, &(buttons_t){BTN_DOWN}, 0.02f);
  step(world, NULL, WARP_SINK_SECONDS + WARP_RISE_SECONDS + 0.2f);
  CHECK(world->level.cave && !world->item_taken[0] && world->drum_count == 0);
  stand_on_pipe(world, 1);
  step(world, &(buttons_t){BTN_DOWN}, 0.02f);
  step(world, NULL, WARP_SINK_SECONDS + WARP_RISE_SECONDS + 0.2f);
  CHECK(!world->level.cave);
  CHECK(memcmp(before, world->level.tiles, sizeof(before)) == 0);
  CHECK(world->item_taken[0] && world->score >= 1234 && world->drum_count == drums);
  CHECK(world->helicopter.x == helicopter_x && world->spawning == false);
  world_free(world);
}

static void nothing_hurts_him_on_the_way_down_and_a_cave_cannot_end_the_game(void) {
  world_t* world = world_new(5, NULL);
  world->spawning = false;
  stand_on_pipe(world, 0);
  step(world, &(buttons_t){BTN_DOWN}, 0.02f);
  player_hurt(world, world->player.x - 20.0f);
  CHECK(world->player.hp == PLAYER_HP);
  step(world, NULL, WARP_SINK_SECONDS + WARP_RISE_SECONDS + 0.2f);
  world->player.x = 1.0e6f; /* nowhere near any castle: and yet */
  step(world, NULL, 0.1f);
  CHECK(world->state != STATE_WON);
  world_free(world);
}

static void the_fade_goes_to_black_as_the_world_swaps_and_back(void) {
  world_t* world = world_new(5, NULL);
  world->spawning = false;
  CHECK(warp_fade(world) == 0.0f);
  stand_on_pipe(world, 0);
  step(world, &(buttons_t){BTN_DOWN}, 0.02f);
  CHECK(warp_fade(world) == 0.0f);
  step(world, NULL, WARP_SINK_SECONDS - 0.05f);
  CHECK(warp_fade(world) > 0.8f);
  step(world, NULL, 0.1f); /* swapped: rising out of black */
  CHECK(warp_fade(world) > 0.6f);
  step(world, NULL, 0.5f);
  CHECK(warp_fade(world) == 0.0f);
  world_free(world);
}

static void the_snow_zone_is_icy(void) {
  world_t* quick = new_empty_world();
  settle(quick);
  quick->player.x = 40 * TILE;
  step(quick, &(buttons_t){BTN_RIGHT}, 0.1f);
  float grass_speed = quick->player.vx;
  world_free(quick);
  world_t* slippery = new_empty_world();
  settle(slippery);
  slippery->player.x = (SNOW_COL + 10) * TILE;
  step(slippery, &(buttons_t){BTN_RIGHT}, 0.1f);
  float ice_speed = slippery->player.vx;
  step(slippery, NULL, 0.1f);
  CHECK(ice_speed > 0.0f && ice_speed < grass_speed * 0.6f);
  CHECK(slippery->player.vx > 0.0f); /* still sliding after he lets go */
  world_free(slippery);
}

static void the_new_scenery_has_its_sprites_and_a_sound(void) {
  CHECK(strcmp(kStripSpecs[pose_decor(DECOR_CACTUS, 1).strip].prefix, "decor_cactus") == 0);
  CHECK(pose_decor(DECOR_CACTUS, 4).frame == 1); /* three cacti: it wraps */
  CHECK(strcmp(kStripSpecs[pose_decor(DECOR_STALACTITE, 2).strip].prefix, "decor_stalactite") == 0);
  CHECK(strcmp(kStripSpecs[pose_decor(DECOR_SNOWMAN, 0).strip].prefix, "decor_snowman") == 0);
  CHECK(pose_ground(BIOME_SNOW, false, 2).strip == STRIP_GROUND_SNOW && pose_ground(BIOME_CAVE, true, 1).strip == STRIP_FILL_CAVE);
  CHECK(pose_ground(BIOME_DESERT, false, 5).frame == 1);
  float volume;
  CHECK(sfx_for_event(EVENT_WARP, &volume) == SFX_WARP);
}

void run_warp_tests(void) {
  RUN(every_level_has_warp_pipes_the_first_just_past_the_start);
  RUN(the_level_runs_forest_then_desert_then_snow);
  RUN(each_zone_has_its_own_decorations);
  RUN(the_cave_is_a_closed_treasure_room_with_a_guardian_and_a_pipe_at_each_end);
  RUN(pressing_down_on_a_pipe_cap_sinks_him_into_a_cave_and_the_pipe_there_brings_him_back);
  RUN(the_main_level_is_exactly_as_he_left_it);
  RUN(nothing_hurts_him_on_the_way_down_and_a_cave_cannot_end_the_game);
  RUN(the_fade_goes_to_black_as_the_world_swaps_and_back);
  RUN(the_snow_zone_is_icy);
  RUN(the_new_scenery_has_its_sprites_and_a_sound);
}
