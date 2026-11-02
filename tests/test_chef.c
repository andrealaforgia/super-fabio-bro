#include <math.h>
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

static void the_chef_stands_where_he_appeared_facing_fabio(void) {
  world_t* world = fresh();
  world->player.x = 5 * TILE;
  float x = 30 * TILE;
  spawn_chef(world, x, world->player.y - 100.0f);
  step(world, NULL, 2.0f);
  const enemy_t* chef = enemy_at(world, 0);
  CHECK(chef->x == x && chef->grounded);
  CHECK(chef->facing == -1);
  world->player.x = 40 * TILE; /* Fabio runs past: the chef turns */
  step(world, NULL, 0.1f);
  CHECK(enemy_at(world, 0)->facing == 1);
  world_free(world);
}

static void far_from_fabio_he_smiles_and_close_to_him_he_swings_the_knife(void) {
  world_t* world = fresh();
  world->player.invulnerable = 1e9f;
  spawn_chef(world, world->player.x + CHEF_ATTACK_RANGE + 80.0f, world->player.y);
  step(world, NULL, 0.5f);
  CHECK(!chef_attacking(enemy_at(world, 0)));
  CHECK(pose_chef(enemy_at(world, 0)).strip == STRIP_CHEF_HAPPY);
  world->player.x += 100.0f;
  step(world, NULL, 0.05f);
  CHECK(chef_attacking(enemy_at(world, 0)));
  CHECK(pose_chef(enemy_at(world, 0)).strip == STRIP_CHEF_ATTACK);
  world->player.x -= 100.0f; /* Fabio backs off: he smiles again */
  step(world, NULL, 0.05f);
  CHECK(pose_chef(enemy_at(world, 0)).strip == STRIP_CHEF_HAPPY);
  world_free(world);
}

static void the_smile_runs_through_all_its_frames_and_the_swing_through_all_its_own(void) {
  world_t* world = fresh();
  world->player.invulnerable = 1e9f;
  spawn_chef(world, world->player.x + 400.0f, world->player.y);
  bool smile[2] = {false};
  for (int i = 0; i < 240; i++) {
    step(world, NULL, 1.0f / 60.0f);
    smile[pose_chef(enemy_at(world, 0)).frame] = true;
  }
  for (int f = 0; f < 2; f++) CHECK(smile[f]);
  world->player.x = enemy_at(world, 0)->x - 100.0f;
  bool swing[CHEF_SWING_FRAMES] = {false};
  for (int i = 0; i < 120; i++) {
    step(world, NULL, 1.0f / 60.0f);
    swing[pose_chef(enemy_at(world, 0)).frame] = true;
  }
  for (int f = 0; f < CHEF_SWING_FRAMES; f++) CHECK(swing[f]);
  world_free(world);
}

static void the_knife_hurts_fabio_from_beyond_the_chefs_body_once_it_is_out(void) {
  world_t* world = fresh();
  spawn_chef(world, world->player.x + 60.0f, world->player.y);
  enemy_t* chef = enemy_at(world, 0);
  CHECK(chef->x - chef->w * 0.5f > world->player.x + PLAYER_W * 0.5f); /* not touching */
  bool hurt = false;
  for (int i = 0; i < 60 && !hurt; i++) {
    world->player.x = chef->x - 60.0f;
    step(world, NULL, 1.0f / 60.0f);
    hurt = world->player.hp < PLAYER_HP;
  }
  CHECK(hurt);
  CHECK(world->player.alive);
  CHECK(world_has_event(world, EVENT_HURT));
  world_free(world);
}

static void out_of_reach_of_the_blade_fabio_is_safe(void) {
  world_t* world = fresh();
  spawn_chef(world, world->player.x + CHEF_ATTACK_RANGE - 10.0f, world->player.y);
  for (int i = 0; i < 60; i++) {
    world->player.x = enemy_at(world, 0)->x - (CHEF_KNIFE_REACH + 40.0f);
    step(world, NULL, 1.0f / 60.0f);
  }
  CHECK(chef_attacking(enemy_at(world, 0)));
  CHECK(world->player.hp == PLAYER_HP);
  world_free(world);
}

