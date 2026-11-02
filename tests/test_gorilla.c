#include <math.h>
#include <string.h>

#include "pose.h"
#include "test_groups.h"
#include "test_support.h"
#include "test_world.h"

#define RELEASE_AT (GORILLA_CYCLE - GORILLA_THROW_SECONDS + GORILLA_RELEASE)

static world_t* fresh(void) {
  world_t* world = new_empty_world();
  settle(world);
  return world;
}

static int bananas(const world_t* world) {
  return (int)pool_get_active_count(&world->bananas);
}

static banana_t* first_banana(world_t* world) {
  size_t cursor = 0;
  return pool_next_active(&world->bananas, &cursor);
}

static void the_gorilla_stands_where_he_appeared(void) {
  world_t* world = fresh();
  world->player.x = 5 * TILE;
  float x = 40 * TILE;
  spawn_gorilla(world, x, world->player.y - 100.0f);
  step(world, NULL, 3.0f);
  CHECK(enemy_at(world, 0)->x == x);
  CHECK(enemy_at(world, 0)->grounded);
  world_free(world);
}

static void he_turns_to_face_fabio_as_fabio_moves(void) {
  world_t* world = fresh();
  world->player.x = 5 * TILE;
  spawn_gorilla(world, 40 * TILE, world->player.y);
  step(world, NULL, 0.1f);
  CHECK(enemy_at(world, 0)->facing == -1);
  world->player.x = 60 * TILE; /* he has run past him */
  step(world, NULL, 0.1f);
  CHECK(enemy_at(world, 0)->facing == 1);
  world_free(world);
}

static void he_stands_roars_beats_his_chest_and_throws_in_turn(void) {
  enemy_t g;
  memset(&g, 0, sizeof(g));
  gorilla_act_t seen[4];
  float at[4] = {0.1f, GORILLA_STAND_SECONDS + 0.1f,
                 GORILLA_STAND_SECONDS + GORILLA_ROAR_SECONDS + 0.1f,
                 GORILLA_CYCLE - GORILLA_THROW_SECONDS + 0.1f};
  for (int i = 0; i < 4; i++) {
    g.anim_time = at[i];
    seen[i] = gorilla_act(&g, NULL);
  }
  CHECK(seen[0] == GORILLA_STANDING && seen[1] == GORILLA_ROARING);
  CHECK(seen[2] == GORILLA_BEATING && seen[3] == GORILLA_THROWING);
  g.anim_time = GORILLA_CYCLE + 0.1f; /* and round again */
  CHECK(gorilla_act(&g, NULL) == GORILLA_STANDING);
}

static void his_frames_follow_what_he_is_doing(void) {
  enemy_t g;
  memset(&g, 0, sizeof(g));
  g.anim_time = 0.5f;
  CHECK(pose_gorilla(&g).strip == STRIP_GORILLA_IDLE);
  /* The roar runs through its four frames once, in order. */
  int last = -1;
  for (float t = 0.0f; t < GORILLA_ROAR_SECONDS; t += 0.01f) {
    g.anim_time = GORILLA_STAND_SECONDS + t;
    pose_t pose = pose_gorilla(&g);
    CHECK(pose.strip == STRIP_GORILLA_ROAR);
    CHECK(pose.frame == last || pose.frame == last + 1);
    last = pose.frame;
  }
  CHECK(last == 3);
  /* The chest beat flicks between its two frames, again and again. */
  int swaps = 0, previous = -1;
  for (float t = 0.0f; t < GORILLA_BEAT_SECONDS; t += 0.01f) {
    g.anim_time = GORILLA_STAND_SECONDS + GORILLA_ROAR_SECONDS + t;
    pose_t pose = pose_gorilla(&g);
    CHECK(pose.strip == STRIP_GORILLA_BEAT);
    if (previous >= 0 && pose.frame != previous) swaps++;
    previous = pose.frame;
  }
  CHECK(swaps >= 5);
  /* The throw ends on its last frame, the arm out, just as the banana goes. */
  g.anim_time = RELEASE_AT - 0.01f;
  CHECK(pose_gorilla(&g).strip == STRIP_GORILLA_THROW && pose_gorilla(&g).frame == 2);
  g.anim_time = RELEASE_AT + 0.01f;
  CHECK(pose_gorilla(&g).frame == 3);
  g.anim_time = GORILLA_CYCLE - 0.01f;
  CHECK(pose_gorilla(&g).frame == 3);
}

