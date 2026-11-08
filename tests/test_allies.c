#include <math.h>
#include <stdlib.h>

#include "bot.h"
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
  world->player.x = 1500.0f; /* room to walk back the other way */
  world->camera_x = world->player.x - 400.0f;
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
  /* Fabio turns round and walks the other way: they go round to the other side of him. */
  buttons_t left = BTN_LEFT;
  step(world, &left, 2.5f);
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
  ally_t g = {.kind = ALLY_GIULIA, .facing = 1, .grounded = true, .left = 5.0f, .age = 1.0f, .platform = -1};
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

static void a_water_polo_ball_brings_giacomo(void) {
  world_t* world = world_new(5, recording_events());
  bool in_opening_row = false;
  for (int row = 0; row < LEVEL_ROWS; row++)
    for (int col = 0; col < 30; col++)
      in_opening_row = in_opening_row || (level_tile(&world->level, col, row) == TILE_QBLOCK && world->level.content[row][col] == ITEM_POLO_BALL);
  CHECK(in_opening_row);
  world->spawning = false;
  world_spawn_pickup(world, ITEM_POLO_BALL, world->player.x, world->player.y - 10.0f);
  step(world, NULL, 0.5f);
  CHECK(allies(world) == 1 && ally_of(world, ALLY_GIACOMO));
  CHECK(world_has_event(world, EVENT_ALLY_ARRIVES));
  world_free(world);
}

static void giacomo_throws_the_ball_from_afar_and_it_kills_the_enemy_it_hits(void) {
  world_t* world = fresh();
  world_summon_ally(world, ALLY_GIACOMO);
  step(world, NULL, 1.0f);
  ally_t* g = ally_of(world, ALLY_GIACOMO);
  float start = g->x;
  still_zombie(world, start + 380.0f);
  step(world, NULL, 0.3f);
  CHECK(world->zombies_killed == 0); /* the ball is still on its way */
  step(world, NULL, 2.0f);
  CHECK(world->zombies_killed == 1);
  CHECK(world_has_event(world, EVENT_ALLY_THROW));
  CHECK(fabsf(g->x - start) < 40.0f && !g->sitting); /* he never ran at it */
  CHECK(pool_get_active_count(&world->polo_balls) == 0); /* the ball went with the zombie */
  /* One behind him gets the ball too, the other way. */
  world_clear_events(world);
  still_zombie(world, g->x - 300.0f);
  step(world, NULL, 2.5f);
  CHECK(world->zombies_killed == 2 && world_has_event(world, EVENT_ALLY_THROW));
  world_free(world);
}

static void giulia_punches_giacomo_and_he_sits_five_seconds_and_she_punches_him_again_every_time_she_reaches_him(void) {
  for (int giacomo_first = 0; giacomo_first < 2; giacomo_first++) {
    world_t* world = fresh();
    world_summon_ally(world, giacomo_first ? ALLY_GIACOMO : ALLY_GIULIA);
    step(world, NULL, 1.0f);
    world_summon_ally(world, giacomo_first ? ALLY_GIULIA : ALLY_GIACOMO);
    ally_t* g = ally_of(world, ALLY_GIACOMO);
    for (int i = 0; i < 60 && !g->sitting; i++) step(world, NULL, 0.1f);
    CHECK(g->sitting);
    step(world, NULL, 0.5f); /* thrown back, and down */
    CHECK(g->grounded);
    CHECK(world_has_event(world, EVENT_ALLY_PUNCH) && world_has_event(world, EVENT_ALLY_OUCH));
    CHECK(pose_ally(g).strip == STRIP_ALLY_GIACOMO_SIT);
    /* For the five seconds an enemy ahead of him is left alone and he does not move. */
    float where = g->x;
    still_zombie(world, g->x + 300.0f + 150.0f);
    step(world, NULL, GIACOMO_SIT_SECONDS - 1.0f);
    CHECK(world->zombies_killed == 0 && fabsf(g->x - where) < 2.0f && g->sitting);
    CHECK(allies(world) == 2);
    /* He gets up after the five seconds; she is still on him, so down he goes again,
     * and so on every time. Count the times he is knocked down. */
    int punches = 1;
    bool was_sitting = true;
    float sat_for = GIACOMO_SIT_SECONDS - 1.0f + 0.5f, longest = 0.0f;
    float up_for = 0.0f, shortest_up = 99.0f;
    for (int i = 0; i < 300; i++) { /* 30 seconds */
      step(world, NULL, 0.1f);
      if (g->sitting) {
        if (!was_sitting) { /* knocked down again: how long was he up? */
          shortest_up = fminf(shortest_up, up_for);
          punches++;
        }
        sat_for += 0.1f;
        up_for = 0.0f;
      } else {
        if (was_sitting) longest = fmaxf(longest, sat_for);
        sat_for = 0.0f;
        up_for += 0.1f;
      }
      was_sitting = g->sitting;
    }
    CHECK(punches >= 3);
    CHECK(longest > GIACOMO_SIT_SECONDS - 0.3f && longest < GIACOMO_SIT_SECONDS + 0.5f);
    /* Up on his feet, he has at least his moment of grace before she hits him again. */
    CHECK(shortest_up >= GIACOMO_GRACE_SECONDS - 0.15f);
    world_free(world);
  }
}