static void landing_on_the_chef_kills_him(void) {
  world_t* world = fresh();
  spawn_chef(world, world->player.x, world->player.y);
  world->player.y -= 50.0f;
  world->player.vy = 300.0f;
  step(world, NULL, 0.1f);
  CHECK(world->player.alive && world->player.hp == PLAYER_HP);
  CHECK(world->player.vy < 0.0f);
  CHECK(world_count_enemies(world, ENEMY_CHEF) == 0);
  CHECK(world->chefs_killed == 1);
  CHECK(world->score == 600);
  CHECK(world_has_event(world, EVENT_CHEF_DIES));
  CHECK(live_effects(world, FX_CHEF_EXPLODE) == 1);
  world_free(world);
}

static void a_bullet_the_pickaxe_and_the_ray_each_kill_the_chef(void) {
  for (int weapon = 0; weapon < 3; weapon++) {
    world_t* world = fresh();
    world->player.invulnerable = 1e9f;
    world->player.ammo = PISTOL_AMMO;
    world->player.spell_time = weapon == 2 ? 5.0f : 0.0f;
    float away = weapon == 1 ? 44.0f : 250.0f;
    spawn_chef(world, world->player.x + away, world->player.y);
    buttons_t press = weapon == 1 ? BTN_MELEE : BTN_SHOOT;
    step(world, &press, 0.05f);
    step(world, NULL, 0.5f);
    CHECK(world_count_enemies(world, ENEMY_CHEF) == 0);
      world_free(world);
  }
}

static void the_chef_shows_two_blast_frames(void) {
  CHECK(pose_chef_death(0.0f).strip == STRIP_CHEF_BOOM && pose_chef_death(0.0f).frame == 0);
  CHECK(pose_chef_death(CHEF_EXPLODE_SECONDS).frame == 1);
}

static void chefs_turn_up_ahead_of_fabio_but_never_more_than_one(void) {
  world_t* world = fresh();
  world->player.x = 10 * TILE;
  world->spawning = true;
  world->zombie_timer = 1e9f;
  world->bug_timer = 1e9f;
  world->book_reader_timer = 1e9f;
  world->monk_timer = 1e9f;
  world->chef_timer = 0.1f;
  world->player.invulnerable = 1e9f;
  world->view_w = 1000.0f;
  world->camera_x = world->player.x - 400.0f;
  step(world, NULL, 0.5f);
  CHECK(world_count_enemies(world, ENEMY_CHEF) == 1);
  CHECK(enemy_at(world, 0)->x > world->player.x);
  world->chef_timer = 0.1f;
  step(world, NULL, 0.5f);
  CHECK(world_count_enemies(world, ENEMY_CHEF) == 1);
  world_free(world);
}

static void a_chef_needs_flat_grass_and_open_air(void) {
  world_t* world = fresh();
  world->player.x = 5 * TILE;
  float y = (float)(GROUND_ROW * TILE);
  CHECK(chef_has_room(world, 30 * TILE, y));
  world->level.tiles[GROUND_ROW][30] = TILE_EMPTY; /* a pit beside him */
  CHECK(!chef_has_room(world, 30 * TILE, y));
  world->level.tiles[GROUND_ROW][30] = TILE_ROCK; /* not grass */
  CHECK(!chef_has_room(world, 30 * TILE, y));
  world->level.tiles[GROUND_ROW][30] = TILE_GRASS;
  world->level.tiles[GROUND_ROW - 2][30] = TILE_BRICK; /* a block in his way overhead */
  CHECK(!chef_has_room(world, 30 * TILE, y));
  world->level.tiles[GROUND_ROW - 2][30] = TILE_EMPTY;
  CHECK(chef_has_room(world, 30 * TILE, y));
  CHECK(!chef_has_room(world, (float)(LEVEL_COLS * TILE - 10), y)); /* off the end */
  world_free(world);
}

static void no_chef_spawns_where_there_is_no_room(void) {
  world_t* world = fresh();
  world->spawning = true;
  world->zombie_timer = 1e9f;
  world->bug_timer = 1e9f;
  world->book_reader_timer = 1e9f;
  world->monk_timer = 1e9f;
  world->chef_timer = 0.1f;
  world->player.x = 10 * TILE;
  world->player.invulnerable = 1e9f;
  world->view_w = 1000.0f;
  world->camera_x = world->player.x - 400.0f;
  for (int col = 0; col < LEVEL_COLS; col++) world->level.tiles[GROUND_ROW - 3][col] = TILE_BRICK;
  step(world, NULL, 3.0f);
  CHECK(world_count_enemies(world, ENEMY_CHEF) == 0);
  world_free(world);
}

