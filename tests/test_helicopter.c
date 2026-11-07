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

static void rest_helicopter_at(world_t* world, float x) {
  world->helicopter = (helicopter_t){true, x, GROUND};
}

/* Fabio drops onto a helicopter right under him, and is flying. */
static world_t* flying(void) {
  world_t* world = fresh();
  rest_helicopter_at(world, world->player.x);
  world->player.y = GROUND - 200.0f;
  world->player.grounded = false;
  step(world, NULL, 0.6f);
  CHECK(world->player.flying);
  return world;
}

static int live_effects(const world_t* world, fx_kind_t kind) {
  int n = 0;
  size_t cursor = 0;
  for (const effect_t* fx; (fx = pool_next_active(&world->effects, &cursor));)
    n += fx->kind == kind;
  return n;
}

static void every_level_has_one_helicopter_resting_on_open_grass(void) {
  for (uint32_t seed = 1; seed <= 40; seed++) {
    level_t* level = calloc(1, sizeof(*level));
    level_generate(level, seed);
    CHECK(level->has_helicopter);
    CHECK(level->helicopter_x > level->spawn_x && level->helicopter_x < level->goal_x);
    CHECK(level->helicopter_y == GROUND);
    int from = (int)floorf((level->helicopter_x - HELICOPTER_W * 0.5f) / TILE);
    int to = (int)floorf((level->helicopter_x + HELICOPTER_W * 0.5f) / TILE);
    for (int col = from; col <= to; col++) {
      CHECK(tile_is_solid(level_tile(level, col, GROUND_ROW)));
      for (int row = GROUND_ROW - 3; row < GROUND_ROW; row++)
        CHECK(!tile_is_solid(level_tile(level, col, row)));
    }
    free(level);
  }
}

static void a_new_world_has_it_resting_and_fabio_on_foot(void) {
  world_t* world = world_new(7, NULL);
  CHECK(world->helicopter.present);
  CHECK(world->helicopter.x == world->level.helicopter_x);
  CHECK(world->helicopter.y == world->level.helicopter_y);
  CHECK(!world->player.flying);
  world_free(world);
}

static void walking_into_it_from_the_side_does_nothing(void) {
  world_t* world = fresh();
  rest_helicopter_at(world, world->player.x + 90.0f);
  buttons_t right = BTN_RIGHT;
  step(world, &right, 1.5f);
  CHECK(world->player.x > world->helicopter.x + HELICOPTER_W * 0.5f); /* he walked past it */
  CHECK(world->helicopter.present);
  CHECK(!world->player.flying);
  CHECK(world->player.hp == PLAYER_HP);
  world_free(world);
}

static void landing_on_it_from_above_puts_fabio_inside(void) {
  world_t* world = fresh();
  rest_helicopter_at(world, world->player.x);
  world->player.y = GROUND - 200.0f;
  world->player.grounded = false;
  world_clear_events(world);
  step(world, NULL, 0.6f);
  CHECK(world->player.flying);
  CHECK(!world->helicopter.present); /* it is the one he is in */
  CHECK(world->player.fuel > HELICOPTER_FUEL_SECONDS - 1.0f);
  CHECK(world_has_event(world, EVENT_HELICOPTER_BOARDS));
  world_free(world);
}

static void a_jump_from_the_ground_reaches_it(void) {
  world_t* world = fresh();
  rest_helicopter_at(world, world->player.x + 110.0f);
  buttons_t forward_and_up = BTN_RIGHT | BTN_JUMP;
  step(world, &forward_and_up, 1.2f);
  CHECK(world->player.flying);
  world_free(world);
}

static void rising_through_it_does_not_board(void) {
  world_t* world = fresh();
  rest_helicopter_at(world, world->player.x);
  world->player.y = GROUND;
  world->player.vy = -300.0f;
  world->player.grounded = false;
  step(world, NULL, 0.05f);
  CHECK(!world->player.flying);
  CHECK(world->helicopter.present);
  world_free(world);
}