static void a_sitting_friend_who_falls_in_a_pit_is_set_down_on_ground_and_still_gets_up(void) {
  world_t* world = fresh();
  world->player.x = 10 * TILE;
  world_summon_ally(world, ALLY_GIACOMO);
  step(world, NULL, 1.0f);
  /* A hole exactly where his place beside Fabio is, and he is down in it, sitting. */
  float offset = ally_offset(ALLY_GIACOMO);
  int col = (int)((world->player.x + offset) / TILE);
  for (int c = col - 2; c <= col + 2; c++)
    for (int row = GROUND_ROW; row < LEVEL_ROWS; row++) world->level.tiles[row][c] = TILE_EMPTY;
  ally_t* g = ally_of(world, ALLY_GIACOMO);
  g->sitting = true;
  g->sit_left = GIACOMO_SIT_SECONDS;
  g->left = 100.0f;
  g->y = level_height() + 100.0f;
  /* Without the fix he is fetched back to the same hole again and again and never stands. */
  float stood_at = -1.0f;
  for (int i = 0; i < 150 && stood_at < 0.0f; i++) {
    step(world, NULL, 0.1f);
    g = ally_of(world, ALLY_GIACOMO);
    if (g && !g->sitting) stood_at = (float)(i + 1) * 0.1f;
  }
  CHECK(stood_at > GIACOMO_SIT_SECONDS - 0.2f && stood_at < GIACOMO_SIT_SECONDS + 3.0f);
  world_free(world);
}

static void giacomo_gets_up_and_carries_on_when_giulia_is_gone(void) {
  world_t* world = fresh();
  world_summon_ally(world, ALLY_GIACOMO);
  world_summon_ally(world, ALLY_GIULIA);
  ally_t* g = ally_of(world, ALLY_GIACOMO);
  for (int i = 0; i < 60 && !g->sitting; i++) step(world, NULL, 0.1f);
  CHECK(g->sitting);
  ally_t* giulia = ally_of(world, ALLY_GIULIA);
  giulia->left = 0.01f; /* she leaves */
  step(world, NULL, GIACOMO_SIT_SECONDS + 0.5f);
  CHECK(ally_of(world, ALLY_GIULIA) == NULL && !g->sitting);
  world_clear_events(world);
  still_zombie(world, g->x + 380.0f);
  step(world, NULL, 3.0f);
  CHECK(world->zombies_killed == 1 && world_has_event(world, EVENT_ALLY_THROW));
  world_free(world);
}

static void giacomo_alone_is_never_punched_and_all_four_friends_fit(void) {
  world_t* world = fresh();
  world_summon_ally(world, ALLY_GIACOMO);
  step(world, NULL, 3.0f);
  CHECK(!ally_of(world, ALLY_GIACOMO)->sitting);
  world_summon_ally(world, ALLY_ANDREA);
  world_summon_ally(world, ALLY_ALESSANDRO);
  world_summon_ally(world, ALLY_GIULIA);
  CHECK(allies(world) == 4);
  step(world, NULL, 4.0f);
  CHECK(ally_of(world, ALLY_GIACOMO)->sitting);
  float volume;
  CHECK(sfx_for_event(EVENT_ALLY_THROW, &volume) == SFX_ALLY_THROW && sfx_for_event(EVENT_ALLY_OUCH, &volume) == SFX_ALLY_OUCH);
  CHECK(pose_ally_item(ALLY_GIACOMO, 0.0f).strip == STRIP_ITEM_POLO_BALL);
  CHECK(pose_polo_ball(0.0f).strip == STRIP_ALLY_GIACOMO_FX);
  world_free(world);
}

static void giulia_catches_giacomo_even_while_fabio_runs(void) {
  world_t* world = fresh();
  world_summon_ally(world, ALLY_GIACOMO);
  world_summon_ally(world, ALLY_GIULIA);
  buttons_t right = BTN_RIGHT;
  bool was_punched = false;
  for (int i = 0; i < 80; i++) {
    step(world, &right, 0.1f);
    was_punched = was_punched || ally_of(world, ALLY_GIACOMO)->sitting;
  }
  CHECK(was_punched);
  world_free(world);
}