static void every_generated_level_has_plenty_of_room_for_a_chef(void) {
  for (uint32_t seed = 1; seed <= 12; seed++) {
    world_t* world = world_new(seed, NULL);
    CHECK(world != NULL);
    world->player.x = 5 * TILE;
    int spots = 0;
    for (int col = 12; col < LEVEL_COLS - 20; col++)
      spots += chef_has_room(world, (float)(col * TILE), (float)(GROUND_ROW * TILE));
    CHECK(spots >= 20);
    world_free(world);
  }
}

static void new_characters_never_appear_on_top_of_one_another(void) {
  for (uint32_t seed = 1; seed <= 12; seed++) {
    world_t* world = world_new(seed, NULL);
    CHECK(world != NULL);
    world->spawning = true;
    world->zombie_timer = 1e9f;
    world->bug_timer = 1e9f;
    world->player.invulnerable = 1e9f;
    world->player.x = 10 * TILE;
    world->view_w = 1000.0f;
    for (int i = 0; i < 60 * 240; i++) {
      world->book_reader_timer = fminf(world->book_reader_timer, 0.05f);
      world->monk_timer = fminf(world->monk_timer, 0.05f);
      world->chef_timer = fminf(world->chef_timer, 0.05f);
      step(world, NULL, 1.0f / 60.0f);
      world->player.x += 1.0f; /* keep walking so that fresh ground comes up ahead */
      world->camera_x = world->player.x - 300.0f;
    }
    size_t a = 0;
    for (const enemy_t* one; (one = pool_next_active(&world->enemies, &a));) {
      size_t b = 0;
      for (const enemy_t* other; (other = pool_next_active(&world->enemies, &b));) {
        if (one == other || one->kind == ENEMY_ZOMBIE || other->kind == ENEMY_ZOMBIE) continue;
        if (one->kind == ENEMY_BUG || other->kind == ENEMY_BUG) continue;
        if (one->kind == ENEMY_MONK && one->variant == MONK_SKELETON) continue;
        if (other->kind == ENEMY_MONK && other->variant == MONK_SKELETON) continue;
        CHECK(fabsf(one->x - other->x) >= CHARACTER_SPACING - 1.0f);
      }
    }
    world_free(world);
  }
}

static void the_demo_bot_deals_with_a_chef_in_its_way(void) {
  world_t* world = fresh();
  world->player.invulnerable = 1e9f;
  spawn_chef(world, world->player.x + 100.0f, world->player.y);
  bot_t bot;
  memset(&bot, 0, sizeof(bot));
  controls_t controls;
  memset(&controls, 0, sizeof(controls));
  for (int i = 0; i < 240; i++) {
    controls_update(&controls, bot_decide(world, &bot, 1.0f / 60.0f));
    world_update(world, &controls, 1.0f / 60.0f);
  }
  CHECK(world->chefs_killed == 1);
  world_free(world);
}

void run_chef_tests(void) {
  RUN(the_chef_stands_where_he_appeared_facing_fabio);
  RUN(far_from_fabio_he_smiles_and_close_to_him_he_swings_the_knife);
  RUN(the_smile_runs_through_all_its_frames_and_the_swing_through_all_its_own);
  RUN(the_knife_hurts_fabio_from_beyond_the_chefs_body_once_it_is_out);
  RUN(out_of_reach_of_the_blade_fabio_is_safe);
  RUN(landing_on_the_chef_kills_him);
  RUN(a_bullet_the_pickaxe_and_the_ray_each_kill_the_chef);
  RUN(the_chef_shows_two_blast_frames);
  RUN(chefs_turn_up_ahead_of_fabio_but_never_more_than_one);
  RUN(a_chef_needs_flat_grass_and_open_air);
  RUN(no_chef_spawns_where_there_is_no_room);
  RUN(every_generated_level_has_plenty_of_room_for_a_chef);
  RUN(new_characters_never_appear_on_top_of_one_another);
  RUN(the_demo_bot_deals_with_a_chef_in_its_way);
}
