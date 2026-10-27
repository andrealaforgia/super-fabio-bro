#include <string.h>

#include "bot.h"
#include "pose.h"
#include "test_groups.h"
#include "test_support.h"
#include "test_world.h"

static world_t* fresh(void) {
  world_t* world = new_empty_world();
  settle(world);
  return world;
}

static int live_effects(const world_t* world, fx_kind_t kind) {
  int n = 0;
  size_t cursor = 0;
  for (const effect_t* fx; (fx = pool_next_active(&world->effects, &cursor));)
    n += fx->kind == kind;
  return n;
}

static void the_monk_stands_exactly_where_he_appeared(void) {
  world_t* world = fresh();
  world->player.invulnerable = 1e9f;
  world->player.x = 5 * TILE;
  float x = 30 * TILE;
  spawn_monk(world, MONK_HOODED, x, world->player.y - 100.0f);
  step(world, NULL, 3.0f);
  CHECK(enemy_at(world, 0)->x == x);
  CHECK(enemy_at(world, 0)->grounded);
  CHECK(enemy_at(world, 0)->y == (float)(GROUND_ROW * TILE));
  world_free(world);
}

static void they_always_face_the_way_fabio_is_coming_from(void) {
  world_t* world = fresh();
  world->player.invulnerable = 1e9f;
  world->player.x = 5 * TILE;
  spawn_monk(world, MONK_HOODED, 20 * TILE, world->player.y);
  spawn_monk(world, MONK_SKELETON, 30 * TILE, world->player.y);
  step(world, NULL, 0.1f);
  CHECK(enemy_at(world, 0)->facing == -1 && enemy_at(world, 1)->facing == -1);
  world->player.x = 40 * TILE; /* he has run past them */
  step(world, NULL, 0.1f);
  CHECK(enemy_at(world, 0)->facing == 1 && enemy_at(world, 1)->facing == 1);
  world_free(world);
}

static void touching_the_monk_pushes_fabio_back_and_costs_a_heart(void) {
  world_t* world = fresh();
  spawn_monk(world, MONK_HOODED, world->player.x + 10.0f, world->player.y);
  step(world, NULL, 0.05f);
  CHECK(world->player.alive);
  CHECK(world->player.hp == PLAYER_HP - 1);
  CHECK(world->player.vx < 0.0f); /* thrown away from him */
  CHECK(world_has_event(world, EVENT_HURT));
  CHECK(world_count_enemies(world, ENEMY_MONK) == 1);
  world_free(world);
}

static void fabio_flashing_after_a_hit_is_not_hurt_again(void) {
  world_t* world = fresh();
  world->player.invulnerable = 1000.0f;
  spawn_monk(world, MONK_HOODED, world->player.x + 10.0f, world->player.y);
  step(world, NULL, 0.05f);
  CHECK(world->player.hp == PLAYER_HP);
  world_free(world);
}

static void landing_on_the_monk_squashes_him_and_bounces_fabio(void) {
  world_t* world = fresh();
  spawn_monk(world, MONK_HOODED, world->player.x, world->player.y);
  world->player.y -= 50.0f;
  world->player.vy = 300.0f;
  step(world, NULL, 0.1f);
  CHECK(world->player.alive && world->player.hp == PLAYER_HP);
  CHECK(world->player.vy < 0.0f);
  CHECK(world_count_enemies(world, ENEMY_MONK) == 0);
  CHECK(world->monks_killed == 1);
  CHECK(world->score == 300);
  CHECK(live_effects(world, FX_MONK_BOOM) == 1);
  world_free(world);
}

static void a_bullet_blows_the_monk_up_and_fabio_lives(void) {
  world_t* world = fresh();
  world->player.ammo = PISTOL_AMMO;
  spawn_monk(world, MONK_HOODED, world->player.x + 250.0f, world->player.y);
  buttons_t fire = BTN_SHOOT;
  step(world, &fire, 0.02f);
  step(world, NULL, 0.6f);
  CHECK(world_count_enemies(world, ENEMY_MONK) == 0);
  CHECK(world->monks_killed == 1);
  CHECK(world->score == 300);
  CHECK(world_has_event(world, EVENT_MONK_DIES));
  CHECK(live_effects(world, FX_MONK_BOOM) == 1);
  CHECK(world->player.alive);
  world_free(world);
}