static void giacomo_crouches_to_roll_the_ball_at_a_beetle_on_the_floor(void) {
  world_t* world = fresh();
  world_summon_ally(world, ALLY_GIACOMO);
  step(world, NULL, 1.0f);
  ally_t* g = ally_of(world, ALLY_GIACOMO);
  spawn_bug(world, BUG_SMALL, g->x + 380.0f, GROUND);
  enemy_t* bug = enemy_at(world, (size_t)(world_count_enemies(world, ENEMY_BUG) - 1));
  bug->speed = 0.0f;
  bool crouched = false;
  for (int i = 0; i < 25 && world->zombies_killed + world->bugs_killed == 0; i++) {
    step(world, NULL, 0.1f);
    crouched = crouched || (g->attack > 0.0f && g->crouching && pose_ally(g).strip == STRIP_ALLY_GIACOMO_LOW);
  }
  CHECK(crouched);
  step(world, NULL, 1.0f);
  CHECK(world_count_enemies(world, ENEMY_BUG) == 0);
  /* A zombie he faces standing up. */
  still_zombie(world, g->x + 380.0f);
  bool stood = false;
  for (int i = 0; i < 25 && world->zombies_killed == 0; i++) {
    step(world, NULL, 0.1f);
    stood = stood || (g->attack > 0.0f && !g->crouching);
  }
  CHECK(stood && world->zombies_killed == 1);
  world_free(world);
}

/* ---- Finding the way up: friends follow Fabio to other floors by walking and jumping. ---- */

static int add_plank(world_t* world, float x, float top, float width) {
  int index = world->level.platform_count++;
  world->level.platforms[index] = (platform_def_t){PLATFORM_STATIC, x, top, width, 0.0f, 1.0f, 0.0f};
  platforms_reset(world);
  return index;
}

/* Fabio stands on a plank (or the ground when `plank` is -1) at x, as if he had jumped up there. */
static void fabio_stands(world_t* world, float x, float feet, int plank) {
  player_t* p = &world->player;
  p->x = x;
  p->y = feet;
  p->vx = p->vy = 0.0f;
  p->grounded = true;
  p->platform = plank;
}

static bool in_sight(const world_t* world, float x) {
  float view = world->view_w > 0.0f ? world->view_w : 1280.0f;
  return x > world->camera_x - 40.0f && x < world->camera_x + view + 40.0f;
}

/* Whether any of him is on the screen: its width, and its height (the level is one screen tall). */
static bool seen_at(const world_t* world, float x, float feet) {
  return in_sight(world, x) && feet - ALLY_H < level_height();
}

/* Runs for `seconds` and returns the biggest distance any friend moved in a 0.05 s step while anyone
 * could see it (the start of the first step included): a jump or a fall is at most about 45 pixels, a
 * sprint 35, a pop from one place to another far more. Friends are told apart by kind. */
static float watch_friends_move(world_t* world, float seconds) {
  float biggest = 0.0f;
  float last_x[ALLY_KIND_COUNT], last_y[ALLY_KIND_COUNT];
  bool known[ALLY_KIND_COUNT] = {false};
  size_t cursor = 0;
  for (ally_t* a; (a = pool_next_active(&world->allies, &cursor));) {
    last_x[a->kind] = a->x;
    last_y[a->kind] = a->y;
    known[a->kind] = true;
  }
  for (float t = 0.0f; t < seconds; t += 0.05f) {
    step(world, NULL, 0.05f);
    cursor = 0;
    for (ally_t* a; (a = pool_next_active(&world->allies, &cursor));) {
      if (known[a->kind] && (seen_at(world, a->x, a->y) || seen_at(world, last_x[a->kind], last_y[a->kind])))
        biggest = fmaxf(biggest, fmaxf(fabsf(a->x - last_x[a->kind]), fabsf(a->y - last_y[a->kind])));
      last_x[a->kind] = a->x;
      last_y[a->kind] = a->y;
      known[a->kind] = true;
    }
  }
  return biggest;
}

static void a_friend_on_the_street_jumps_up_to_a_plank_fabio_has_climbed_onto(void) {
  for (int kind = 0; kind < ALLY_KIND_COUNT; kind++) {
    world_t* world = fresh();
    world_summon_ally(world, (ally_kind_t)kind);
    step(world, NULL, 1.0f);
    int plank = add_plank(world, 20.0f * TILE, GROUND - 100.0f, 160.0f);
    fabio_stands(world, 20.0f * TILE, GROUND - 100.0f, plank);
    float biggest = watch_friends_move(world, 6.0f);
    ally_t* a = ally_of(world, (ally_kind_t)kind);
    CHECK(a && a->platform == plank && fabsf(a->y - (GROUND - 100.0f)) < 1.5f);
    CHECK(biggest < 60.0f); /* he walked and jumped there: no leap from the street to the plank */
    world_free(world);
  }
}

