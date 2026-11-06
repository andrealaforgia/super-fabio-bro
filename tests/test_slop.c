#include <math.h>
#include <string.h>

#include "pose.h"
#include "sfx.h"
#include "test_groups.h"
#include "test_support.h"
#include "test_world.h"

#define RELEASE_AT (SLOP_CYCLE - SLOP_THROW_SECONDS + SLOP_RELEASE)
#define GROUND ((float)(GROUND_ROW * TILE))

static world_t* fresh(void) {
  world_t* world = new_empty_world();
  settle(world);
  return world;
}

static banana_t* first_ball(world_t* world) {
  size_t cursor = 0;
  return pool_next_active(&world->bananas, &cursor);
}

static bool effect_live(const world_t* world, fx_kind_t kind) {
  size_t cursor = 0;
  for (const effect_t* fx; (fx = pool_next_active(&world->effects, &cursor));)
    if (fx->kind == kind) return true;
  return false;
}

static void the_slop_monster_stands_where_he_appears_turned_to_fabio(void) {
  world_t* world = fresh();
  world->player.x = 5 * TILE;
  float x = 40 * TILE;
  spawn_slop(world, x, GROUND);
  step(world, NULL, 2.0f);
  CHECK(enemy_at(world, 0)->kind == ENEMY_SLOP);
  CHECK(enemy_at(world, 0)->x == x && enemy_at(world, 0)->facing == -1);
  world->player.x = 60 * TILE;
  step(world, NULL, 0.1f);
  CHECK(enemy_at(world, 0)->facing == 1);
  world_free(world);
}

static void he_drips_winds_up_and_throws_in_turn(void) {
  enemy_t s;
  memset(&s, 0, sizeof(s));
  float at[3] = {0.1f, SLOP_STAND_SECONDS + 0.1f, SLOP_CYCLE - SLOP_THROW_SECONDS + 0.1f};
  slop_act_t seen[3];
  for (int i = 0; i < 3; i++) {
    s.anim_time = at[i];
    seen[i] = slop_act(&s, NULL);
  }
  CHECK(seen[0] == SLOP_STANDING && seen[1] == SLOP_WINDING_UP && seen[2] == SLOP_THROWING);
  s.anim_time = 0.1f;
  CHECK(pose_slop(&s).strip == STRIP_SLOP_IDLE);
  s.anim_time = SLOP_STAND_SECONDS + 0.1f;
  CHECK(pose_slop(&s).strip == STRIP_SLOP_WINDUP);
  s.anim_time = SLOP_CYCLE - 0.01f;
  CHECK(pose_slop(&s).strip == STRIP_SLOP_THROW && pose_slop(&s).frame == 3);
}

static void a_ball_of_slop_leaves_his_hand_at_the_release_and_never_without_a_ball(void) {
  world_t* world = fresh();
  world->player.x = 8 * TILE;
  world->player.invulnerable = 1e9f;
  spawn_slop(world, world->player.x + 400.0f, GROUND);
  step(world, NULL, RELEASE_AT - 0.2f);
  CHECK(first_ball(world) == NULL);
  step(world, NULL, 0.3f);
  banana_t* b = first_ball(world);
  CHECK(b != NULL && b->slop);
  CHECK(world_has_event(world, EVENT_SLOP_THROWN));
  CHECK(!world_has_event(world, EVENT_BANANA_THROWN));
  world_free(world);
}

static void he_says_his_line_at_the_start_of_each_round_and_only_in_view(void) {
  world_t* world = fresh();
  world->player.x = 8 * TILE;
  world->player.invulnerable = 1e9f;
  spawn_slop(world, world->player.x + 400.0f, GROUND);
  step(world, NULL, 0.1f);
  CHECK(world_has_event(world, EVENT_SLOP_SPEAKS));
  world_free(world);

  world = fresh();
  world->player.x = 8 * TILE;
  spawn_slop(world, world->player.x + 2000.0f, GROUND); /* far off screen */
  step(world, NULL, SLOP_CYCLE + 0.5f);
  CHECK(!world_has_event(world, EVENT_SLOP_SPEAKS));
  world_free(world);
}

static void the_ball_costs_a_heart_and_splats(void) {
  world_t* world = fresh();
  banana_t* b = pool_acquire(&world->bananas, NULL);
  *b = (banana_t){world->player.x + 40.0f, world->player.y - PLAYER_H * 0.5f, -400.0f, 0.0f, 0.0f, -1, true};
  step(world, NULL, 0.2f);
  CHECK(world->player.hp == PLAYER_HP - 1);
  CHECK(first_ball(world) == NULL);
  CHECK(effect_live(world, FX_SLOP_SPLAT));
  world_free(world);
}

static void slop_hurts_nobody_but_fabio(void) {
  world_t* world = fresh();
  world->player.x = 5 * TILE;
  spawn_zombie(world, 20 * TILE, GROUND);
  enemy_at(world, 0)->speed = 0.0f;
  banana_t* b = pool_acquire(&world->bananas, NULL);
  *b = (banana_t){enemy_at(world, 0)->x - 60.0f, enemy_at(world, 0)->y - 30.0f, 300.0f, 0.0f, 0.0f, -1, true};
  step(world, NULL, 0.5f);
  CHECK(world_count_enemies(world, ENEMY_ZOMBIE) == 1);
  CHECK(world->zombies_killed == 0);
  world_free(world);
}

