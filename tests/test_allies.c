#include <math.h>
#include <stdlib.h>

#include "pose.h"
#include "sfx.h"
#include "test_groups.h"
#include "test_support.h"
#include "test_world.h"

#define GROUND ((float)(GROUND_ROW * TILE))

static world_t* fresh(void) {
  world_t* world = new_empty_world();
  settle(world);
  world->player.facing = 1;
  world->zombie_timer = 99.0f;
  return world;
}

static ally_t* ally_of(world_t* world, ally_kind_t kind) {
  size_t cursor = 0;
  for (ally_t* a; (a = pool_next_active(&world->allies, &cursor));)
    if (a->kind == kind) return a;
  return NULL;
}

static int allies(const world_t* world) { return (int)pool_get_active_count(&world->allies); }

static bool effect_live(const world_t* world, fx_kind_t kind) {
  size_t cursor = 0;
  for (const effect_t* fx; (fx = pool_next_active(&world->effects, &cursor));)
    if (fx->kind == kind) return true;
  return false;
}

static enemy_t* still_zombie(world_t* world, float x) {
  spawn_zombie(world, x, GROUND);
  enemy_t* e = enemy_at(world, (size_t)(world_count_enemies(world, ENEMY_ZOMBIE) - 1));
  e->speed = 0.0f;
  return e;
}

static void the_opening_row_holds_the_three_pickups_and_each_brings_its_friend(void) {
  world_t* world = world_new(5, recording_events());
  bool seen[3] = {false, false, false};
  for (int row = 0; row < LEVEL_ROWS; row++)
    for (int col = 0; col < 30; col++) {
      if (level_tile(&world->level, col, row) != TILE_QBLOCK) continue;
      item_kind_t what = (item_kind_t)world->level.content[row][col];
      seen[0] = seen[0] || what == ITEM_SUSHI;
      seen[1] = seen[1] || what == ITEM_DRAGON;
      seen[2] = seen[2] || what == ITEM_GLOVES;
    }
  CHECK(seen[0] && seen[1] && seen[2]);
  world->spawning = false;
  world_spawn_pickup(world, ITEM_SUSHI, world->player.x, world->player.y - 10.0f);
  world_spawn_pickup(world, ITEM_DRAGON, world->player.x, world->player.y - 10.0f);
  world_spawn_pickup(world, ITEM_GLOVES, world->player.x, world->player.y - 10.0f);
  step(world, NULL, 0.5f);
  CHECK(allies(world) == 3);
  CHECK(ally_of(world, ALLY_ANDREA) && ally_of(world, ALLY_ALESSANDRO) && ally_of(world, ALLY_GIULIA));
  CHECK(world_has_event(world, EVENT_ALLY_ARRIVES));
  world_free(world);
}

static void each_friend_stands_a_few_steps_ahead_of_fabio_at_his_own_distance(void) {
  world_t* world = fresh();
  world_summon_ally(world, ALLY_ANDREA);
  world_summon_ally(world, ALLY_ALESSANDRO);
  world_summon_ally(world, ALLY_GIULIA);
  CHECK(effect_live(world, FX_ALLY_POOF));
  step(world, NULL, 1.2f);
  float fabio = world->player.x;
  float a = ally_of(world, ALLY_ANDREA)->x - fabio, b = ally_of(world, ALLY_ALESSANDRO)->x - fabio,
        c = ally_of(world, ALLY_GIULIA)->x - fabio;
  CHECK(a > 60.0f && a < b && b < c && c < 260.0f);
  CHECK(fabsf(a - ally_offset(ALLY_ANDREA)) < 10.0f);
  /* Fabio turns round: they go round to the other side of him. */
  world->player.facing = -1;
  step(world, NULL, 1.5f);
  CHECK(ally_of(world, ALLY_ANDREA)->x < world->player.x - 60.0f);
  world_free(world);
}

static void a_friend_stays_thirty_seconds_and_then_goes_in_a_poof(void) {
  world_t* world = fresh();
  world_summon_ally(world, ALLY_GIULIA);
  step(world, NULL, ALLY_SECONDS - 0.5f);
  CHECK(allies(world) == 1);
  step(world, NULL, 0.7f);
  CHECK(allies(world) == 0);
  CHECK(effect_live(world, FX_ALLY_POOF));
  world_free(world);
}

static void calling_a_friend_who_is_here_gives_him_a_fresh_thirty_seconds_not_a_twin(void) {
  world_t* world = fresh();
  world_summon_ally(world, ALLY_ANDREA);
  step(world, NULL, 6.0f);
  world_summon_ally(world, ALLY_ANDREA);
  CHECK(allies(world) == 1);
  CHECK(ally_of(world, ALLY_ANDREA)->left > ALLY_SECONDS - 0.1f);
  step(world, NULL, 25.0f); /* well past the first thirty, within the second */
  CHECK(allies(world) == 1);
  world_free(world);
}