static void the_pickaxe_reaches_the_monk_before_he_reaches_fabio(void) {
  world_t* world = fresh();
  spawn_monk(world, MONK_HOODED, world->player.x + 44.0f, world->player.y);
  buttons_t swing = BTN_MELEE;
  step(world, &swing, 0.3f);
  CHECK(world_count_enemies(world, ENEMY_MONK) == 0);
  CHECK(world->player.alive);
  world_free(world);
}

static void the_tdd_ray_blows_him_up_too(void) {
  world_t* world = fresh();
  world->player.spell_time = 5.0f;
  spawn_monk(world, MONK_HOODED, world->player.x + 200.0f, world->player.y);
  buttons_t fire = BTN_SHOOT;
  step(world, &fire, 0.05f);
  CHECK(world_count_enemies(world, ENEMY_MONK) == 0);
  CHECK(world->player.alive);
  world_free(world);
}

static void the_explosion_runs_through_every_frame_and_ends(void) {
  CHECK(pose_monk_boom(0.0f).strip == STRIP_MONK_BOOM);
  CHECK(pose_monk_boom(0.0f).frame == 0);
  int previous = 0;
  for (float age = 0.0f; age < 1.9f; age += 0.01f) {
    int frame = pose_monk_boom(age).frame;
    CHECK(frame >= previous);
    CHECK(frame - previous <= 1);
    previous = frame;
  }
  CHECK(previous == kStripSpecs[STRIP_MONK_BOOM].frames - 1);
  world_t* world = fresh();
  spawn_monk(world, MONK_HOODED, world->player.x + 300.0f, world->player.y);
  world_kill_enemy(world, enemy_at(world, 0), false);
  step(world, NULL, 0.9f);
  CHECK(live_effects(world, FX_MONK_BOOM) == 1);
  step(world, NULL, 1.5f);
  CHECK(live_effects(world, FX_MONK_BOOM) == 0);
  world_free(world);
}

static void monks_turn_up_ahead_of_fabio_but_never_more_than_two(void) {
  world_t* world = fresh();
  world->spawning = true;
  world->zombie_timer = 1e9f;
  world->bug_timer = 1e9f;
  world->book_reader_timer = 1e9f;
  world->monk_timer = 2.0f;
  world->player.invulnerable = 1e9f;
  step(world, NULL, 1.5f);
  CHECK(world_count_enemies(world, ENEMY_MONK) == 0);
  step(world, NULL, 1.0f);
  CHECK(world_count_enemies(world, ENEMY_MONK) == 1);
  CHECK(enemy_at(world, 0)->x > world->camera_x + world->view_w);
  int most = 0;
  for (int i = 0; i < 120; i++) {
    step(world, NULL, 1.0f);
    int n = world_count_enemies(world, ENEMY_MONK);
    if (n > most) most = n;
  }
  CHECK(most >= 1 && most <= MONK_MAX);
  world_free(world);
}

static void the_skeleton_pushes_back_and_can_be_stomped_too(void) {
  world_t* world = fresh();
  spawn_monk(world, MONK_SKELETON, world->player.x + 10.0f, world->player.y);
  step(world, NULL, 0.05f);
  CHECK(world->player.alive && world->player.hp == PLAYER_HP - 1);
  CHECK(world->player.vx < 0.0f);
  world_free(world);
  world = fresh();
  spawn_monk(world, MONK_SKELETON, world->player.x, world->player.y);
  world->player.y -= 50.0f;
  world->player.vy = 300.0f;
  step(world, NULL, 0.1f);
  CHECK(world->player.hp == PLAYER_HP);
  CHECK(world_count_enemies(world, ENEMY_MONK) == 0);
  CHECK(live_effects(world, FX_SKELETON_VANISH) == 1);
  world_free(world);
}

static void a_bullet_makes_the_skeleton_vanish_not_explode(void) {
  world_t* world = fresh();
  world->player.ammo = PISTOL_AMMO;
  spawn_monk(world, MONK_SKELETON, world->player.x + 250.0f, world->player.y);
  buttons_t fire = BTN_SHOOT;
  step(world, &fire, 0.02f);
  step(world, NULL, 0.6f);
  CHECK(world_count_enemies(world, ENEMY_MONK) == 0);
  CHECK(world->monks_killed == 1);
  CHECK(world->score == 300);
  CHECK(world_has_event(world, EVENT_MONK_DIES));
  CHECK(live_effects(world, FX_SKELETON_VANISH) == 1);
  CHECK(live_effects(world, FX_MONK_BOOM) == 0);
  CHECK(world->player.alive);
  world_free(world);
}

