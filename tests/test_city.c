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

static world_t* world_for(uint32_t seed) {
  world_t* world = world_new(seed, recording_events());
  world->spawning = false;
  return world;
}

static int count_buildings(const level_t* level, building_kind_t kind) {
  int n = 0;
  for (int i = 0; i < level->building_count; i++) n += level->buildings[i].kind == kind;
  return n;
}

static int count_decor(const level_t* level, decor_kind_t kind) {
  int n = 0;
  for (int i = 0; i < level->decor_count; i++) n += level->decor[i].kind == kind;
  return n;
}

static void every_level_ends_in_the_city_of_sulmona_just_before_the_castle(void) {
  for (uint32_t seed = 1; seed <= 30; seed++) {
    level_t* level = level_for(seed);
    CHECK(level_biome_at(level, CITY_COL - 1) == BIOME_SNOW && level_biome_at(level, CITY_COL) == BIOME_CITY);
    CHECK(level->building_count >= 7);
    CHECK(count_buildings(level, BUILDING_THAI) >= 2 && count_buildings(level, BUILDING_HOTEL) >= 2);
    CHECK(count_buildings(level, BUILDING_APARTMENT) >= 2);
    CHECK(count_decor(level, DECOR_CAR) >= 4);
    CHECK(level->castle_x > level->buildings[level->building_count - 1].x);
    CHECK(level->castle_x < (float)(LEVEL_COLS * TILE));
    for (int i = 0; i < level->building_count; i++) CHECK(level->buildings[i].x >= (float)(CITY_COL * TILE));
    free(level);
  }
}

static void the_welcome_sign_stands_at_the_entrance_and_the_two_road_signs_in_the_city(void) {
  for (uint32_t seed = 1; seed <= 20; seed++) {
    level_t* level = level_for(seed);
    CHECK(count_decor(level, DECOR_WELCOME) == 1 && count_decor(level, DECOR_SIGNPOST) == 1);
    float welcome = 0.0f, post = 0.0f;
    for (int i = 0; i < level->decor_count; i++) {
      if (level->decor[i].kind == DECOR_WELCOME) welcome = level->decor[i].x;
      if (level->decor[i].kind == DECOR_SIGNPOST) post = level->decor[i].x;
    }
    CHECK(welcome >= (float)(CITY_COL * TILE) && welcome < level->buildings[0].x); /* before the first building */
    CHECK(post > welcome && post < level->castle_x);
    free(level);
  }
}

static void ovid_and_the_aqueduct_are_among_the_monuments(void) {
  for (uint32_t seed = 1; seed <= 20; seed++) {
    level_t* level = level_for(seed);
    CHECK(count_decor(level, DECOR_OVID) == 1 && count_decor(level, DECOR_FOUNTAIN) == 1);
    CHECK(level->has_aqueduct);
    CHECK(level->aqueduct_x1 - level->aqueduct_x0 > 600.0f);
    CHECK(level->tap_x < level->aqueduct_x0);
    CHECK(level->spout_count >= 4);
    for (int i = 0; i < level->spout_count; i++)
      CHECK(level->spout_x[i] > level->aqueduct_x0 && level->spout_x[i] < level->aqueduct_x1);
    CHECK(level->aqueduct_deck < GROUND - 100.0f);
    free(level);
  }
}

/* Everything he is meant to stand on can be reached with a jump (a jump rises 156 pixels). */
static void every_balcony_and_roof_can_be_reached_by_jumping(void) {
  for (uint32_t seed = 1; seed <= 20; seed++) {
    level_t* level = level_for(seed);
    int statics = 0;
    for (int i = 0; i < level->platform_count; i++) {
      const platform_def_t* plank = &level->platforms[i];
      if (plank->kind != PLATFORM_STATIC) continue;
      statics++;
      float rise = GROUND - plank->y;
      CHECK(rise > 30.0f && rise <= 3.0f * BALCONY_STEP + 1.0f);
      if (rise <= 140.0f) continue;
      /* Higher up it needs a step below it: another plank or the stone stair, within a jump. */
      bool supported = false;
      for (int j = 0; j < level->platform_count; j++) {
        const platform_def_t* below = &level->platforms[j];
        if (j == i || below->kind != PLATFORM_STATIC) continue;
        bool overlaps = fabsf(below->x - plank->x) < (below->w + plank->w) * 0.5f;
        if (overlaps && below->y > plank->y && below->y - plank->y <= 140.0f) supported = true;
      }
      if (!supported && fabsf(plank->y - level->aqueduct_deck) < 1.0f) {
        int col = (int)(level->aqueduct_x0 / TILE);
        supported = level_tile(level, col, GROUND_ROW - 3) == TILE_STONE;
      }
      CHECK(supported);
    }
    CHECK(statics >= 20);
    free(level);
  }
}