static void a_friend_climbs_a_staircase_of_planks_one_jump_at_a_time(void) {
  world_t* world = fresh();
  world_summon_ally(world, ALLY_ALESSANDRO);
  step(world, NULL, 1.0f);
  add_plank(world, 18.0f * TILE, GROUND - 110.0f, 120.0f);
  int top = add_plank(world, 24.0f * TILE, GROUND - 220.0f, 140.0f); /* too high to reach from the street */
  fabio_stands(world, 24.0f * TILE, GROUND - 220.0f, top);
  float biggest = watch_friends_move(world, 9.0f);
  ally_t* a = ally_of(world, ALLY_ALESSANDRO);
  CHECK(a && a->platform == top && fabsf(a->y - (GROUND - 220.0f)) < 1.5f);
  CHECK(biggest < 60.0f);
  CHECK(!effect_live(world, FX_ALLY_POOF)); /* nobody was brought over in a puff */
  world_free(world);
}

static void a_friend_gets_up_onto_a_block_that_has_sides_by_running_at_it(void) {
  world_t* world = fresh();
  world_summon_ally(world, ALLY_GIULIA);
  step(world, NULL, 1.0f);
  for (int col = 18; col <= 23; col++) world->level.tiles[GROUND_ROW - 4][col] = TILE_BRICK; /* a row of blocks, 128 up */
  float top = (float)((GROUND_ROW - 4) * TILE);
  fabio_stands(world, 20.5f * TILE, top, -1);
  float biggest = watch_friends_move(world, 9.0f);
  ally_t* a = ally_of(world, ALLY_GIULIA);
  CHECK(a && fabsf(a->y - top) < 1.5f && a->x > 18.0f * TILE && a->x < 24.0f * TILE);
  CHECK(biggest < 60.0f);
  world_free(world);
}

static void a_friend_walks_off_the_edge_to_come_down_after_fabio(void) {
  world_t* world = fresh();
  int plank = add_plank(world, 20.0f * TILE, GROUND - 100.0f, 160.0f);
  fabio_stands(world, 20.0f * TILE, GROUND - 100.0f, plank);
  world_summon_ally(world, ALLY_ANDREA);
  step(world, NULL, 4.0f);
  ally_t* a = ally_of(world, ALLY_ANDREA);
  CHECK(a && a->platform == plank); /* up on the plank with him */
  fabio_stands(world, 25.0f * TILE, GROUND, -1); /* Fabio drops to the street */
  float biggest = watch_friends_move(world, 6.0f);
  a = ally_of(world, ALLY_ANDREA);
  CHECK(a && a->platform == -1 && fabsf(a->y - GROUND) < 1.5f);
  CHECK(biggest < 60.0f);
  world_free(world);
}

static void a_friend_with_no_way_up_waits_below_and_keeps_pace_never_popping_up_to_fabio(void) {
  world_t* world = fresh();
  world_summon_ally(world, ALLY_ANDREA);
  step(world, NULL, 1.0f);
  int perch = add_plank(world, 20.0f * TILE, GROUND - 400.0f, 120.0f); /* out of anybody's reach */
  fabio_stands(world, 20.0f * TILE, GROUND - 400.0f, perch);
  float biggest = watch_friends_move(world, 16.0f);
  ally_t* a = ally_of(world, ALLY_ANDREA);
  CHECK(a && a->platform != perch && a->y > GROUND - 5.0f); /* still below, as long as he stays up there */
  CHECK(fabsf(a->x - 20.0f * TILE) < 300.0f); /* but keeping pace with Fabio's x */
  CHECK(biggest < 60.0f && !effect_live(world, FX_ALLY_POOF));
  world_free(world);
}

static void a_friend_left_far_behind_sprints_to_catch_up_and_never_pops_up_in_front(void) {
  world_t* world = fresh();
  world_summon_ally(world, ALLY_ANDREA);
  step(world, NULL, 1.0f);
  world->player.x += 900.0f; /* Fabio is suddenly far ahead (in the game he would have run there) */
  float biggest = watch_friends_move(world, 4.0f);
  ally_t* a = ally_of(world, ALLY_ANDREA);
  CHECK(a && fabsf(a->x - world->player.x - ally_offset(ALLY_ANDREA)) < 40.0f);
  CHECK(biggest < 60.0f);
  world_free(world);
}