static void a_ball_splats_against_a_wall(void) {
  world_t* world = fresh();
  world->player.x = 5 * TILE;
  for (int row = 0; row < GROUND_ROW; row++) world->level.tiles[row][14] = TILE_STONE;
  banana_t* b = pool_acquire(&world->bananas, NULL);
  *b = (banana_t){10 * TILE, GROUND - 80.0f, 400.0f, 0.0f, 0.0f, -1, true};
  step(world, NULL, 0.7f);
  CHECK(first_ball(world) == NULL);
  CHECK(effect_live(world, FX_SLOP_SPLAT));
  world_free(world);
}

static void touching_him_costs_a_heart_and_stomping_him_kills_him(void) {
  world_t* world = fresh();
  spawn_slop(world, world->player.x + 10.0f, GROUND);
  step(world, NULL, 0.05f);
  CHECK(world->player.hp == PLAYER_HP - 1);
  world_free(world);

  world = fresh();
  spawn_slop(world, world->player.x, GROUND);
  world->player.y -= 60.0f;
  world->player.vy = 300.0f;
  step(world, NULL, 0.1f);
  CHECK(world->player.hp == PLAYER_HP);
  CHECK(world_count_enemies(world, ENEMY_SLOP) == 0);
  CHECK(world->slops_killed == 1 && world->score == 450);
  CHECK(world_has_event(world, EVENT_SLOP_DIES));
  world_free(world);
}

static void a_bullet_kills_him_and_he_bursts_into_a_shower_of_slop(void) {
  world_t* world = fresh();
  world->player.ammo = PISTOL_AMMO;
  spawn_slop(world, world->player.x + 250.0f, GROUND);
  step(world, &(buttons_t){BTN_SHOOT}, 0.02f);
  step(world, NULL, 0.5f);
  CHECK(world_count_enemies(world, ENEMY_SLOP) == 0);
  CHECK(effect_live(world, FX_SLOP_DEATH));
  int pieces = 0, slop_pieces = 0;
  size_t cursor = 0;
  for (const debris_t* d; (d = pool_next_active(&world->debris, &cursor));) {
    pieces++;
    slop_pieces += d->kind == DEBRIS_SLOP && d->variant >= 0 && d->variant < SLOP_PIECES;
  }
  CHECK(pieces == SLOP_SHOWER && slop_pieces == SLOP_SHOWER);
  world_free(world);
}

static void slop_monsters_turn_up_ahead_of_fabio_one_at_a_time(void) {
  world_t* world = fresh();
  world->spawning = true;
  world->zombie_timer = 1e9f;
  world->bug_timer = 1e9f;
  world->book_reader_timer = 1e9f;
  world->monk_timer = 1e9f;
  world->chef_timer = 1e9f;
  world->gorilla_timer = 1e9f;
  world->slop_timer = 1.0f;
  step(world, NULL, 3.0f);
  CHECK(world_count_enemies(world, ENEMY_SLOP) == 1);
  CHECK(enemy_at(world, 0)->x > world->camera_x + world->view_w);
  world->slop_timer = 0.0f;
  step(world, NULL, 3.0f);
  CHECK(world_count_enemies(world, ENEMY_SLOP) == SLOP_MAX);
  world_free(world);
}

static void the_slop_monster_has_his_sounds_and_every_frame(void) {
  float volume;
  CHECK(sfx_for_event(EVENT_SLOP_THROWN, &volume) == SFX_SLOP_THROW);
  CHECK(sfx_for_event(EVENT_SLOP_DIES, &volume) == SFX_SLOP_DIES);
  CHECK(sfx_for_event(EVENT_SLOP_SPEAKS, &volume) == SFX_VOICE_SLOP);
  CHECK(kStripSpecs[STRIP_SLOP_DEATH].frames == 8 && kStripSpecs[STRIP_SLOP_BALL].frames == 6);
  CHECK(pose_slop_effect(FX_SLOP_DEATH, 0.79f, 0.8f).frame == 7);
  CHECK(pose_slop_effect(FX_SLOP_SPLAT, 0.0f, 0.6f).strip == STRIP_SLOP_SPLAT);
}

void run_slop_tests(void) {
  RUN(the_slop_monster_stands_where_he_appears_turned_to_fabio);
  RUN(he_drips_winds_up_and_throws_in_turn);
  RUN(a_ball_of_slop_leaves_his_hand_at_the_release_and_never_without_a_ball);
  RUN(he_says_his_line_at_the_start_of_each_round_and_only_in_view);
  RUN(the_ball_costs_a_heart_and_splats);
  RUN(slop_hurts_nobody_but_fabio);
  RUN(a_ball_splats_against_a_wall);
  RUN(touching_him_costs_a_heart_and_stomping_him_kills_him);
  RUN(a_bullet_kills_him_and_he_bursts_into_a_shower_of_slop);
  RUN(slop_monsters_turn_up_ahead_of_fabio_one_at_a_time);
  RUN(the_slop_monster_has_his_sounds_and_every_frame);
}