static void andrea_strokes_his_belly_and_everything_round_him_goes(void) {
  world_t* world = fresh();
  world_summon_ally(world, ALLY_ANDREA);
  step(world, NULL, 1.0f);
  float x = ally_of(world, ALLY_ANDREA)->x;
  still_zombie(world, x + 70.0f);   /* in the wobble */
  still_zombie(world, x - 60.0f);   /* behind him: that too */
  still_zombie(world, x + 400.0f);  /* well out of it */
  step(world, NULL, 1.2f);
  CHECK(world->zombies_killed == 2 && world_count_enemies(world, ENEMY_ZOMBIE) == 1);
  CHECK(world_has_event(world, EVENT_ALLY_BELLY));
  CHECK(world->score > 0 && world->player.hp == PLAYER_HP);
  world_free(world);
}

static void alessandro_runs_at_an_enemy_and_strikes_it_with_kung_fu(void) {
  world_t* world = fresh();
  world_summon_ally(world, ALLY_ALESSANDRO);
  step(world, NULL, 1.0f);
  float start = ally_of(world, ALLY_ALESSANDRO)->x;
  still_zombie(world, start + 230.0f);
  step(world, NULL, 0.35f);
  CHECK(ally_of(world, ALLY_ALESSANDRO)->x > start + 40.0f); /* he is on his way */
  step(world, NULL, 1.5f);
  CHECK(world->zombies_killed == 1);
  CHECK(world_has_event(world, EVENT_ALLY_KUNGFU) && effect_live(world, FX_ALLY_SLASH) + 1 > 0);
  world_free(world);
}

static void giulia_punches_what_comes_near_her(void) {
  world_t* world = fresh();
  world_summon_ally(world, ALLY_GIULIA);
  step(world, NULL, 1.0f);
  float start = ally_of(world, ALLY_GIULIA)->x;
  still_zombie(world, start + 200.0f);
  still_zombie(world, start + 235.0f);
  step(world, NULL, 3.0f);
  CHECK(world->zombies_killed == 2);
  CHECK(world_has_event(world, EVENT_ALLY_PUNCH));
  world_free(world);
}

static void nothing_that_touches_a_friend_hurts_him_and_the_enemies_still_come_for_fabio(void) {
  world_t* world = fresh();
  world_summon_ally(world, ALLY_GIULIA);
  step(world, NULL, 1.0f);
  ally_t* g = ally_of(world, ALLY_GIULIA);
  g->cooldown = 100.0f; /* she is not going to hit back */
  spawn_zombie(world, g->x, GROUND);
  step(world, NULL, 0.5f);
  CHECK(allies(world) == 1 && world->player.hp == PLAYER_HP);
  world_free(world);
}

static void a_friend_left_far_behind_or_fallen_in_a_pit_turns_up_at_fabios_side(void) {
  world_t* world = fresh();
  world_summon_ally(world, ALLY_ANDREA);
  step(world, NULL, 0.5f);
  world->player.x += 900.0f;
  step(world, NULL, 0.3f);
  CHECK(fabsf(ally_of(world, ALLY_ANDREA)->x - world->player.x) < 200.0f);
  ally_of(world, ALLY_ANDREA)->y = level_height() + 100.0f;
  step(world, NULL, 0.1f);
  CHECK(ally_of(world, ALLY_ANDREA)->y < GROUND + 5.0f);
  world_free(world);
}

static void the_friends_jump_the_holes_in_the_ground(void) {
  for (int kind = 0; kind < ALLY_KIND_COUNT; kind++) {
    world_t* world = fresh();
    for (int col = 20; col < 26; col++) /* a hole six columns wide, the widest there is */
      for (int row = GROUND_ROW; row < LEVEL_ROWS; row++) world->level.tiles[row][col] = TILE_EMPTY;
    world->player.x = 31 * TILE;
    world_summon_ally(world, (ally_kind_t)kind);
    ally_t* a = ally_of(world, (ally_kind_t)kind);
    a->x = 14 * TILE;
    a->y = GROUND;
    a->left = 100.0f;
    float lowest = 0.0f;
    for (int i = 0; i < 100; i++) { /* five seconds, watching how low he ever gets */
      step(world, NULL, 0.05f);
      a = ally_of(world, (ally_kind_t)kind);
      if (a && a->y > lowest) lowest = a->y;
    }
    a = ally_of(world, (ally_kind_t)kind);
    CHECK(a != NULL && a->x > 26 * TILE && a->y <= GROUND + 1.0f);
    CHECK(lowest <= GROUND + 1.0f); /* he never fell into it, and was never fetched back out of it */
    world_free(world);
  }
}