static void flying_he_hovers_climbs_and_sinks_at_will(void) {
  world_t* world = flying();
  float y = world->player.y;
  step(world, NULL, 0.5f);
  CHECK_NEAR(world->player.y, y, 1.0); /* no gravity: he hangs in the air */
  buttons_t up = BTN_JUMP;
  step(world, &up, 0.5f);
  CHECK(world->player.y < y - 60.0f);
  float high = world->player.y;
  buttons_t down = BTN_DOWN;
  step(world, &down, 0.8f);
  CHECK(world->player.y > high + 60.0f);
  world_free(world);
}

static void the_up_arrow_climbs_too(void) {
  world_t* world = flying();
  float y = world->player.y;
  buttons_t up = BTN_UP;
  step(world, &up, 0.5f);
  CHECK(world->player.y < y - 60.0f);
  world_free(world);
}

static void he_cannot_leave_the_top_of_the_screen(void) {
  world_t* world = flying();
  buttons_t up = BTN_JUMP;
  step(world, &up, 5.0f);
  CHECK_NEAR(world->player.y, HELICOPTER_CEILING, 0.5);
  world_free(world);
}

static void he_flies_sideways_and_faces_the_way_he_goes(void) {
  world_t* world = flying();
  float x = world->player.x;
  buttons_t left = BTN_LEFT;
  step(world, &left, 1.0f);
  CHECK(world->player.x < x - 100.0f);
  CHECK(world->player.facing == -1);
  buttons_t right = BTN_RIGHT;
  step(world, &right, 1.0f);
  CHECK(world->player.facing == 1);
  world_free(world);
}

static void he_can_come_down_to_the_ground_but_not_into_a_pit(void) {
  world_t* world = flying();
  buttons_t down = BTN_DOWN;
  step(world, &down, 4.0f);
  CHECK_NEAR(world->player.y, GROUND, 0.5);
  CHECK(world->player.flying);
  int col = (int)(world->player.x / TILE);
  for (int c = col + 2; c < col + 30; c++) /* a long pit ahead */
    for (int row = GROUND_ROW; row < LEVEL_ROWS; row++) world->level.tiles[row][c] = TILE_EMPTY;
  buttons_t over_the_pit = BTN_RIGHT | BTN_DOWN;
  step(world, &over_the_pit, 3.0f);
  CHECK(world->player.x > (float)((col + 8) * TILE));
  CHECK(world->player.hp == PLAYER_HP && world->player.alive);
  CHECK(world->player.y <= GROUND + 0.5f);
  world_free(world);
}

static void the_fuel_runs_down_one_unit_a_second(void) {
  world_t* world = flying();
  float fuel = world->player.fuel;
  step(world, NULL, 10.0f);
  CHECK_NEAR(world->player.fuel, fuel - 10.0f, 0.1);
  world_free(world);
}

static void after_thirty_seconds_it_explodes_and_fabio_falls_unharmed(void) {
  world_t* world = flying();
  buttons_t up = BTN_JUMP;
  step(world, &up, 1.0f);
  step(world, NULL, HELICOPTER_FUEL_SECONDS - 3.0f);
  CHECK(world->player.flying);
  CHECK(live_effects(world, FX_HELICOPTER_BOOM) == 0);
  float altitude = world->player.y;
  world_clear_events(world);
  step(world, NULL, 3.2f);
  CHECK(!world->player.flying);
  CHECK(world_has_event(world, EVENT_HELICOPTER_EXPLODES));
  CHECK(world->player.y > altitude); /* he is falling */
  step(world, NULL, 2.0f);
  CHECK(world->player.alive);
  CHECK(world->player.hp == PLAYER_HP);
  CHECK(world->state == STATE_PLAYING);
  CHECK(world->player.grounded);
  CHECK_NEAR(world->player.y, GROUND, 0.5);
  world_free(world);
}

static void the_explosion_happens_where_the_helicopter_was_then_clears(void) {
  world_t* world = flying();
  step(world, NULL, HELICOPTER_FUEL_SECONDS - 0.2f);
  float x = world->player.x;
  step(world, NULL, 0.35f);
  CHECK(live_effects(world, FX_HELICOPTER_BOOM) == 1);
  size_t cursor = 0;
  const effect_t* fx = pool_next_active(&world->effects, &cursor);
  CHECK_NEAR(fx->x, x, 2.0);
  step(world, NULL, HELICOPTER_BOOM_SECONDS + 0.2f);
  CHECK(live_effects(world, FX_HELICOPTER_BOOM) == 0);
  CHECK(!world->helicopter.present); /* there is only the one, and it is gone */
  world_free(world);
}

