#include <math.h>
#include <string.h>

#include "pose.h"
#include "test_groups.h"
#include "test_support.h"
#include "test_world.h"

#define CAST_AT (DEMON_STAND_SECONDS + DEMON_RELEASE)

static world_t* fresh(void) {
  world_t* world = new_empty_world();
  settle(world);
  return world;
}

static int fireballs(const world_t* world) {
  return (int)pool_get_active_count(&world->fireballs);
}

static fireball_t* first_fireball(world_t* world) {
  size_t cursor = 0;
  return pool_next_active(&world->fireballs, &cursor);
}

static int live_effects(const world_t* world, fx_kind_t kind) {
  int n = 0;
  size_t cursor = 0;
  for (const effect_t* fx; (fx = pool_next_active(&world->effects, &cursor));)
    n += fx->kind == kind;
  return n;
}

static void a_demon_rises_where_the_monk_went_up_in_smoke(void) {
  world_t* world = fresh();
  world->player.invulnerable = 1e9f;
  float x = world->player.x + 300.0f;
  spawn_monk(world, MONK_HOODED, x, world->player.y);
  world_kill_enemy(world, enemy_at(world, 0), false);
  step(world, NULL, DEMON_RISE_DELAY - 0.1f);
  CHECK(world_count_enemies(world, ENEMY_DEMON) == 0); /* not while the blast is at its height */
  step(world, NULL, 0.2f);
  CHECK(world_count_enemies(world, ENEMY_DEMON) == 1);
  const enemy_t* d = first_enemy(world);
  CHECK(d->kind == ENEMY_DEMON);
  CHECK(d->x == x);
  CHECK(d->y == world->player.y);
  step(world, NULL, 3.0f);
  CHECK(world_count_enemies(world, ENEMY_DEMON) == 1); /* one monk, one demon */
  world_free(world);
}

static void he_fades_in_as_the_monk_fades_out(void) {
  world_t* world = fresh();
  spawn_monk(world, MONK_HOODED, world->player.x + 300.0f, world->player.y);
  world_kill_enemy(world, enemy_at(world, 0), false);
  step(world, NULL, DEMON_RISE_DELAY + 0.02f);
  const enemy_t* d = first_enemy(world);
  CHECK(d && d->kind == ENEMY_DEMON);
  CHECK(demon_presence(d) < 0.1f); /* barely there as the smoke thins */
  float before = demon_presence(d);
  step(world, NULL, DEMON_FADE_SECONDS * 0.5f);
  CHECK(demon_presence(d) > before && demon_presence(d) < 1.0f);
  step(world, NULL, DEMON_FADE_SECONDS * 0.5f + 0.05f);
  CHECK(demon_presence(d) == 1.0f);
  size_t cursor = 0;
  bool smoke_left = false;
  for (const effect_t* fx; (fx = pool_next_active(&world->effects, &cursor));)
    smoke_left = smoke_left || fx->kind == FX_MONK_BOOM;
  CHECK(!smoke_left); /* all there just as the monk's smoke has gone */
  world_free(world);
}

static void walking_into_him_while_he_fades_in_does_nothing(void) {
  world_t* world = fresh();
  spawn_demon(world, world->player.x + 10.0f, world->player.y);
  step(world, NULL, DEMON_FADE_SECONDS * 0.5f);
  CHECK(world->player.hp == PLAYER_HP);
  step(world, NULL, DEMON_FADE_SECONDS * 0.5f + 0.05f);
  CHECK(world->player.hp == PLAYER_HP - 1); /* once he is all there, he hurts */
  world_free(world);
}

static void the_skeleton_leaves_no_demon(void) {
  world_t* world = fresh();
  spawn_monk(world, MONK_SKELETON, world->player.x + 300.0f, world->player.y);
  world_kill_enemy(world, enemy_at(world, 0), false);
  step(world, NULL, 2.0f);
  CHECK(world_count_enemies(world, ENEMY_DEMON) == 0);
  world_free(world);
}

static void he_stays_put_and_faces_fabio(void) {
  world_t* world = fresh();
  world->player.invulnerable = 1e9f;
  float x = world->player.x + 300.0f;
  spawn_demon(world, x, world->player.y);
  step(world, NULL, 0.1f);
  CHECK(enemy_at(world, 0)->facing == -1);
  world->player.x += 600.0f;
  world->camera_x += 600.0f;
  step(world, NULL, 1.0f);
  CHECK(enemy_at(world, 0)->facing == 1);
  CHECK(enemy_at(world, 0)->x == x);
  world_free(world);
}

static void he_casts_a_fireball_straight_at_fabio(void) {
  world_t* world = fresh();
  world->player.invulnerable = 1e9f;
  spawn_demon(world, world->player.x + 300.0f, world->player.y);
  step(world, NULL, CAST_AT - 0.05f);
  CHECK(fireballs(world) == 0);
  step(world, NULL, 0.1f);
  CHECK(fireballs(world) == 1);
  CHECK(world_has_event(world, EVENT_DEMON_CASTS));
  const fireball_t* f = first_fireball(world);
  CHECK(f->vx < 0.0f);
  CHECK_NEAR(sqrtf(f->vx * f->vx + f->vy * f->vy), FIREBALL_SPEED, 1.0f);
  float aim_y = world->player.y - PLAYER_H * 0.5f;
  float t = (world->player.x - f->x) / f->vx; /* it gets to him at his middle */
  CHECK_NEAR(f->y + f->vy * t, aim_y, 2.0f);
  world_free(world);
}