static void the_friends_go_down_a_pipe_with_fabio(void) {
  world_t* world = world_new(5, NULL);
  world->spawning = false;
  world_summon_ally(world, ALLY_ALESSANDRO);
  step(world, NULL, 0.5f);
  player_t* p = &world->player;
  p->x = (float)((world->level.warp_col[0] + 1) * TILE);
  p->y = (float)(world->level.warp_row[0] * TILE);
  p->grounded = true;
  step(world, NULL, 0.05f);
  step(world, &(buttons_t){BTN_DOWN}, 0.02f);
  step(world, NULL, WARP_SINK_SECONDS + 0.1f);
  CHECK(world->level.cave && allies(world) == 1);
  CHECK(fabsf(ally_of(world, ALLY_ALESSANDRO)->x - p->x) < 260.0f);
  world_free(world);
}

static void the_friends_have_their_poses_and_their_sounds(void) {
  world_t* world = fresh();
  world_summon_ally(world, ALLY_ALESSANDRO);
  ally_t* a = ally_of(world, ALLY_ALESSANDRO);
  a->grounded = true;
  a->vx = 0.0f;
  CHECK(pose_ally(a).strip == STRIP_ALLY_ALESSANDRO_IDLE);
  a->vx = 150.0f;
  CHECK(pose_ally(a).strip == STRIP_ALLY_ALESSANDRO_WALK);
  a->grounded = false;
  CHECK(pose_ally(a).strip == STRIP_ALLY_ALESSANDRO_JUMP);
  a->grounded = true;
  a->attack = 0.2f;
  CHECK(pose_ally(a).strip == STRIP_ALLY_ALESSANDRO_ATTACK);
  ally_t g = {ALLY_GIULIA, 0, 0, 0, 0, 1, true, 5.0f, 1.0f, 0.0f, 0.0f, 0.0f, false, -1};
  CHECK(pose_ally(&g).strip == STRIP_ALLY_GIULIA_IDLE);
  CHECK(pose_ally_item(ALLY_ANDREA, 0.0f).strip == STRIP_ITEM_SUSHI && pose_ally_item(ALLY_GIULIA, 0.0f).strip == STRIP_ITEM_GLOVES);
  CHECK(pose_ally_effect(FX_ALLY_PUNCH, 0.1f, 0.35f).strip == STRIP_ALLY_GIULIA_FX);
  float volume;
  CHECK(sfx_for_event(EVENT_ALLY_ARRIVES, &volume) == SFX_ALLY_ARRIVE);
  CHECK(sfx_for_event(EVENT_ALLY_BELLY, &volume) == SFX_ALLY_BELLY);
  CHECK(sfx_for_event(EVENT_ALLY_KUNGFU, &volume) == SFX_ALLY_KUNGFU);
  CHECK(sfx_for_event(EVENT_ALLY_PUNCH, &volume) == SFX_ALLY_PUNCH);
  world_free(world);
}

static void levels_offer_the_three_pickups_now_and_then(void) {
  int found[3] = {0, 0, 0};
  for (uint32_t seed = 1; seed <= 20; seed++) {
    level_t* level = calloc(1, sizeof(*level));
    level_generate(level, seed);
    for (int row = 0; row < LEVEL_ROWS; row++)
      for (int col = 30; col < LEVEL_COLS; col++)
        if (level->tiles[row][col] == TILE_QBLOCK) {
          item_kind_t what = (item_kind_t)level->content[row][col];
          found[0] += what == ITEM_SUSHI;
          found[1] += what == ITEM_DRAGON;
          found[2] += what == ITEM_GLOVES;
        }
    free(level);
  }
  CHECK(found[0] > 3 && found[1] > 3 && found[2] > 3);
}

void run_ally_tests(void) {
  RUN(the_opening_row_holds_the_three_pickups_and_each_brings_its_friend);
  RUN(each_friend_stands_a_few_steps_ahead_of_fabio_at_his_own_distance);
  RUN(a_friend_stays_thirty_seconds_and_then_goes_in_a_poof);
  RUN(calling_a_friend_who_is_here_gives_him_a_fresh_thirty_seconds_not_a_twin);
  RUN(andrea_strokes_his_belly_and_everything_round_him_goes);
  RUN(alessandro_runs_at_an_enemy_and_strikes_it_with_kung_fu);
  RUN(giulia_punches_what_comes_near_her);
  RUN(nothing_that_touches_a_friend_hurts_him_and_the_enemies_still_come_for_fabio);
  RUN(a_friend_left_far_behind_or_fallen_in_a_pit_turns_up_at_fabios_side);
  RUN(the_friends_jump_the_holes_in_the_ground);
  RUN(the_friends_go_down_a_pipe_with_fabio);
  RUN(the_friends_have_their_poses_and_their_sounds);
  RUN(levels_offer_the_three_pickups_now_and_then);
}