static void a_friend_who_fell_in_a_pit_runs_back_in_from_the_edge_of_the_screen(void) {
  world_t* world = fresh();
  world->player.x = 40.0f * TILE; /* away from the start, where there is room behind the screen */
  world->camera_x = world->player.x - 400.0f;
  world_summon_ally(world, ALLY_GIULIA);
  step(world, NULL, 1.0f);
  ally_t* a = ally_of(world, ALLY_GIULIA);
  a->y = level_height() + 100.0f; /* down the hole */
  step(world, NULL, 0.1f);
  a = ally_of(world, ALLY_GIULIA);
  CHECK(a->x < world->camera_x); /* back in, out of sight behind Fabio... */
  CHECK(a->y <= GROUND + 1.0f && a->vx > 100.0f); /* ...on the ground and running towards him */
  float biggest = watch_friends_move(world, 5.0f);
  a = ally_of(world, ALLY_GIULIA);
  CHECK(fabsf(a->x - world->player.x - ally_offset(ALLY_GIULIA)) < 40.0f);
  CHECK(biggest < 60.0f);
  world_free(world);
}

static void a_friend_arrives_at_fabio_and_runs_on_ahead_to_his_place(void) {
  for (int kind = 0; kind < ALLY_KIND_COUNT; kind++) {
    world_t* world = fresh();
    world_summon_ally(world, (ally_kind_t)kind);
    ally_t* a = ally_of(world, (ally_kind_t)kind);
    CHECK(fabsf(a->x - world->player.x) < 20.0f); /* he is there with Fabio, not already out in front */
    float biggest = watch_friends_move(world, 2.0f);
    a = ally_of(world, (ally_kind_t)kind);
    CHECK(fabsf(a->x - world->player.x - ally_offset((ally_kind_t)kind)) < 12.0f);
    CHECK(biggest < 60.0f);
    world_free(world);
  }
}

static void in_real_levels_with_enemies_the_friends_move_smoothly_all_the_way(void) {
  for (uint32_t seed = 3; seed <= 6; seed++) {
    world_t* world = world_new(seed, recording_events());
    bot_t bot;
    memset(&bot, 0, sizeof(bot));
    controls_t controls;
    memset(&controls, 0, sizeof(controls));
    world->view_w = 960.0f;
    for (int k = 0; k < ALLY_KIND_COUNT; k++) world_summon_ally(world, (ally_kind_t)k);
    float last_x[ALLY_KIND_COUNT], last_y[ALLY_KIND_COUNT];
    bool known[ALLY_KIND_COUNT] = {false};
    size_t cursor = 0;
    for (ally_t* a; (a = pool_next_active(&world->allies, &cursor));) { /* where they stand before the first frame */
      last_x[a->kind] = a->x;
      last_y[a->kind] = a->y;
      known[a->kind] = true;
    }
    float biggest = 0.0f, calm_after = 0.0f;
    for (float t = 0.0f; t < 60.0f && world->state == STATE_PLAYING; t += 1.0f / 60.0f) {
      /* a pipe to a cave is a change of scene: the friends arrive with him */
      if (world->warp.phase != WARP_NONE) calm_after = t + 0.2f;
      for (ally_kind_t kind = 0; kind < ALLY_KIND_COUNT; kind++) { /* keep them alive for the whole run */
        ally_t* a = ally_of(world, kind);
        if (a) a->left = ALLY_SECONDS;
      }
      buttons_t now = bot_decide(world, &bot, 1.0f / 60.0f);
      controls_update(&controls, now);
      world_update(world, &controls, 1.0f / 60.0f);
      cursor = 0;
      for (ally_t* a; (a = pool_next_active(&world->allies, &cursor));) {
        /* frame to frame: a jump is at most 14 pixels, a sprint 11, a pop from one place to another far more */
        if (known[a->kind] && t > calm_after && (seen_at(world, a->x, a->y) || seen_at(world, last_x[a->kind], last_y[a->kind])))
          biggest = fmaxf(biggest, fmaxf(fabsf(a->x - last_x[a->kind]), fabsf(a->y - last_y[a->kind])));
        last_x[a->kind] = a->x;
        last_y[a->kind] = a->y;
        known[a->kind] = true;
      }
    }
    CHECK(biggest < 25.0f);
    world_free(world);
  }
}

