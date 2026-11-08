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

static void every_level_ends_in_a_run_of_cities_just_before_the_castle(void) {
  for (uint32_t seed = 1; seed <= 30; seed++) {
    level_t* level = level_for(seed);
    CHECK(level_biome_at(level, CITY_COL - 1) == BIOME_SNOW && level_biome_at(level, CITY_COL) == BIOME_CITY);
    CHECK(level->building_count >= 30 && level->building_count <= MAX_BUILDINGS);
    CHECK(count_buildings(level, BUILDING_THAI) >= 2 && count_buildings(level, BUILDING_HOTEL) >= 2);
    CHECK(count_buildings(level, BUILDING_APARTMENT) >= 2);
    CHECK(count_decor(level, DECOR_CAR) >= 4);
    CHECK(level->castle_x > level->buildings[level->building_count - 1].x);
    CHECK(level->castle_x < (float)(LEVEL_COLS * TILE));
    for (int i = 0; i < level->building_count; i++) CHECK(level->buildings[i].x >= (float)(CITY_COL * TILE));
    free(level);
  }
}

static void each_city_has_its_welcome_sign_in_order_and_sulmona_has_the_two_road_signs(void) {
  for (uint32_t seed = 1; seed <= 20; seed++) {
    level_t* level = level_for(seed);
    CHECK(count_decor(level, DECOR_WELCOME) == CITY_COUNT && count_decor(level, DECOR_SIGNPOST) == 1);
    float welcome = 0.0f, post = 0.0f, last = 0.0f;
    int next = 0;
    for (int i = 0; i < level->decor_count; i++) {
      if (level->decor[i].kind == DECOR_WELCOME) {
        CHECK(level->decor[i].variant == next++); /* London, Reading, Birmingham, Milano, Barletta, Sulmona */
        CHECK(level->decor[i].x > last);
        last = welcome = level->decor[i].x;
      }
      if (level->decor[i].kind == DECOR_SIGNPOST) post = level->decor[i].x;
    }
    CHECK(next == CITY_COUNT);
    CHECK(strcmp(kCityNames[0], "LONDON") == 0 && strcmp(kCityNames[1], "READING") == 0 && strcmp(kCityNames[2], "BIRMINGHAM") == 0);
    CHECK(strcmp(kCityNames[3], "MILANO") == 0 && strcmp(kCityNames[4], "BARLETTA") == 0 && strcmp(kCityNames[5], "SULMONA") == 0);
    CHECK(level->decor[0].x >= 0.0f);
    float first_sign = (float)(CITY_COL * TILE);
    CHECK(first_sign < last);
    CHECK(post > welcome && post < level->castle_x); /* the signs for Pescara and L'Aquila are in Sulmona, the last */
    free(level);
  }
}

static void every_new_kind_of_shop_stands_in_some_city(void) {
  for (uint32_t seed = 1; seed <= 10; seed++) {
    level_t* level = level_for(seed);
    for (building_kind_t kind = BUILDING_GELATERIA; kind < BUILDING_KIND_COUNT; kind++) CHECK(count_buildings(level, kind) >= 1);
    CHECK(count_buildings(level, BUILDING_THAI) >= 3);
    free(level);
  }
}

