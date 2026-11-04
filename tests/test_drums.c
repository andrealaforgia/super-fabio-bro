#include <math.h>
#include <string.h>

#include "pose.h"
#include "sfx.h"
#include "test_groups.h"
#include "test_support.h"
#include "test_world.h"

#define GROUND ((float)(GROUND_ROW * TILE))

static world_t* fresh(void) {
  world_t* world = new_empty_world();
  settle(world);
  return world;
}

static void stand_kit_at(world_t* world, float x) {
  world->drums = (drum_kit_t){true, true, x, GROUND, 0.0f, 0.0f, -1};
}

/* Fabio comes down on a kit right under him. */
static world_t* playing(void) {
  world_t* world = fresh();
  stand_kit_at(world, world->player.x);
  world->player.y = GROUND - 120.0f;
  world->player.grounded = false;
  step(world, NULL, 0.3f);
  CHECK(world->drums.playing > 0.0f);
  return world;
}

static int live_words(const world_t* world) {
  int n = 0;
  size_t cursor = 0;
  while (pool_next_active(&world->drum_words, &cursor)) n++;
  return n;
}

static void every_level_has_a_kit_on_open_grass_near_the_start(void) {
  for (uint32_t seed = 1; seed <= 40; seed++) {
    level_t* level = calloc(1, sizeof(*level));
    level_generate(level, seed);
    CHECK(level->has_drums);
    CHECK(level->drums_x > level->spawn_x && level->drums_x < level->goal_x);
    CHECK(level->drums_y == GROUND);
    CHECK(fabsf(level->drums_x - level->helicopter_x) > HELICOPTER_W + DRUM_W);
    int from = (int)floorf((level->drums_x - DRUM_W * 0.5f) / TILE);
    int to = (int)floorf((level->drums_x + DRUM_W * 0.5f) / TILE);
    for (int col = from; col <= to; col++) {
      CHECK(tile_is_solid(level_tile(level, col, GROUND_ROW)));
      for (int row = GROUND_ROW - 3; row < GROUND_ROW; row++)
        CHECK(!tile_is_solid(level_tile(level, col, row)));
    }
    free(level);
  }
}

static void walking_past_the_kit_on_the_ground_does_nothing(void) {
  world_t* world = fresh();
  stand_kit_at(world, world->player.x + 90.0f);
  step(world, &(buttons_t){BTN_RIGHT}, 1.0f);
  CHECK(world->drums.playing == 0.0f);
  world_free(world);
}

static void jumping_into_the_kit_sits_him_at_it_with_no_one_pressing_anything(void) {
  world_t* world = playing();
  CHECK(world_has_event(world, EVENT_DRUMS_START));
  CHECK(world->player.x == world->drums.x);
  float x = world->player.x;
  step(world, &(buttons_t){BTN_RIGHT | BTN_RUN}, 1.0f);
  CHECK(world->player.x == x);
  world_free(world);
}

static void he_plays_for_ten_seconds_and_then_hops_out(void) {
  world_t* world = playing();
  step(world, NULL, 9.0f);
  CHECK(world->drums.playing > 0.0f);
  step(world, NULL, 1.0f);
  CHECK(world->drums.playing == 0.0f);
  CHECK(world->player.alive);
  CHECK(world->player.vy < 0.0f || world->player.y < GROUND);
  step(world, &(buttons_t){BTN_RIGHT}, 0.6f);
  CHECK(world->player.x > world->drums.x);
  world_free(world);
}

static void every_jump_into_the_kit_starts_a_fresh_ten_seconds(void) {
  world_t* world = playing();
  step(world, NULL, 10.2f);
  CHECK(world->drums.playing == 0.0f); /* hopping out of it does not sit him straight back down */
  step(world, &(buttons_t){BTN_RIGHT}, 1.2f);
  CHECK(world->drums.playing == 0.0f);
  world->player.x = world->drums.x;
  world->player.y = GROUND - 120.0f;
  world->player.grounded = false;
  step(world, NULL, 0.3f);
  CHECK(world->drums.playing > DRUM_SECONDS - 0.5f && world->drums.playing <= DRUM_SECONDS);
  step(world, NULL, 4.0f);
  CHECK(world->drums.playing > 5.0f && world->drums.playing < 6.0f);
  world_free(world);
}