static void fabio_jumps_up_from_the_street_to_a_balcony_then_the_next_and_the_roof(void) {
  world_t* world = world_for(3);
  const level_t* level = &world->level;
  int apartment = -1;
  for (int i = 0; i < level->building_count && apartment < 0; i++)
    if (level->buildings[i].floors == 3) apartment = i;
  CHECK(apartment >= 0);
  const building_t* b = &level->buildings[apartment];
  player_t* p = &world->player;
  p->x = b->x + b->w * 0.5f;
  p->y = GROUND;
  p->grounded = true;
  world->camera_x = p->x - 300.0f;
  step(world, NULL, 0.1f);
  buttons_t jump = BTN_JUMP;
  for (int floor = 1; floor <= 3; floor++) {
    step(world, &jump, 0.7f);
    step(world, NULL, 0.1f);
    CHECK(p->grounded && p->platform >= 0);
    CHECK(fabsf(p->y - (GROUND - (float)floor * BALCONY_STEP)) < 1.0f);
  }
  /* He can walk along the roof. */
  buttons_t right = BTN_RIGHT;
  float x0 = p->x;
  step(world, &right, 0.4f);
  CHECK(p->x > x0 + 30.0f && p->grounded && p->platform >= 0);
  world_free(world);
}

static const platform_def_t* plank_of_vase(const level_t* level, int vase) { return &level->platforms[level->vase_plank[vase]]; }

static void stand_on_the_balcony_of(world_t* world, int vase, float dx) {
  const level_t* level = &world->level;
  player_t* p = &world->player;
  p->x = level->vase_x[vase] + dx;
  p->y = plank_of_vase(level, vase)->y;
  p->vx = p->vy = 0.0f;
  p->grounded = true;
  world->camera_x = p->x - 300.0f;
}

static void balconies_have_vases_standing_on_them(void) {
  for (uint32_t seed = 1; seed <= 20; seed++) {
    level_t* level = level_for(seed);
    CHECK(level->vase_count >= 8 && level->vase_count <= MAX_VASES);
    for (int i = 0; i < level->vase_count; i++) {
      const platform_def_t* plank = plank_of_vase(level, i);
      CHECK(plank->kind == PLATFORM_STATIC);
      CHECK(level->vase_y[i] == plank->y);
      CHECK(fabsf(level->vase_x[i] - plank->x) <= plank->w * 0.5f);
    }
    free(level);
  }
}

static void walking_by_a_vase_on_its_balcony_knocks_it_off_onto_the_enemy_below(void) {
  world_t* world = world_for(2);
  stand_on_the_balcony_of(world, 0, 200.0f); /* well away from it */
  step(world, NULL, 0.2f);
  CHECK(world->vases[0].state == VASE_STANDING);
  spawn_zombie(world, world->level.vase_x[0], GROUND);
  int zombies = world_count_enemies(world, ENEMY_ZOMBIE);
  world_clear_events(world);
  stand_on_the_balcony_of(world, 0, 10.0f);
  step(world, NULL, 0.05f);
  CHECK(world->vases[0].state == VASE_FALLING);
  step(world, NULL, 1.0f);
  CHECK(world->vases[0].state == VASE_SMASHED);
  CHECK(world_count_enemies(world, ENEMY_ZOMBIE) == zombies - 1);
  CHECK(world_has_event(world, EVENT_VASE_SMASHES));
  world_free(world);
}

static void a_vase_that_misses_smashes_on_the_street_and_is_gone_for_good(void) {
  world_t* world = world_for(2);
  stand_on_the_balcony_of(world, 0, 0.0f);
  step(world, NULL, 1.0f);
  CHECK(world->vases[0].state == VASE_SMASHED);
  CHECK(world->player.hp == PLAYER_HP); /* it never hurts him */
  stand_on_the_balcony_of(world, 0, 0.0f);
  step(world, NULL, 0.5f);
  CHECK(world->vases[0].state == VASE_SMASHED);
  world_free(world);
}

static void a_vase_falling_hurts_nobody_but_enemies(void) {
  world_t* world = world_for(2);
  stand_on_the_balcony_of(world, 0, 0.0f);
  step(world, NULL, 0.05f);
  /* Fabio walks away from under it. */
  world->player.invulnerable = 0.0f;
  CHECK(world->vases[0].state == VASE_FALLING);
  world_free(world);
}

static void turn_the_water_on(world_t* world) {
  const level_t* level = &world->level;
  player_t* p = &world->player;
  p->x = level->tap_x;
  p->y = GROUND;
  p->vx = p->vy = 0.0f;
  p->grounded = true;
  world->camera_x = p->x - 300.0f;
  step(world, NULL, 0.1f);
  buttons_t up = BTN_UP | BTN_JUMP; /* the up key is the jump key too */
  step(world, &up, 0.03f);
  step(world, NULL, 0.02f);
}

static void up_at_the_tap_turns_the_aqueduct_water_on_for_fourteen_seconds(void) {
  world_t* world = world_for(5);
  CHECK(!water_running(world));
  world_clear_events(world);
  turn_the_water_on(world);
  CHECK(water_running(world));
  CHECK(world_has_event(world, EVENT_WATER_ON));
  CHECK(world->water_left > WATER_SECONDS - 0.5f && world->water_left <= WATER_SECONDS);
  step(world, NULL, WATER_SECONDS - 0.5f);
  CHECK(water_running(world));
  step(world, NULL, 1.0f);
  CHECK(!water_running(world));
  world_free(world);
}