static void he_throws_one_banana_per_round_at_fabio(void) {
  world_t* world = fresh();
  world->player.invulnerable = 1e9f;
  spawn_gorilla(world, world->player.x + 500.0f, world->player.y);
  step(world, NULL, RELEASE_AT - 0.05f);
  CHECK(bananas(world) == 0);
  step(world, NULL, 0.1f);
  CHECK(bananas(world) == 1);
  CHECK(world_has_event(world, EVENT_BANANA_THROWN));
  banana_t* b = first_banana(world);
  CHECK(b->vx < 0.0f); /* towards Fabio, on his left */
  CHECK(b->x < enemy_at(world, 0)->x);
  world_free(world);
}

static void no_banana_while_he_is_off_screen(void) {
  world_t* world = fresh();
  spawn_gorilla(world, world->camera_x + world->view_w + 100.0f, world->player.y);
  step(world, NULL, GORILLA_CYCLE * 2.0f);
  CHECK(bananas(world) == 0);
  CHECK(!world_has_event(world, EVENT_BANANA_THROWN));
  world_free(world);
}

static void he_throws_from_anywhere_on_screen(void) {
  world_t* world = fresh();
  world->player.invulnerable = 1e9f;
  world->view_w = 2000.0f; /* a wide window */
  spawn_gorilla(world, world->player.x + 1500.0f, world->player.y);
  step(world, NULL, RELEASE_AT + 0.05f);
  CHECK(bananas(world) == 1);
  world_free(world);
}

static void the_banana_goes_over_a_low_wall_in_front_of_him(void) {
  world_t* world = fresh();
  world->player.invulnerable = 1e9f;
  float gx = world->player.x + 500.0f;
  spawn_gorilla(world, gx, world->player.y);
  step(world, NULL, 0.1f);
  int col = (int)((gx - enemy_at(world, 0)->w * 0.5f - 8.0f) / TILE); /* a step, two tiles high */
  world->level.tiles[GROUND_ROW - 1][col] = TILE_STONE;
  world->level.tiles[GROUND_ROW - 2][col] = TILE_STONE;
  step(world, NULL, RELEASE_AT - 0.1f + 0.4f);
  CHECK(world_has_event(world, EVENT_BANANA_THROWN));
  CHECK(bananas(world) == 1);
  CHECK(first_banana(world)->x < (float)(col * TILE)); /* past the step */
  world_free(world);
}

static void under_a_block_he_still_gets_the_banana_away(void) {
  world_t* world = fresh();
  world->player.invulnerable = 1e9f;
  float gx = world->player.x + 400.0f;
  spawn_gorilla(world, gx, world->player.y);
  step(world, NULL, 0.1f);
  /* Blocks just over his head and in front of him, where he swings it. */
  int col = (int)((gx - enemy_at(world, 0)->w * 0.3f) / TILE);
  for (int c = col - 2; c <= col; c++) world->level.tiles[GROUND_ROW - 3][c] = TILE_STONE;
  step(world, NULL, RELEASE_AT - 0.1f + 0.02f);
  CHECK(world_has_event(world, EVENT_BANANA_THROWN));
  CHECK(bananas(world) == 1); /* in the air, not squashed */
  world_free(world);
}

/* A banana flying flat straight at Fabio's middle from close by: sure to hit. */
static void banana_at_fabio(world_t* world) {
  const player_t* p = &world->player;
  banana_t* b = pool_acquire(&world->bananas, NULL);
  *b = (banana_t){p->x + 60.0f, p->y - PLAYER_H * 0.5f, -400.0f, 0.0f, 0.0f, -1};
}

static void a_banana_costs_fabio_one_heart(void) {
  world_t* world = fresh();
  CHECK(world->player.hp == PLAYER_HP);
  banana_at_fabio(world);
  step(world, NULL, 0.3f);
  CHECK(world->player.alive);
  CHECK(world->player.hp == PLAYER_HP - 1);
  CHECK(world->player.invulnerable > 0.0f); /* flashing, like after any hit */
  CHECK(world_has_event(world, EVENT_HURT));
  CHECK(!world_has_event(world, EVENT_DEATH));
  CHECK(bananas(world) == 0);
  world_free(world);
}

static void a_banana_on_his_last_heart_ends_the_game(void) {
  world_t* world = fresh();
  world->player.hp = 1;
  banana_at_fabio(world);
  step(world, NULL, 0.3f);
  CHECK(!world->player.alive);
  CHECK(world_has_event(world, EVENT_DEATH));
  world_free(world);
}

static void a_flashing_fabio_shrugs_a_banana_off(void) {
  world_t* world = fresh();
  world->player.invulnerable = 1e9f;
  banana_at_fabio(world);
  step(world, NULL, 0.3f);
  CHECK(world->player.hp == PLAYER_HP);
  CHECK(bananas(world) == 0); /* it hit him all the same */
  world_free(world);
}