static void nothing_hurts_him_while_flying(void) {
  world_t* world = flying();
  player_hurt(world, world->player.x + 50.0f);
  CHECK(world->player.hp == PLAYER_HP);
  CHECK(world->player.flying);
  CHECK(world->player.invulnerable == 0.0f);
  world_free(world);
}

static int live_skewers(const world_t* world) {
  int n = 0;
  size_t cursor = 0;
  object_pool_t* pool = (object_pool_t*)&world->skewers;
  for (; pool_next_active(pool, &cursor);) n++;
  return n;
}

static skewer_t* first_skewer(world_t* world) {
  size_t cursor = 0;
  return pool_next_active(&world->skewers, &cursor);
}

static void shooting_while_flying_throws_a_skewer_without_ammo(void) {
  world_t* world = flying();
  world->player.ammo = 0;
  buttons_t shoot = BTN_SHOOT;
  step(world, &shoot, 0.02f);
  CHECK(live_skewers(world) == 1);
  CHECK(world->player.ammo == 0);
  world_free(world);
}

static int live_rays(const world_t* world) {
  int n = 0;
  size_t cursor = 0;
  object_pool_t* pool = (object_pool_t*)&world->rays;
  for (; pool_next_active(pool, &cursor);) n++;
  return n;
}

static int live_bullets(const world_t* world) {
  int n = 0;
  size_t cursor = 0;
  object_pool_t* pool = (object_pool_t*)&world->bullets;
  for (; pool_next_active(pool, &cursor);) n++;
  return n;
}

static void with_the_spell_on_the_blast_wins_even_in_the_helicopter(void) {
  world_t* world = flying();
  world->player.spell_time = 5.0f;
  world->player.ammo = 5;
  buttons_t shoot = BTN_SHOOT;
  step(world, &shoot, 0.02f);
  CHECK(live_rays(world) == 1);
  CHECK(live_skewers(world) == 0);
  CHECK(live_bullets(world) == 0);
  world_free(world);
}

static void in_the_helicopter_skewers_win_over_the_pistol(void) {
  world_t* world = flying();
  world->player.ammo = 5;
  buttons_t shoot = BTN_SHOOT;
  step(world, &shoot, 0.02f);
  CHECK(live_skewers(world) == 1);
  CHECK(live_bullets(world) == 0);
  CHECK(world->player.ammo == 5);
  world_free(world);
}

static void on_foot_without_the_spell_the_pistol_is_used(void) {
  world_t* world = fresh();
  world->player.ammo = 5;
  buttons_t shoot = BTN_SHOOT;
  step(world, &shoot, 0.02f);
  CHECK(live_bullets(world) == 1);
  CHECK(live_skewers(world) == 0);
  CHECK(live_rays(world) == 0);
  world_free(world);
}

static void throwing_a_skewer_makes_its_own_silly_noise(void) {
  world_t* world = flying();
  buttons_t shoot = BTN_SHOOT;
  step(world, &shoot, 0.02f);
  CHECK(world_has_event(world, EVENT_SKEWER_THROWN));
  CHECK(!world_has_event(world, EVENT_SHOOT));
  float volume = 1.0f;
  CHECK(sfx_for_event(EVENT_SKEWER_THROWN, &volume) == SFX_SKEWER);
  world_free(world);
}

static void on_foot_there_are_no_skewers(void) {
  world_t* world = fresh();
  world->player.ammo = 0;
  buttons_t shoot = BTN_SHOOT;
  step(world, &shoot, 0.3f);
  CHECK(live_skewers(world) == 0);
  world_free(world);
}