static void up_anywhere_else_does_nothing(void) {
  world_t* world = world_for(5);
  player_t* p = &world->player;
  p->x = world->level.tap_x + 3.0f * TILE;
  p->y = GROUND;
  p->grounded = true;
  step(world, NULL, 0.1f);
  buttons_t up = BTN_UP;
  step(world, &up, 0.05f);
  CHECK(!water_running(world));
  CHECK(!fabio_at_tap(world));
  world_free(world);
}

static void pressing_up_again_while_the_water_runs_does_not_restart_it(void) {
  world_t* world = world_for(5);
  turn_the_water_on(world);
  step(world, NULL, 5.0f);
  float left = world->water_left;
  buttons_t up = BTN_UP;
  step(world, &up, 0.03f);
  step(world, NULL, 0.02f);
  CHECK(world->water_left < left);
  world_free(world);
}

static void the_water_washes_away_the_enemies_under_the_falling_curtains(void) {
  world_t* world = world_for(5);
  turn_the_water_on(world);
  world_clear_events(world);
  spawn_zombie(world, world->level.spout_x[1], GROUND);
  spawn_zombie(world, world->level.spout_x[3], GROUND);
  spawn_zombie(world, world->level.aqueduct_x1 + 300.0f, GROUND); /* dry, beyond the arches */
  CHECK(world_count_enemies(world, ENEMY_ZOMBIE) == 3);
  step(world, NULL, 0.1f);
  CHECK(world_count_enemies(world, ENEMY_ZOMBIE) == 1);
  CHECK(world->score > 0);
  world_free(world);
}

static void with_the_water_off_the_arches_are_harmless(void) {
  world_t* world = world_for(5);
  spawn_zombie(world, world->level.spout_x[1], GROUND);
  world->player.x = world->level.spout_x[1] + 40.0f;
  step(world, NULL, 0.1f);
  CHECK(world_count_enemies(world, ENEMY_ZOMBIE) == 1);
  world_free(world);
}

static void the_water_comes_with_the_game_state_after_a_restart_and_a_pipe_trip(void) {
  world_t* world = world_for(5);
  turn_the_water_on(world);
  world_restart(world, 5);
  CHECK(!water_running(world));
  CHECK(world->vases[0].state == VASE_STANDING);
  world_free(world);
}

static void the_city_has_its_poses_and_its_sounds(void) {
  float volume;
  CHECK(sfx_for_event(EVENT_VASE_SMASHES, &volume) == SFX_VASE && volume > 0.0f);
  CHECK(sfx_for_event(EVENT_WATER_ON, &volume) == SFX_WATER && volume > 0.0f);
  CHECK(pose_masseuse(0, 0.0f).strip == STRIP_THAI_WINDOW && pose_masseuse(1, 0.0f).strip == STRIP_THAI_WINDOW_B);
  /* The masseuses keep animating: all six of her frames come round. */
  bool seen[6] = {false};
  for (float t = 0.0f; t < 6.0f; t += 0.1f) seen[pose_masseuse(0, t).frame] = true;
  for (int i = 0; i < 6; i++) CHECK(seen[i]);
  CHECK(pose_decor(DECOR_CAR, 3).strip == STRIP_DECOR_CAR && pose_decor(DECOR_OVID, 0).strip == STRIP_DECOR_OVID);
  CHECK(pose_decor(DECOR_FOUNTAIN, 1).strip == STRIP_DECOR_FOUNTAIN);
}

void run_city_tests(void) {
  RUN(every_level_ends_in_the_city_of_sulmona_just_before_the_castle);
  RUN(the_welcome_sign_stands_at_the_entrance_and_the_two_road_signs_in_the_city);
  RUN(ovid_and_the_aqueduct_are_among_the_monuments);
  RUN(every_balcony_and_roof_can_be_reached_by_jumping);
  RUN(fabio_jumps_up_from_the_street_to_a_balcony_then_the_next_and_the_roof);
  RUN(balconies_have_vases_standing_on_them);
  RUN(walking_by_a_vase_on_its_balcony_knocks_it_off_onto_the_enemy_below);
  RUN(a_vase_that_misses_smashes_on_the_street_and_is_gone_for_good);
  RUN(a_vase_falling_hurts_nobody_but_enemies);
  RUN(up_at_the_tap_turns_the_aqueduct_water_on_for_fourteen_seconds);
  RUN(up_anywhere_else_does_nothing);
  RUN(pressing_up_again_while_the_water_runs_does_not_restart_it);
  RUN(the_water_washes_away_the_enemies_under_the_falling_curtains);
  RUN(with_the_water_off_the_arches_are_harmless);
  RUN(the_water_comes_with_the_game_state_after_a_restart_and_a_pipe_trip);
  RUN(the_city_has_its_poses_and_its_sounds);
}