static void he_is_no_marksman_so_fabio_has_a_chance(void) {
  world_t* world = fresh();
  spawn_gorilla(world, world->player.x + 400.0f, world->player.y);
  int hits = 0, misses = 0;
  for (int i = 0; i < 40; i++) {
    world->player.hp = PLAYER_HP;
    world->player.invulnerable = 0.0f;
    world_clear_events(world);
    world_throw_banana(world, enemy_at(world, 0));
    step(world, NULL, 1.6f); /* it lands, one way or the other */
    if (world_has_event(world, EVENT_HURT)) hits++; else misses++;
    for (banana_t* b; (b = first_banana(world));) pool_release_object(&world->bananas, b);
    enemy_at(world, 0)->anim_time = 0.0f; /* he throws only when told to */
  }
  CHECK(hits >= 4);   /* standing still is still dangerous */
  CHECK(misses >= 8); /* but far from certain death */
  world_free(world);
}

static void a_banana_kills_anybody_in_its_way(void) {
  world_t* world = fresh();
  float gx = world->player.x + 400.0f;
  spawn_gorilla(world, gx, world->player.y);
  spawn_monk(world, MONK_HOODED, gx - 60.0f, world->player.y);
  enemy_at(world, 1)->h = 200.0f; /* tall enough to be in the arc's way */
  step(world, NULL, RELEASE_AT + 1.5f);
  CHECK(world_count_enemies(world, ENEMY_MONK) == 0);
  CHECK(world->player.alive);
  CHECK(world_count_enemies(world, ENEMY_GORILLA) == 1);
  world_free(world);
}

static void a_banana_never_harms_its_own_gorilla(void) {
  world_t* world = fresh();
  spawn_gorilla(world, world->player.x + 400.0f, world->player.y);
  world_throw_banana(world, enemy_at(world, 0));
  step(world, NULL, 0.1f);
  CHECK(world_count_enemies(world, ENEMY_GORILLA) == 1);
  world_free(world);
}

static void a_wall_squashes_the_banana(void) {
  world_t* world = fresh();
  world->player.invulnerable = 1e9f;
  int col = (int)(world->player.x / TILE) + 4;
  for (int row = 1; row < GROUND_ROW; row++) world->level.tiles[row][col] = TILE_STONE;
  spawn_gorilla(world, world->player.x + 400.0f, world->player.y);
  step(world, NULL, RELEASE_AT + 2.0f);
  CHECK(world->player.alive);
  CHECK(bananas(world) == 0);
  world_free(world);
}

static void the_banana_spins_along_an_arc(void) {
  world_t* world = fresh();
  world->player.invulnerable = 1e9f;
  spawn_gorilla(world, world->player.x + 500.0f, world->player.y);
  world_throw_banana(world, enemy_at(world, 0));
  banana_t* b = first_banana(world);
  float vy0 = b->vy;
  CHECK(vy0 < 0.0f); /* it goes up first */
  step(world, NULL, 0.3f);
  CHECK(b->vy > vy0);
  CHECK(b->age > 0.25f);
  world_free(world);
}

static void touching_the_gorilla_costs_a_heart(void) {
  world_t* world = fresh();
  spawn_gorilla(world, world->player.x + 10.0f, world->player.y);
  step(world, NULL, 0.05f);
  CHECK(world->player.alive);
  CHECK(world->player.hp == PLAYER_HP - 1);
  CHECK(world->player.vx < 0.0f);
  world_free(world);
}

static void landing_on_him_squashes_him(void) {
  world_t* world = fresh();
  spawn_gorilla(world, world->player.x, world->player.y);
  world->player.y -= 60.0f;
  world->player.vy = 300.0f;
  step(world, NULL, 0.1f);
  CHECK(world->player.hp == PLAYER_HP);
  CHECK(world_count_enemies(world, ENEMY_GORILLA) == 0);
  CHECK(world->gorillas_killed == 1);
  CHECK(world->score == 400);
  CHECK(world_has_event(world, EVENT_GORILLA_DIES));
  world_free(world);
}