static void the_vanishing_runs_through_every_frame_and_ends(void) {
  CHECK(pose_skeleton_vanish(0.0f).strip == STRIP_SKELETON_VANISH);
  CHECK(pose_skeleton_vanish(0.0f).frame == 0);
  int previous = 0;
  for (float age = 0.0f; age < 1.0f; age += 0.005f) {
    int frame = pose_skeleton_vanish(age).frame;
    CHECK(frame >= previous && frame - previous <= 1);
    previous = frame;
  }
  CHECK(previous == kStripSpecs[STRIP_SKELETON_VANISH].frames - 1);
  world_t* world = fresh();
  spawn_monk(world, MONK_SKELETON, world->player.x + 300.0f, world->player.y);
  world_kill_enemy(world, enemy_at(world, 0), false);
  step(world, NULL, 0.3f);
  CHECK(live_effects(world, FX_SKELETON_VANISH) == 1);
  step(world, NULL, 0.4f);
  CHECK(live_effects(world, FX_SKELETON_VANISH) == 0); /* quick: gone in well under a second */
  world_free(world);
}

static void both_kinds_turn_up_over_time(void) {
  world_t* world = fresh();
  world->spawning = true;
  world->zombie_timer = 1e9f;
  world->bug_timer = 1e9f;
  world->book_reader_timer = 1e9f;
  world->monk_timer = 0.1f;
  world->player.invulnerable = 1e9f;
  bool seen[MONK_VARIANTS] = {false};
  for (int i = 0; i < 60 * 30 * 4; i++) {
    step(world, NULL, 1.0f / 60.0f);
    size_t cursor = 0;
    for (enemy_t* e; (e = pool_next_active(&world->enemies, &cursor));) {
      if (e->kind != ENEMY_MONK) continue;
      seen[e->variant] = true;
      pool_release_object(&world->enemies, e);
    }
  }
  CHECK(seen[MONK_HOODED] && seen[MONK_SKELETON]);
  world_free(world);
}

static void the_skeleton_stands_taller_than_the_monk(void) {
  world_t* world = fresh();
  spawn_monk(world, MONK_HOODED, world->player.x + 200.0f, world->player.y);
  spawn_monk(world, MONK_SKELETON, world->player.x + 300.0f, world->player.y);
  CHECK(enemy_at(world, 1)->h > enemy_at(world, 0)->h + 8.0f);
  CHECK(enemy_at(world, 1)->w > enemy_at(world, 0)->w);
  world_free(world);
}

static void the_demo_bot_deals_with_a_monk_in_its_way(void) {
  world_t* world = fresh();
  bot_t bot;
  memset(&bot, 0, sizeof(bot));
  spawn_monk(world, MONK_HOODED, world->player.x + 400.0f, world->player.y);
  controls_t controls;
  memset(&controls, 0, sizeof(controls));
  for (int i = 0; i < 60 * 8; i++) {
    controls_update(&controls, bot_decide(world, &bot, 1.0f / 60.0f));
    world_update(world, &controls, 1.0f / 60.0f);
  }
  CHECK(world->player.alive);
  CHECK(world->monks_killed == 1);
  world_free(world);
}

void run_monk_tests(void) {
  RUN(the_monk_stands_exactly_where_he_appeared);
  RUN(they_always_face_the_way_fabio_is_coming_from);
  RUN(touching_the_monk_pushes_fabio_back_and_costs_a_heart);
  RUN(fabio_flashing_after_a_hit_is_not_hurt_again);
  RUN(landing_on_the_monk_squashes_him_and_bounces_fabio);
  RUN(a_bullet_blows_the_monk_up_and_fabio_lives);
  RUN(the_pickaxe_reaches_the_monk_before_he_reaches_fabio);
  RUN(the_tdd_ray_blows_him_up_too);
  RUN(the_explosion_runs_through_every_frame_and_ends);
  RUN(monks_turn_up_ahead_of_fabio_but_never_more_than_two);
  RUN(the_skeleton_pushes_back_and_can_be_stomped_too);
  RUN(a_bullet_makes_the_skeleton_vanish_not_explode);
  RUN(the_vanishing_runs_through_every_frame_and_ends);
  RUN(both_kinds_turn_up_over_time);
  RUN(the_skeleton_stands_taller_than_the_monk);
  RUN(the_demo_bot_deals_with_a_monk_in_its_way);
}