static void a_fireball_costs_fabio_a_heart(void) {
  world_t* world = fresh();
  spawn_demon(world, world->player.x + 250.0f, world->player.y);
  step(world, NULL, CAST_AT + 1.2f);
  CHECK(world->player.alive);
  CHECK(world->player.hp == PLAYER_HP - 1);
  CHECK(fireballs(world) == 0);
  world_free(world);
}

static void a_fireball_flies_past_everybody_else_but_not_through_walls(void) {
  world_t* world = fresh();
  world->player.invulnerable = 1e9f;
  float dx = world->player.x + 300.0f;
  spawn_demon(world, dx, world->player.y);
  spawn_monk(world, MONK_HOODED, dx - 120.0f, world->player.y);
  step(world, NULL, CAST_AT + 0.6f);
  CHECK(world_count_enemies(world, ENEMY_MONK) == 1);
  world_free(world);

  world = fresh();
  world->player.invulnerable = 1e9f;
  int col = (int)((world->player.x + 150.0f) / TILE);
  for (int row = 1; row < GROUND_ROW; row++) world->level.tiles[row][col] = TILE_STONE;
  spawn_demon(world, world->player.x + 300.0f, world->player.y);
  step(world, NULL, CAST_AT + 0.6f);
  CHECK(fireballs(world) == 0);
  CHECK(world->player.hp == PLAYER_HP);
  world_free(world);
}

static void no_spell_while_he_is_off_screen(void) {
  world_t* world = fresh();
  spawn_demon(world, world->camera_x + world->view_w + 80.0f, world->player.y);
  step(world, NULL, DEMON_CYCLE * 2.0f);
  CHECK(fireballs(world) == 0);
  world_free(world);
}

static void stomped_or_shot_he_dissolves(void) {
  world_t* world = fresh();
  spawn_demon(world, world->player.x, world->player.y);
  enemy_at(world, 0)->anim_time = DEMON_FADE_SECONDS; /* all there */
  world->player.y -= 50.0f;
  world->player.vy = 300.0f;
  step(world, NULL, 0.1f);
  CHECK(world_count_enemies(world, ENEMY_DEMON) == 0);
  CHECK(world->player.hp == PLAYER_HP);
  CHECK(world->demons_killed == 1);
  CHECK(world->score == 500);
  CHECK(world_has_event(world, EVENT_DEMON_DIES));
  CHECK(live_effects(world, FX_DEMON_DEATH) == 1);
  step(world, NULL, DEMON_DEATH_SECONDS + 0.1f);
  CHECK(live_effects(world, FX_DEMON_DEATH) == 0);
  world_free(world);

  world = fresh();
  world->player.ammo = PISTOL_AMMO;
  spawn_demon(world, world->player.x + 250.0f, world->player.y);
  buttons_t fire = BTN_SHOOT;
  step(world, &fire, 0.02f);
  step(world, NULL, 0.5f);
  CHECK(world_count_enemies(world, ENEMY_DEMON) == 0);
  world_free(world);
}

static void touching_him_hurts(void) {
  world_t* world = fresh();
  spawn_demon(world, world->player.x + 10.0f, world->player.y);
  enemy_at(world, 0)->anim_time = DEMON_FADE_SECONDS; /* all there */
  step(world, NULL, 0.05f);
  CHECK(world->player.hp == PLAYER_HP - 1);
  world_free(world);
}

static void his_frames_follow_what_he_is_doing(void) {
  enemy_t d;
  memset(&d, 0, sizeof(d));
  int seen = 0;
  for (float t = 0.0f; t < DEMON_STAND_SECONDS; t += 0.02f) {
    d.anim_time = t;
    pose_t pose = pose_demon(&d);
    CHECK(pose.strip == STRIP_DEMON_STAND);
    seen |= 1 << pose.frame;
  }
  CHECK(seen == 0xf); /* he shifts through all four */
  int last = -1;
  for (float t = 0.0f; t < DEMON_CAST_SECONDS; t += 0.01f) {
    d.anim_time = DEMON_STAND_SECONDS + t;
    pose_t pose = pose_demon(&d);
    CHECK(pose.strip == STRIP_DEMON_CAST);
    CHECK(pose.frame == last || pose.frame == last + 1);
    last = pose.frame;
  }
  CHECK(last == 3);
  d.anim_time = DEMON_STAND_SECONDS + DEMON_RELEASE + 0.01f; /* the spark leaves his fist */
  CHECK(pose_demon(&d).frame == 2);
  last = -1;
  for (float age = 0.0f; age < DEMON_DEATH_SECONDS + 0.2f; age += 0.01f) {
    pose_t pose = pose_demon_death(age);
    CHECK(pose.frame == last || pose.frame == last + 1);
    last = pose.frame;
  }
  CHECK(last == 7);
}

void run_demon_tests(void) {
  RUN(a_demon_rises_where_the_monk_went_up_in_smoke);
  RUN(he_fades_in_as_the_monk_fades_out);
  RUN(walking_into_him_while_he_fades_in_does_nothing);
  RUN(the_skeleton_leaves_no_demon);
  RUN(he_stays_put_and_faces_fabio);
  RUN(he_casts_a_fireball_straight_at_fabio);
  RUN(a_fireball_costs_fabio_a_heart);
  RUN(a_fireball_flies_past_everybody_else_but_not_through_walls);
  RUN(no_spell_while_he_is_off_screen);
  RUN(stomped_or_shot_he_dissolves);
  RUN(touching_him_hurts);
  RUN(his_frames_follow_what_he_is_doing);
}