static void a_bullet_the_ray_or_the_pickaxe_kill_him(void) {
  world_t* world = fresh();
  world->player.ammo = PISTOL_AMMO;
  spawn_gorilla(world, world->player.x + 250.0f, world->player.y);
  buttons_t fire = BTN_SHOOT;
  step(world, &fire, 0.02f);
  step(world, NULL, 0.5f);
  CHECK(world_count_enemies(world, ENEMY_GORILLA) == 0);

  world->player.ammo = 0;
  world->player.spell_time = 5.0f;
  spawn_gorilla(world, world->player.x + 200.0f, world->player.y);
  step(world, &fire, 0.05f);
  CHECK(world_count_enemies(world, ENEMY_GORILLA) == 0);

  world->player.spell_time = 0.0f;
  spawn_gorilla(world, world->player.x + 50.0f, world->player.y);
  buttons_t swing = BTN_MELEE;
  step(world, &swing, 0.3f);
  CHECK(world_count_enemies(world, ENEMY_GORILLA) == 0);
  CHECK(world->gorillas_killed == 3);
  world_free(world);
}

static void gorillas_turn_up_ahead_of_fabio_one_at_a_time(void) {
  world_t* world = fresh();
  world->spawning = true;
  world->zombie_timer = 1e9f;
  world->bug_timer = 1e9f;
  world->book_reader_timer = 1e9f;
  world->monk_timer = 1e9f;
  world->gorilla_timer = 2.0f;
  step(world, NULL, 1.5f);
  CHECK(world_count_enemies(world, ENEMY_GORILLA) == 0);
  step(world, NULL, 1.0f);
  CHECK(world_count_enemies(world, ENEMY_GORILLA) == 1);
  CHECK(enemy_at(world, 0)->x > world->camera_x + world->view_w);
  world->gorilla_timer = 0.0f; /* another is due, but one is enough */
  step(world, NULL, 3.0f);
  CHECK(world_count_enemies(world, ENEMY_GORILLA) == GORILLA_MAX);
  world_free(world);
}

static void a_burst_gorilla_showers_harmless_bananas_every_way(void) {
  world_t* world = fresh();
  spawn_gorilla(world, world->player.x + 60.0f, world->player.y);
  spawn_zombie(world, world->player.x + 100.0f, world->player.y);
  enemy_at(world, 1)->speed = 0.0f; /* stays in the shower */
  world_kill_enemy(world, enemy_at(world, 0), false);
  int pieces = (int)pool_get_active_count(&world->debris);
  CHECK(pieces == BANANA_SHOWER);
  CHECK(bananas(world) == 0); /* none of them is a thrown banana */
  int left = 0, right = 0, up = 0, down = 0;
  size_t cursor = 0;
  for (const debris_t* d; (d = pool_next_active(&world->debris, &cursor));) {
    CHECK(d->kind == DEBRIS_BANANA);
    CHECK(d->grow == 1.0f); /* bananas keep their size */
    left += d->vx < 0.0f, right += d->vx > 0.0f, up += d->vy < 0.0f, down += d->vy > 0.0f;
  }
  CHECK(left > 15 && right > 15 && up > 15 && down > 15);
  step(world, NULL, 0.3f); /* too soon for any to leave the level */
  CHECK(world->player.hp == PLAYER_HP); /* Fabio stood right in it */
  CHECK(world_count_enemies(world, ENEMY_ZOMBIE) == 1);
  CHECK((int)pool_get_active_count(&world->debris) == BANANA_SHOWER); /* nothing squashed them */
  step(world, NULL, 1.4f);
  CHECK(pool_get_active_count(&world->debris) == 0); /* all faded away */
  world_free(world);
}

void run_gorilla_tests(void) {
  RUN(the_gorilla_stands_where_he_appeared);
  RUN(he_turns_to_face_fabio_as_fabio_moves);
  RUN(he_stands_roars_beats_his_chest_and_throws_in_turn);
  RUN(his_frames_follow_what_he_is_doing);
  RUN(he_throws_one_banana_per_round_at_fabio);
  RUN(no_banana_while_he_is_off_screen);
  RUN(he_throws_from_anywhere_on_screen);
  RUN(the_banana_goes_over_a_low_wall_in_front_of_him);
  RUN(under_a_block_he_still_gets_the_banana_away);
  RUN(a_banana_costs_fabio_one_heart);
  RUN(a_banana_on_his_last_heart_ends_the_game);
  RUN(a_flashing_fabio_shrugs_a_banana_off);
  RUN(he_is_no_marksman_so_fabio_has_a_chance);
  RUN(a_banana_kills_anybody_in_its_way);
  RUN(a_banana_never_harms_its_own_gorilla);
  RUN(a_wall_squashes_the_banana);
  RUN(the_banana_spins_along_an_arc);
  RUN(touching_the_gorilla_costs_a_heart);
  RUN(landing_on_him_squashes_him);
  RUN(a_bullet_the_ray_or_the_pickaxe_kill_him);
  RUN(a_burst_gorilla_showers_harmless_bananas_every_way);
  RUN(gorillas_turn_up_ahead_of_fabio_one_at_a_time);
}