static void the_skewer_leaves_at_45_degrees_levels_out_at_the_top_then_dives_80_more(void) {
  world_t* world = flying();
  world->player.facing = 1;
  buttons_t shoot = BTN_SHOOT;
  step(world, &shoot, 0.01f);
  skewer_t* s = first_skewer(world);
  CHECK(s != NULL);
  CHECK(fabsf(skewer_tilt(s) - 45.0f) < 3.0f);
  float last = skewer_tilt(s);
  float top = s->y;
  for (int i = 0; i < 40 && s->vy < 0.0f; i++) {
    step(world, NULL, 0.01f);
    s = first_skewer(world);
    CHECK(s != NULL);
    CHECK(skewer_tilt(s) <= last + 0.001f); /* only ever turns towards flat */
    last = skewer_tilt(s);
    top = fminf(top, s->y);
  }
  step(world, NULL, 0.1f);
  s = first_skewer(world);
  CHECK(s != NULL);
  CHECK(skewer_tilt(s) < 0.0f); /* past the top it keeps turning, nose down */
  CHECK(s->y > top - 1.0f);     /* and it has begun to drop */
  float prev = skewer_tilt(s);
  for (int i = 0; i < 80; i++) {
    step(world, NULL, 0.01f);
    s = first_skewer(world);
    if (!s) break;
    CHECK(skewer_tilt(s) <= prev + 0.001f);
    CHECK(skewer_tilt(s) >= -SKEWER_DIVE - 0.001f);
    prev = skewer_tilt(s);
  }
  CHECK(prev < -60.0f); /* close to straight down by the end */
  world_free(world);
}

static void the_skewer_flies_the_way_he_faces(void) {
  world_t* world = flying();
  world->player.facing = -1;
  buttons_t shoot = BTN_SHOOT;
  step(world, &shoot, 0.01f);
  CHECK(first_skewer(world)->vx < 0.0f);
  world_free(world);
}

static void a_skewer_kills_the_zombie_it_hits(void) {
  world_t* world = flying();
  world->player.facing = 1;
  world->player.y = GROUND; /* low, so the zombie is in the way of the first part of the arc */
  buttons_t shoot = BTN_SHOOT;
  step(world, &shoot, 0.01f);
  skewer_t* s = first_skewer(world);
  spawn_zombie(world, s->x + 30.0f, GROUND);
  int before = world->zombies_killed;
  step(world, NULL, 0.1f);
  CHECK(world->zombies_killed == before + 1);
  world_free(world);
}

static void a_skewer_is_gone_after_its_flight(void) {
  world_t* world = flying();
  buttons_t shoot = BTN_SHOOT;
  step(world, &shoot, 0.01f);
  step(world, NULL, SKEWER_LIFE + 0.2f);
  CHECK(live_skewers(world) == 0);
  world_free(world);
}

static void on_his_last_heart_he_is_still_safe_in_it(void) {
  world_t* world = flying();
  world->player.hp = 1;
  player_hurt(world, world->player.x + 50.0f);
  CHECK(world->player.alive);
  CHECK(world->player.flying);
  world_free(world);
}

static void the_helicopter_comes_back_with_a_new_level(void) {
  world_t* world = flying();
  world_restart(world, 3);
  CHECK(world->helicopter.present);
  CHECK(!world->player.flying);
  world_free(world);
}

static void the_poses_follow_the_helicopter(void) {
  CHECK(pose_player(&(player_t){.alive = true, .flying = true}).strip == STRIP_FAB_HELI);
  CHECK(kStripSpecs[STRIP_FAB_HELI].frames == 3);
  CHECK(kStripSpecs[STRIP_HELI_BOOM].frames == 7);
  CHECK(kStripSpecs[STRIP_HELI_REST].frames == 0);
  int seen = 0;
  for (float t = 0.0f; t < 1.0f; t += 0.01f) {
    pose_t pose = pose_player(&(player_t){.alive = true, .flying = true, .anim_time = t});
    CHECK(pose.strip == STRIP_FAB_HELI);
    seen |= 1 << pose.frame;
  }
  CHECK(seen == 0x7); /* the rotor goes through all three frames */
  CHECK(pose_effect(FX_HELICOPTER_BOOM).strip == STRIP_HELI_BOOM);
  int last = -1;
  for (float age = 0.0f; age < HELICOPTER_BOOM_SECONDS + 0.1f; age += 0.01f) {
    pose_t pose = pose_helicopter_boom(age);
    CHECK(pose.strip == STRIP_HELI_BOOM);
    CHECK(pose.frame == last || pose.frame == last + 1);
    last = pose.frame;
  }
  CHECK(last == 6);
}