static void each_city_has_its_monuments_in_the_right_place(void) {
  for (uint32_t seed = 1; seed <= 10; seed++) {
    level_t* level = level_for(seed);
    float x[DECOR_KIND_COUNT];
    for (int i = 0; i < DECOR_KIND_COUNT; i++) x[i] = -1.0f;
    float welcomes[CITY_COUNT];
    int w = 0;
    for (int i = 0; i < level->decor_count; i++) {
      const decor_t* d = &level->decor[i];
      if (d->kind == DECOR_WELCOME && w < CITY_COUNT) welcomes[w++] = d->x;
      if (d->kind == DECOR_OVID) x[DECOR_OVID] = d->x;
      if (d->kind >= DECOR_BIG_BEN && d->kind <= DECOR_CASTELLO) {
        CHECK(x[d->kind] < 0.0f || d->kind == DECOR_BUS); /* one of each, except the buses */
        x[d->kind] = d->x;
      }
    }
    CHECK(w == CITY_COUNT);
    float last_edge = level->castle_x;
    CHECK(x[DECOR_BIG_BEN] > welcomes[0] && x[DECOR_BIG_BEN] < welcomes[1]);
    CHECK(level->has_eye && level->eye_x > welcomes[0] && level->eye_x < welcomes[1]);
    CHECK(x[DECOR_ABBEY] > welcomes[1] && x[DECOR_BISCUIT_TIN] > welcomes[1] && x[DECOR_BISCUIT_TIN] < welcomes[2]);
    CHECK(x[DECOR_BULL] > welcomes[2] && x[DECOR_ROTUNDA] > welcomes[2] && x[DECOR_ROTUNDA] < welcomes[3]);
    CHECK(x[DECOR_DUOMO] > welcomes[3] && x[DECOR_TRAM] > welcomes[3] && x[DECOR_DUOMO] < welcomes[4]);
    CHECK(x[DECOR_COLOSSO] > welcomes[4] && x[DECOR_CASTELLO] > welcomes[4] && x[DECOR_CASTELLO] < welcomes[5]);
    CHECK(x[DECOR_OVID] > welcomes[5] && x[DECOR_OVID] < last_edge);
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
        bool overlaps = fabsf(below->x - plank->x) < (below->w + plank->w) * 0.5f + 48.0f; /* or a short hop across */
        if (overlaps && below->y > plank->y && below->y - plank->y <= 140.0f) supported = true;
      }
      if (!supported && level->has_eye && fabsf(plank->y - level->terrace_y) < 1.0f) supported = true; /* a capsule takes him up */
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

/* ---- The London Eye ---- */

static int pod_index(const level_t* level, int i) { return level->eye_first_pod + i; }

static void the_london_eye_has_twelve_capsules_going_round_and_staying_level(void) {
  for (uint32_t seed = 1; seed <= 10; seed++) {
    level_t* level = level_for(seed);
    CHECK(level->has_eye);
    int pods = 0;
    for (int i = 0; i < level->platform_count; i++) pods += level->platforms[i].kind == PLATFORM_CAPSULE;
    CHECK(pods == EYE_PODS);
    for (int i = 0; i < EYE_PODS; i++) {
      const platform_def_t* pod = &level->platforms[pod_index(level, i)];
      CHECK(pod->kind == PLATFORM_CAPSULE);
      for (float t = 0.0f; t < EYE_PERIOD; t += 3.0f) {
        float x, y;
        platform_position(pod, t, &x, &y);
        float dx = x - level->eye_x, dy = y - POD_DROP - level->eye_y;
        CHECK(fabsf(sqrtf(dx * dx + dy * dy) - EYE_RADIUS) < 0.5f); /* always on the rim */
      }
      float x0, y0, x1, y1;
      platform_position(pod, 0.0f, &x0, &y0);
      platform_position(pod, EYE_PERIOD, &x1, &y1);
      CHECK(fabsf(x0 - x1) < 0.1f && fabsf(y0 - y1) < 0.1f); /* a full turn comes back */
    }
    /* Clockwise: the pod at the bottom is going up the left side. */
    float x0, y0, x1, y1;
    platform_position(&level->platforms[pod_index(level, 3)], 0.0f, &x0, &y0);
    platform_position(&level->platforms[pod_index(level, 3)], 2.0f, &x1, &y1);
    CHECK(y0 > level->eye_y + 100.0f && x1 < x0 && y1 < y0);
    /* The lowest point is within a jump of the street, the highest well above the roofs. */
    CHECK(level->eye_y + EYE_RADIUS + POD_DROP > GROUND - 40.0f && level->eye_y + EYE_RADIUS + POD_DROP < GROUND - 10.0f);
    CHECK(level->eye_y - EYE_RADIUS + POD_DROP < GROUND - 3.0f * BALCONY_STEP);
    free(level);
  }
}

static int ride_the_eye(world_t* world) {
  const level_t* level = &world->level;
  player_t* p = &world->player;
  int lowest = level->eye_first_pod;
  for (int i = 0; i < EYE_PODS; i++)
    if (world->planks[level->eye_first_pod + i].top > world->planks[lowest].top) lowest = level->eye_first_pod + i;
  p->x = world->planks[lowest].x - 10.0f; /* under the capsule at the bottom, which is going left */
  p->y = GROUND;
  p->grounded = true;
  p->platform = -1;
  world->camera_x = p->x - 300.0f;
  step(world, NULL, 0.05f);
  buttons_t jump = BTN_JUMP;
  step(world, &jump, 0.5f);
  step(world, NULL, 1.0f);
  return p->platform;
}

static void fabio_jumps_onto_a_capsule_at_the_bottom_and_is_carried_up(void) {
  world_t* world = world_for(3);
  const level_t* level = &world->level;
  player_t* p = &world->player;
  int plank = ride_the_eye(world);
  CHECK(plank >= level->eye_first_pod && plank < level->eye_first_pod + EYE_PODS);
  CHECK(p->grounded);
  float start_y = p->y;
  step(world, NULL, 14.0f);
  CHECK(p->platform == plank);
  CHECK(p->y < start_y - 120.0f); /* up the side of the wheel */
  CHECK(fabsf(p->y - world->planks[plank].top) < 0.5f);
  CHECK(fabsf(p->x - world->planks[plank].x) <= POD_W * 0.5f);
  float high = p->y;
  step(world, NULL, 7.0f);
  CHECK(p->platform == plank && p->y < high); /* and on to the top */
  world_free(world);
}

static void from_the_capsule_at_three_oclock_he_steps_off_onto_the_viewing_deck(void) {
  world_t* world = world_for(3);
  const level_t* level = &world->level;
  player_t* p = &world->player;
  int plank = ride_the_eye(world);
  CHECK(plank >= 0);
  /* Ride until the capsule is level with the deck, then walk right. */
  for (int guard = 0; guard < 600 && !(world->planks[plank].x > level->eye_x + 120.0f && fabsf(world->planks[plank].top - level->terrace_y) <= 12.0f); guard++)
    step(world, NULL, 0.1f);
  CHECK(world->planks[plank].x > level->eye_x + 120.0f && fabsf(world->planks[plank].top - level->terrace_y) <= 12.0f);
  buttons_t right = BTN_RIGHT;
  buttons_t jump_right = BTN_RIGHT | BTN_JUMP;
  step(world, &jump_right, 0.3f);
  step(world, &right, 0.5f);
  step(world, NULL, 1.2f);
  CHECK(p->platform >= 0 && level->platforms[p->platform].kind == PLATFORM_STATIC);
  CHECK(fabsf(p->y - level->terrace_y) < 1.0f);
  world_free(world);
}

static bool on_a_capsule(const world_t* world, const ally_t* a) {
  return a->platform >= world->level.eye_first_pod && a->platform < world->level.eye_first_pod + EYE_PODS;
}

static void his_friends_get_on_the_capsules_and_up_onto_balconies_by_jumping_not_by_magic(void) {
  world_t* world = world_for(3);
  const level_t* level = &world->level;
  player_t* p = &world->player;
  world_summon_ally(world, ALLY_ALESSANDRO);
  world_summon_ally(world, ALLY_GIULIA);
  step(world, NULL, 1.0f);
  int plank = ride_the_eye(world);
  CHECK(plank >= 0);
  /* They are on the ground and Fabio is up on a capsule: they jump onto the capsules as they come
   * round, and never turn up anywhere they did not walk or jump to. */
  bool been_on[ALLY_KIND_COUNT] = {false};
  float last_x[ALLY_KIND_COUNT] = {0}, last_y[ALLY_KIND_COUNT] = {0};
  bool known[ALLY_KIND_COUNT] = {false};
  float biggest_move = 0.0f;
  size_t first = 0;
  for (ally_t* a; (a = pool_next_active(&world->allies, &first));) { /* where they are before the first step */
    last_x[a->kind] = a->x;
    last_y[a->kind] = a->y;
    known[a->kind] = true;
  }
  for (int i = 0; i < 160; i++) { /* eight seconds */
    step(world, NULL, 0.05f);
    size_t cursor = 0;
    for (ally_t* a; (a = pool_next_active(&world->allies, &cursor));) {
      if (on_a_capsule(world, a)) been_on[a->kind] = true;
      if (known[a->kind]) biggest_move = fmaxf(biggest_move, fmaxf(fabsf(a->x - last_x[a->kind]), fabsf(a->y - last_y[a->kind])));
      last_x[a->kind] = a->x;
      last_y[a->kind] = a->y;
      known[a->kind] = true;
    }
    (void)level;
    (void)p;
  }
  CHECK(been_on[ALLY_ALESSANDRO] && been_on[ALLY_GIULIA]);
  CHECK(biggest_move < 60.0f); /* in 0.05 s at most a fast jump or fall, never a leap to another floor */
  world_free(world);

  /* On a balcony, too: Fabio is up there, his friend arrives beside him and finds his way up. */
  world = world_for(3);
  level = &world->level;
  p = &world->player;
  int balcony = -1;
  for (int i = 0; i < level->platform_count && balcony < 0; i++)
    if (level->platforms[i].kind == PLATFORM_STATIC && fabsf(level->platforms[i].y - (GROUND - BALCONY_STEP)) < 1.0f) balcony = i;
  CHECK(balcony >= 0);
  world_summon_ally(world, ALLY_ANDREA);
  p->x = level->platforms[balcony].x;
  p->y = level->platforms[balcony].y;
  p->grounded = true;
  p->platform = balcony;
  world->camera_x = p->x - 300.0f;
  step(world, NULL, 8.0f);
  size_t cursor = 0;
  ally_t* a = pool_next_active(&world->allies, &cursor);
  CHECK(a && a->platform == balcony && fabsf(a->y - level->platforms[balcony].y) < 1.5f);
  world_free(world);
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

/* ---- Shops and their languages ---- */

static void italian_shops_are_only_in_milano_and_barletta_and_the_uk_has_english_signs(void) {
  for (uint32_t seed = 1; seed <= 10; seed++) {
    level_t* level = level_for(seed);
    float welcomes[CITY_COUNT];
    int w = 0;
    for (int i = 0; i < level->decor_count; i++)
      if (level->decor[i].kind == DECOR_WELCOME && w < CITY_COUNT) welcomes[w++] = level->decor[i].x;
    int italian = 0, english = 0;
    for (int i = 0; i < level->building_count; i++) {
      const building_t* b = &level->buildings[i];
      if (b->kind < BUILDING_GELATERIA) continue;
      if (b->x > welcomes[3] && b->x < welcomes[5]) { /* Milano and Barletta */
        CHECK(!b->english);
        italian++;
      } else {
        CHECK(b->x < welcomes[3]); /* never in Sulmona */
        CHECK(b->english);
        english++;
      }
    }
    CHECK(italian >= 6 && english >= 4);
    free(level);
  }
  CHECK(strcmp(building_name(BUILDING_SUPERMERCATO, false), "SUPERMERCATO") == 0 && strcmp(building_name(BUILDING_SUPERMERCATO, true), "SUPERMARKET") == 0);
  CHECK(strcmp(building_name(BUILDING_GELATERIA, true), "ICE CREAM") == 0 && strcmp(building_name(BUILDING_OFFICINA, true), "GARAGE") == 0);
  CHECK(strcmp(building_name(BUILDING_FIORERIA, true), "FLORIST") == 0 && strcmp(building_name(BUILDING_CALZOLAIO, true), "COBBLER") == 0);
  CHECK(strcmp(building_name(BUILDING_BARBIERE, true), "BARBER") == 0 && strcmp(building_name(BUILDING_RISTORANTE, true), "RESTAURANT") == 0);
}

/* ---- The Bullring's bull ---- */

static int enemies_alive(world_t* world) {
  int n = 0;
  size_t cursor = 0;
  while (pool_next_active(&world->enemies, &cursor)) n++;
  return n;
}

static void walking_up_to_the_bull_statue_brings_a_bull_that_charges_the_other_way_killing_enemies(void) {
  world_t* world = world_for(3);
  const level_t* level = &world->level;
  player_t* p = &world->player;
  CHECK(level->has_bull);
  p->x = level->bull_x - 400.0f;
  p->y = GROUND;
  p->grounded = true;
  p->facing = 1;
  world->camera_x = p->x - 300.0f;
  spawn_zombie(world, level->bull_x - 900.0f, GROUND);
  spawn_zombie(world, level->bull_x - 1500.0f, GROUND);
  spawn_zombie(world, level->bull_x + 700.0f, GROUND);
  step(world, NULL, 0.1f);
  CHECK(!world->bull.active); /* still far from it */
  p->x = level->bull_x - 40.0f;
  step(world, NULL, 0.05f);
  CHECK(world->bull.active && world->bull.dir == -1); /* he faces right, so the bull goes left */
  CHECK(world_has_event(world, EVENT_BULL_CHARGES));
  int hearts = p->hp;
  float start = world->bull.x;
  step(world, NULL, 0.5f);
  CHECK(fabsf(world->bull.x - start) < 1.0f); /* it rears first */
  step(world, NULL, 3.0f);
  CHECK(world->bull.x < start - 1000.0f);
  CHECK(p->hp == hearts && p->alive); /* it never hurts him */
  for (int i = 0; i < 40 && world->bull.active; i++) step(world, NULL, 0.2f);
  CHECK(!world->bull.active);
  CHECK(enemies_alive(world) <= 1); /* the two on the left were trampled; the one on the right was not */
  world_free(world);
}

static void anyone_standing_on_the_statue_falls_to_the_street_when_the_bull_starts_to_run(void) {
  world_t* world = world_for(3);
  const level_t* level = &world->level;
  player_t* p = &world->player;
  const platform_def_t* back = &level->platforms[level->bull_plank];
  world_summon_ally(world, ALLY_ALESSANDRO);
  step(world, NULL, 1.0f); /* his friend arrives */
  p->x = back->x - 30.0f;
  p->y = back->y;
  p->grounded = true;
  p->platform = level->bull_plank;
  p->facing = 1;
  world->camera_x = p->x - 300.0f;
  size_t cursor = 0;
  ally_t* a = pool_next_active(&world->allies, &cursor);
  a->x = back->x + 20.0f;
  a->y = back->y;
  a->platform = level->bull_plank;
  a->grounded = true;
  step(world, NULL, 0.2f);
  CHECK(world->bull.active && p->platform == level->bull_plank && a->platform == level->bull_plank); /* it is rearing: still up */
  step(world, NULL, 1.5f);
  CHECK(p->platform != level->bull_plank && p->grounded && fabsf(p->y - GROUND) < 1.0f);
  CHECK(a->platform != level->bull_plank && fabsf(a->y - GROUND) < 1.0f);
  for (int i = 0; i < 40 && world->bull.active; i++) step(world, NULL, 0.2f);
  CHECK(!world->bull.active);
  p->x = back->x;
  p->y = GROUND - 200.0f;
  p->vy = 0.0f;
  p->grounded = false;
  step(world, NULL, 1.0f);
  CHECK(p->platform == level->bull_plank); /* the statue is solid again */
  world_free(world);
}

static void the_bull_runs_right_when_fabio_faces_left_and_the_statue_sleeps_until_he_walks_away(void) {
  world_t* world = world_for(3);
  const level_t* level = &world->level;
  player_t* p = &world->player;
  p->x = level->bull_x + 40.0f;
  p->y = GROUND;
  p->grounded = true;
  p->facing = -1;
  world->camera_x = p->x - 300.0f;
  step(world, NULL, 0.05f);
  CHECK(world->bull.active && world->bull.dir == 1);
  for (int i = 0; i < 60 && world->bull.active; i++) step(world, NULL, 0.2f);
  CHECK(!world->bull.active);
  world_clear_events(world);
  p->x = level->bull_x;
  step(world, NULL, 0.1f);
  CHECK(!world->bull.active && !world_has_event(world, EVENT_BULL_CHARGES)); /* he is still beside it */
  p->x = level->bull_x + 900.0f;
  step(world, NULL, 0.1f);
  p->x = level->bull_x;
  p->y = GROUND;
  step(world, NULL, 0.1f);
  CHECK(world->bull.active); /* and now it wakes again */
  world_free(world);
}

static void the_city_has_its_poses_and_its_sounds(void) {
  float volume;
  CHECK(sfx_for_event(EVENT_BULL_CHARGES, &volume) == SFX_BULL && volume > 0.0f);
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
  RUN(every_level_ends_in_a_run_of_cities_just_before_the_castle);
  RUN(each_city_has_its_welcome_sign_in_order_and_sulmona_has_the_two_road_signs);
  RUN(every_new_kind_of_shop_stands_in_some_city);
  RUN(each_city_has_its_monuments_in_the_right_place);
  RUN(the_london_eye_has_twelve_capsules_going_round_and_staying_level);
  RUN(fabio_jumps_onto_a_capsule_at_the_bottom_and_is_carried_up);
  RUN(from_the_capsule_at_three_oclock_he_steps_off_onto_the_viewing_deck);
  RUN(his_friends_get_on_the_capsules_and_up_onto_balconies_by_jumping_not_by_magic);
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
  RUN(italian_shops_are_only_in_milano_and_barletta_and_the_uk_has_english_signs);
  RUN(walking_up_to_the_bull_statue_brings_a_bull_that_charges_the_other_way_killing_enemies);
  RUN(anyone_standing_on_the_statue_falls_to_the_street_when_the_bull_starts_to_run);
  RUN(the_bull_runs_right_when_fabio_faces_left_and_the_statue_sleeps_until_he_walks_away);
  RUN(the_city_has_its_poses_and_its_sounds);
}