static void nothing_hurts_him_while_he_plays(void) {
  world_t* world = playing();
  int hp = world->player.hp;
  player_hurt(world, world->player.x - 30.0f);
  CHECK(world->player.hp == hp);
  world_free(world);
}

static void all_six_words_are_shouted_with_their_sounds(void) {
  world_t* world = playing();
  bool seen[DRUM_WORDS] = {false};
  for (int i = 0; i < 50; i++) {
    step(world, NULL, 0.2f);
    for (int w = 0; w < DRUM_WORDS; w++) seen[w] = seen[w] || world_has_event(world, (event_kind_t)(EVENT_DRUM_BOOM + w));
  }
  for (int w = 0; w < DRUM_WORDS; w++) CHECK(seen[w]);
  CHECK(strcmp(kDrumWords[0], "BOOM") == 0 && strcmp(kDrumWords[1], "BADABOOM") == 0 &&
        strcmp(kDrumWords[2], "TASH") == 0 && strcmp(kDrumWords[3], "BODOBOOOM") == 0 &&
        strcmp(kDrumWords[4], "BOING") == 0 && strcmp(kDrumWords[5], "DISH") == 0);
  for (int w = 0; w < DRUM_WORDS; w++) {
    float volume;
    CHECK(sfx_for_event((event_kind_t)(EVENT_DRUM_BOOM + w), &volume) == (sfx_id_t)(SFX_DRUM_BOOM + w));
  }
  world_free(world);
}

static void a_ring_swells_to_its_full_size_and_is_gone(void) {
  CHECK(drum_word_radius(0.0f) == 0.0f);
  CHECK(drum_word_radius(DRUM_WORD_SECONDS * 0.5f) > 0.5f * DRUM_WORD_RADIUS);
  CHECK(drum_word_radius(DRUM_WORD_SECONDS) == DRUM_WORD_RADIUS);
  world_t* world = playing();
  step(world, NULL, 1.0f);
  CHECK(live_words(world) > 3);
  step(world, NULL, 9.2f + DRUM_WORD_SECONDS);
  CHECK(live_words(world) == 0);
  world_free(world);
}

static void whatever_enters_a_ring_dies_and_what_stays_out_lives(void) {
  world_t* world = playing();
  float x = world->drums.x;
  spawn_zombie(world, x + 200.0f, GROUND);
  spawn_zombie(world, x + 2000.0f, GROUND);
  CHECK(world_count_enemies(world, ENEMY_ZOMBIE) == 2);
  step(world, NULL, 1.4f);
  CHECK(world_count_enemies(world, ENEMY_ZOMBIE) == 1);
  CHECK(world->zombies_killed == 1);
  CHECK(world->score > 0);
  world_free(world);
}

static void the_kit_plays_its_frames_very_quickly(void) {
  world_t* world = playing();
  int changes = 0;
  pose_t last = pose_drums(&world->drums, 0.0f);
  CHECK(last.strip == STRIP_DRUMS_PLAY);
  for (float t = 0.0f; t < 1.0f; t += 0.01f) {
    pose_t now = pose_drums(&world->drums, t);
    changes += now.frame != last.frame;
    last = now;
  }
  CHECK(changes >= 18);
  world->drums.playing = 0.0f;
  CHECK(pose_drums(&world->drums, 3.0f).strip == STRIP_DRUMS_REST);
  world_free(world);
}

void run_drum_tests(void) {
  RUN(every_level_has_a_kit_on_open_grass_near_the_start);
  RUN(walking_past_the_kit_on_the_ground_does_nothing);
  RUN(jumping_into_the_kit_sits_him_at_it_with_no_one_pressing_anything);
  RUN(he_plays_for_ten_seconds_and_then_hops_out);
  RUN(every_jump_into_the_kit_starts_a_fresh_ten_seconds);
  RUN(nothing_hurts_him_while_he_plays);
  RUN(all_six_words_are_shouted_with_their_sounds);
  RUN(a_ring_swells_to_its_full_size_and_is_gone);
  RUN(whatever_enters_a_ring_dies_and_what_stays_out_lives);
  RUN(the_kit_plays_its_frames_very_quickly);
}