/* Up in the air, in the helicopter: he lets go of everything, then holds down and taps jump. */
static void bail_out(world_t* world) {
  buttons_t down = BTN_DOWN, down_jump = BTN_DOWN | BTN_JUMP;
  step(world, &down, 0.03f);
  step(world, &down_jump, 0.02f);
  step(world, NULL, 0.01f);
}

static world_t* flying_high(void) {
  world_t* world = flying();
  world->player.y = GROUND - 250.0f;
  world->player.vy = 0.0f;
  world->player.fuel = HELICOPTER_FUEL_SECONDS;
  step(world, NULL, 0.05f);
  return world;
}

static void jumping_out_of_the_helicopter_high_up_opens_a_parachute(void) {
  world_t* world = flying_high();
  world_clear_events(world);
  bail_out(world);
  CHECK(!world->player.flying && world->player.chute);
  CHECK(world_has_event(world, EVENT_PARACHUTE_OPENS));
  CHECK(!world->helicopter.present); /* the helicopter does not come back */
  world_free(world);
}

static void tapping_jump_or_up_twice_quickly_jumps_out_too(void) {
  for (int key = 0; key < 2; key++) {
    world_t* world = flying_high();
    buttons_t tap = key ? BTN_UP : BTN_JUMP;
    step(world, &tap, 0.04f);
    step(world, NULL, 0.1f);
    CHECK(world->player.flying);
    step(world, &tap, 0.04f);
    CHECK(!world->player.flying && world->player.chute);
    world_free(world);
  }
}

static void jump_alone_still_climbs_and_down_alone_still_sinks(void) {
  world_t* world = flying_high();
  buttons_t jump = BTN_JUMP, down = BTN_DOWN;
  step(world, &jump, 0.3f);
  CHECK(world->player.flying && !world->player.chute);
  step(world, &down, 0.3f);
  CHECK(world->player.flying && !world->player.chute);
  world_free(world);
}

static void under_the_parachute_he_floats_down_slowly(void) {
  world_t* world = flying_high();
  bail_out(world);
  step(world, NULL, 0.5f);
  float y0 = world->player.y;
  step(world, NULL, 1.0f);
  float fell = world->player.y - y0;
  CHECK(world->player.chute);
  CHECK(fell > 60.0f && fell <= PARACHUTE_FALL * 1.02f);
  CHECK(world->player.vy <= PARACHUTE_FALL + 0.5f);
  world_free(world);
}

static void he_steers_under_the_parachute_but_not_as_fast_as_he_walks(void) {
  world_t* world = flying_high();
  bail_out(world);
  buttons_t right = BTN_RIGHT | BTN_RUN;
  float x0 = world->player.x;
  step(world, &right, 0.8f);
  CHECK(world->player.x > x0 + 40.0f);
  CHECK(world->player.vx <= WALK_SPEED * PARACHUTE_DRIFT + 1.0f);
  CHECK(world->player.facing == 1);
  world_free(world);
}

static void he_can_still_shoot_while_parachuting(void) {
  world_t* world = flying_high();
  bail_out(world);
  world->player.ammo = 5;
  buttons_t shoot = BTN_SHOOT;
  step(world, &shoot, 0.3f);
  CHECK(world->player.chute);
  CHECK(world->player.ammo < 5);
  CHECK(world_has_event(world, EVENT_SHOOT));
  CHECK(pose_player(&world->player).strip == STRIP_CHUTE_SHOOT);
  world_free(world);
}

static void touching_down_closes_the_parachute_and_leaves_the_canopy_on_the_ground(void) {
  world_t* world = flying_high();
  bail_out(world);
  step(world, NULL, 4.0f);
  CHECK(world->player.grounded && !world->player.chute);
  CHECK(world->player.landing > 0.0f || live_effects(world, FX_CHUTE_REMAINS) == 1);
  CHECK(live_effects(world, FX_CHUTE_REMAINS) == 1);
  CHECK(world->player.hp == PLAYER_HP); /* a soft landing */
  step(world, NULL, PARACHUTE_REMAINS_SECONDS + 0.2f);
  CHECK(live_effects(world, FX_CHUTE_REMAINS) == 0);
  CHECK(world->player.landing == 0.0f);
  world_free(world);
}