static void giacomo_left_behind_while_he_sits_catches_up_on_his_own_feet(void) {
  world_t* world = fresh();
  world_summon_ally(world, ALLY_GIACOMO);
  world_summon_ally(world, ALLY_GIULIA);
  buttons_t run_right = BTN_RIGHT | BTN_RUN;
  float biggest = 0.0f, last_x[ALLY_KIND_COUNT], last_y[ALLY_KIND_COUNT];
  bool known[ALLY_KIND_COUNT] = {false};
  size_t cursor = 0;
  for (ally_t* a; (a = pool_next_active(&world->allies, &cursor));) {
    last_x[a->kind] = a->x;
    last_y[a->kind] = a->y;
    known[a->kind] = true;
  }
  bool sat = false;
  for (int i = 0; i < 400; i++) { /* 20 seconds with Fabio running on, and Giulia knocking Giacomo down */
    step(world, &run_right, 0.05f);
    cursor = 0;
    for (ally_t* a; (a = pool_next_active(&world->allies, &cursor));) {
      if (a->kind == ALLY_GIACOMO && a->sitting) sat = true;
      if (known[a->kind] && (seen_at(world, a->x, a->y) || seen_at(world, last_x[a->kind], last_y[a->kind])))
        biggest = fmaxf(biggest, fmaxf(fabsf(a->x - last_x[a->kind]), fabsf(a->y - last_y[a->kind])));
      last_x[a->kind] = a->x;
      last_y[a->kind] = a->y;
    }
  }
  CHECK(sat);
  CHECK(biggest < 60.0f); /* nobody was ever picked up and put down somewhere else */
  world_free(world);
}

static int facing_flips(world_t* world, ally_kind_t kind, float seconds) {
  int flips = 0;
  ally_t* a = ally_of(world, kind);
  int last = a->facing;
  for (float t = 0.0f; t < seconds; t += 1.0f / 120.0f) {
    step(world, NULL, 1.0f / 120.0f);
    a = ally_of(world, kind);
    if (a->facing != last) flips++;
    last = a->facing;
  }
  return flips;
}

static void a_friend_keeps_his_stance_while_fabio_stands_still_and_does_not_jitter(void) {
  world_t* world = fresh();
  world_summon_ally(world, ALLY_ANDREA);
  step(world, NULL, 1.0f);
  ally_t* a = ally_of(world, ALLY_ANDREA);
  a->x = world->player.x + ally_offset(ALLY_ANDREA) + 7.0f; /* a little off his place, so he has to nudge back */
  CHECK(facing_flips(world, ALLY_ANDREA, 3.0f) == 0);
  CHECK(ally_of(world, ALLY_ANDREA)->facing == 1);
  world_free(world);
}

static void the_formation_follows_the_way_fabio_travels_not_the_way_he_faces(void) {
  world_t* world = fresh();
  world->player.x = 1500.0f; /* room to walk back */
  world->camera_x = world->player.x - 400.0f;
  world_summon_ally(world, ALLY_ANDREA);
  world_summon_ally(world, ALLY_GIULIA);
  step(world, NULL, 1.5f);
  /* He walks left while striking back to the right: facing says right, travel says left. */
  buttons_t left = BTN_LEFT;
  for (int i = 0; i < 40; i++) {
    world->player.facing = 1;
    step(world, &left, 0.05f);
  }
  CHECK(world->player.vx < -40.0f);
  CHECK(ally_of(world, ALLY_ANDREA)->x < world->player.x - 40.0f);
  CHECK(ally_of(world, ALLY_GIULIA)->x < world->player.x - 100.0f);
  /* He stops, and turns to face the other way without moving: they stay where they are ahead of him. */
  step(world, NULL, 1.0f);
  world->player.facing = 1;
  step(world, NULL, 1.0f);
  CHECK(ally_of(world, ALLY_ANDREA)->x < world->player.x - 40.0f);
  world_free(world);
}

static void recovery_never_shows_a_friend_moving_from_one_place_to_another(void) {
  /* At the start of the level there is no ground behind the screen: he comes in from the other side. */
  world_t* world = fresh();
  world_summon_ally(world, ALLY_GIULIA);
  step(world, NULL, 1.0f);
  ally_t* a = ally_of(world, ALLY_GIULIA);
  float view = world->view_w > 0.0f ? world->view_w : 1280.0f;
  a->y = level_height() + 100.0f;
  step(world, NULL, 0.05f);
  a = ally_of(world, ALLY_GIULIA);
  CHECK(!in_sight(world, a->x) && a->x > world->camera_x + view && a->y <= GROUND + 1.0f);
  world_free(world);

  /* The same at the far end of the level, facing left. */
  world = fresh();
  world->player.x = level_width() - 200.0f;
  world->player.facing = -1;
  world->camera_x = world->player.x - 700.0f;
  world_summon_ally(world, ALLY_GIULIA);
  buttons_t left = BTN_LEFT;
  step(world, &left, 1.0f);
  a = ally_of(world, ALLY_GIULIA);
  a->y = level_height() + 100.0f;
  step(world, &left, 0.05f);
  a = ally_of(world, ALLY_GIULIA);
  CHECK(!in_sight(world, a->x) && a->y <= GROUND + 1.0f);
  world_free(world);

  /* A friend who is far away but still on the screen is not moved at all: he sprints. */
  world = fresh();
  world->view_w = 2400.0f;
  world_summon_ally(world, ALLY_ANDREA);
  step(world, NULL, 1.0f);
  a = ally_of(world, ALLY_ANDREA);
  a->x = world->player.x + 1600.0f;
  float before = a->x;
  step(world, NULL, 0.05f);
  a = ally_of(world, ALLY_ANDREA);
  CHECK(fabsf(a->x - before) < 60.0f);
  world_free(world);
}