static void jumping_out_just_above_the_ground_needs_no_parachute(void) {
  world_t* world = flying();
  world->player.y = GROUND - 12.0f;
  world->player.vy = 0.0f;
  world_clear_events(world);
  bail_out(world);
  CHECK(!world->player.flying && !world->player.chute);
  CHECK(!world_has_event(world, EVENT_PARACHUTE_OPENS));
  step(world, NULL, 1.0f);
  CHECK(world->player.grounded);
  world_free(world);
}

static void running_out_of_fuel_up_high_opens_the_parachute_too(void) {
  world_t* world = flying_high();
  world->player.fuel = 0.05f;
  step(world, NULL, 0.2f);
  CHECK(!world->player.flying && world->player.chute);
  world_free(world);
}

static void the_parachute_has_its_poses_and_its_sound(void) {
  world_t* world = flying_high();
  bail_out(world);
  CHECK(pose_player(&world->player).strip == STRIP_CHUTE_OPEN);
  step(world, NULL, PARACHUTE_OPEN_SECONDS + 0.1f);
  CHECK(pose_player(&world->player).strip == STRIP_CHUTE_FALL);
  float volume;
  CHECK(sfx_for_event(EVENT_PARACHUTE_OPENS, &volume) == SFX_PARACHUTE && volume > 0.0f);
  world_free(world);
}

void run_helicopter_tests(void) {
  RUN(every_level_has_one_helicopter_resting_on_open_grass);
  RUN(a_new_world_has_it_resting_and_fabio_on_foot);
  RUN(walking_into_it_from_the_side_does_nothing);
  RUN(landing_on_it_from_above_puts_fabio_inside);
  RUN(a_jump_from_the_ground_reaches_it);
  RUN(rising_through_it_does_not_board);
  RUN(flying_he_hovers_climbs_and_sinks_at_will);
  RUN(the_up_arrow_climbs_too);
  RUN(he_cannot_leave_the_top_of_the_screen);
  RUN(he_flies_sideways_and_faces_the_way_he_goes);
  RUN(he_can_come_down_to_the_ground_but_not_into_a_pit);
  RUN(the_fuel_runs_down_one_unit_a_second);
  RUN(after_thirty_seconds_it_explodes_and_fabio_falls_unharmed);
  RUN(the_explosion_happens_where_the_helicopter_was_then_clears);
  RUN(nothing_hurts_him_while_flying);
  RUN(shooting_while_flying_throws_a_skewer_without_ammo);
  RUN(with_the_spell_on_the_blast_wins_even_in_the_helicopter);
  RUN(in_the_helicopter_skewers_win_over_the_pistol);
  RUN(on_foot_without_the_spell_the_pistol_is_used);
  RUN(throwing_a_skewer_makes_its_own_silly_noise);
  RUN(on_foot_there_are_no_skewers);
  RUN(the_skewer_leaves_at_45_degrees_levels_out_at_the_top_then_dives_80_more);
  RUN(the_skewer_flies_the_way_he_faces);
  RUN(a_skewer_kills_the_zombie_it_hits);
  RUN(a_skewer_is_gone_after_its_flight);
  RUN(on_his_last_heart_he_is_still_safe_in_it);
  RUN(the_helicopter_comes_back_with_a_new_level);
  RUN(the_poses_follow_the_helicopter);
  RUN(jumping_out_of_the_helicopter_high_up_opens_a_parachute);
  RUN(tapping_jump_or_up_twice_quickly_jumps_out_too);
  RUN(jump_alone_still_climbs_and_down_alone_still_sinks);
  RUN(under_the_parachute_he_floats_down_slowly);
  RUN(he_steers_under_the_parachute_but_not_as_fast_as_he_walks);
  RUN(he_can_still_shoot_while_parachuting);
  RUN(touching_down_closes_the_parachute_and_leaves_the_canopy_on_the_ground);
  RUN(jumping_out_just_above_the_ground_needs_no_parachute);
  RUN(running_out_of_fuel_up_high_opens_the_parachute_too);
  RUN(the_parachute_has_its_poses_and_its_sound);
}