static void a_friend_fallen_in_a_cave_comes_back_in_the_cave_not_on_its_roof(void) {
  world_t* world = fresh();
  level_generate_cave(&world->level, 7, 1);
  platforms_reset(world);
  world->player.x = 1100.0f;
  world->player.y = GROUND;
  world->player.facing = -1;
  world->view_w = 960.0f;
  world->camera_x = world->player.x - 400.0f;
  world_summon_ally(world, ALLY_GIULIA);
  step(world, NULL, 0.5f);
  ally_t* a = ally_of(world, ALLY_GIULIA);
  a->y = level_height() + 100.0f;
  step(world, NULL, 0.05f);
  a = ally_of(world, ALLY_GIULIA);
  CHECK(a->x > 0.0f && a->x < (float)(world->level.cols * TILE) && a->y > (float)(2 * TILE)); /* inside, on the cave floor */
  world_free(world);
}

static void a_friend_comes_down_from_a_plank_that_the_plank_below_sits_inside(void) {
  world_t* world = fresh();
  world_summon_ally(world, ALLY_ANDREA);
  step(world, NULL, 1.0f);
  int upper = add_plank(world, 640.0f, GROUND - 200.0f, 200.0f);
  int lower = add_plank(world, 640.0f, GROUND - 100.0f, 100.0f); /* narrower, right under it */
  ally_t* a = ally_of(world, ALLY_ANDREA);
  a->x = 640.0f;
  a->y = GROUND - 200.0f;
  a->vy = 0.0f;
  a->platform = upper;
  a->grounded = true;
  fabio_stands(world, 640.0f, GROUND - 100.0f, lower);
  float biggest = watch_friends_move(world, 14.0f);
  a = ally_of(world, ALLY_ANDREA);
  CHECK(a && a->platform == lower && fabsf(a->y - (GROUND - 100.0f)) < 1.5f);
  CHECK(biggest < 60.0f);
  world_free(world);
}

static void a_jump_that_would_hit_a_ceiling_is_made_from_somewhere_it_clears(void) {
  world_t* world = fresh();
  world_summon_ally(world, ALLY_ANDREA);
  step(world, NULL, 1.0f);
  for (int col = 19; col <= 21; col++) world->level.tiles[GROUND_ROW - 3][col] = TILE_BRICK; /* a roof under the plank */
  int plank = add_plank(world, 20.5f * TILE, GROUND - 100.0f, 160.0f);
  fabio_stands(world, 20.5f * TILE, GROUND - 100.0f, plank);
  ally_t* under = ally_of(world, ALLY_ANDREA);
  under->x = 20.0f * TILE; /* right under the roof, where a straight jump would hit it */
  under->vx = 0.0f;
  float biggest = watch_friends_move(world, 12.0f);
  ally_t* a = ally_of(world, ALLY_ANDREA);
  CHECK(a && a->platform == plank);
  CHECK(biggest < 60.0f);
  world_free(world);
}

static void a_friend_off_the_screen_with_nowhere_to_come_in_from_waits_unseen_then_runs_in(void) {
  world_t* world = fresh();
  level_generate_cave(&world->level, 7, 1); /* 1536 pixels wide */
  platforms_reset(world);
  world->player.x = 320.0f;
  world->player.y = GROUND;
  world->view_w = 1472.0f; /* the screen shows nearly all of it: no ground beyond either edge */
  world->camera_x = 0.0f;
  world_summon_ally(world, ALLY_GIULIA);
  step(world, NULL, 0.5f);
  ally_t* a = ally_of(world, ALLY_GIULIA);
  a->y = level_height() + 100.0f;
  float biggest = watch_friends_move(world, 1.0f);
  a = ally_of(world, ALLY_GIULIA);
  CHECK(!seen_at(world, a->x, a->y) && biggest < 60.0f); /* out of sight, and not moved where it can be seen */
  world->view_w = 800.0f; /* now there is ground beyond the edge of what is shown */
  float after = watch_friends_move(world, 4.0f);
  a = ally_of(world, ALLY_GIULIA);
  CHECK(a->y <= GROUND + 1.0f && fabsf(a->x - world->player.x - ally_offset(ALLY_GIULIA)) < 40.0f);
  CHECK(after < 60.0f);
  world_free(world);
}

static void a_friend_held_by_the_edge_of_his_plank_does_not_run_on_past_it_with_fabio(void) {
  world_t* world = fresh();
  int plank = add_plank(world, 768.0f, GROUND - 128.0f, 384.0f); /* 576 to 960 */
  fabio_stands(world, 700.0f, GROUND - 128.0f, plank);
  world_summon_ally(world, ALLY_GIACOMO);
  ally_t* g = ally_of(world, ALLY_GIACOMO);
  g->x = 940.0f;
  g->y = GROUND - 128.0f;
  g->platform = plank;
  g->grounded = true;
  buttons_t run_right = BTN_RIGHT | BTN_RUN;
  for (int i = 0; i < 60; i++) { /* a second of Fabio running along the plank, towards the end of it */
    step(world, &run_right, 1.0f / 60.0f);
    g = ally_of(world, ALLY_GIACOMO);
    CHECK(g && g->platform == plank && fabsf(g->y - (GROUND - 128.0f)) < 1.5f);
  }
  world_free(world);
}

static void a_plank_only_a_step_above_the_street_still_has_to_be_climbed(void) {
  world_t* world = fresh();
  world_summon_ally(world, ALLY_ANDREA);
  step(world, NULL, 1.0f);
  int plank = add_plank(world, 640.0f, GROUND - 20.0f, 200.0f); /* within 24 px of the street, but not the street */
  fabio_stands(world, 640.0f, GROUND - 20.0f, plank);
  float biggest = watch_friends_move(world, 10.0f);
  ally_t* a = ally_of(world, ALLY_ANDREA);
  CHECK(a && a->platform == plank && fabsf(a->y - (GROUND - 20.0f)) < 1.5f);
  CHECK(biggest < 60.0f);
  world_free(world);
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
  RUN(the_friends_jump_the_holes_in_the_ground);
  RUN(the_friends_go_down_a_pipe_with_fabio);
  RUN(the_friends_have_their_poses_and_their_sounds);
  RUN(levels_offer_the_three_pickups_now_and_then);
  RUN(a_water_polo_ball_brings_giacomo);
  RUN(giacomo_throws_the_ball_from_afar_and_it_kills_the_enemy_it_hits);
  RUN(giulia_punches_giacomo_and_he_sits_five_seconds_and_she_punches_him_again_every_time_she_reaches_him);
  RUN(a_sitting_friend_who_falls_in_a_pit_is_set_down_on_ground_and_still_gets_up);
  RUN(giacomo_gets_up_and_carries_on_when_giulia_is_gone);
  RUN(giacomo_alone_is_never_punched_and_all_four_friends_fit);
  RUN(giulia_catches_giacomo_even_while_fabio_runs);
  RUN(giacomo_crouches_to_roll_the_ball_at_a_beetle_on_the_floor);
  RUN(a_friend_on_the_street_jumps_up_to_a_plank_fabio_has_climbed_onto);
  RUN(a_friend_climbs_a_staircase_of_planks_one_jump_at_a_time);
  RUN(a_friend_gets_up_onto_a_block_that_has_sides_by_running_at_it);
  RUN(a_friend_walks_off_the_edge_to_come_down_after_fabio);
  RUN(a_friend_with_no_way_up_waits_below_and_keeps_pace_never_popping_up_to_fabio);
  RUN(a_friend_left_far_behind_sprints_to_catch_up_and_never_pops_up_in_front);
  RUN(a_friend_who_fell_in_a_pit_runs_back_in_from_the_edge_of_the_screen);
  RUN(a_friend_arrives_at_fabio_and_runs_on_ahead_to_his_place);
  RUN(giacomo_left_behind_while_he_sits_catches_up_on_his_own_feet);
  RUN(in_real_levels_with_enemies_the_friends_move_smoothly_all_the_way);
  RUN(a_friend_keeps_his_stance_while_fabio_stands_still_and_does_not_jitter);
  RUN(the_formation_follows_the_way_fabio_travels_not_the_way_he_faces);
  RUN(recovery_never_shows_a_friend_moving_from_one_place_to_another);
  RUN(a_friend_fallen_in_a_cave_comes_back_in_the_cave_not_on_its_roof);
  RUN(a_friend_comes_down_from_a_plank_that_the_plank_below_sits_inside);
  RUN(a_jump_that_would_hit_a_ceiling_is_made_from_somewhere_it_clears);
  RUN(a_friend_off_the_screen_with_nowhere_to_come_in_from_waits_unseen_then_runs_in);
  RUN(a_friend_held_by_the_edge_of_his_plank_does_not_run_on_past_it_with_fabio);
  RUN(a_plank_only_a_step_above_the_street_still_has_to_be_climbed);
}
