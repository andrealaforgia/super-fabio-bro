#include <string.h>

#include "test_groups.h"
#include "test_support.h"
#include "test_world.h"

static world_t* fresh(void) {
  world_t* world = malloc(sizeof(*world));
  empty_world(world);
  settle(world);
  world->player.invulnerable = 1000.0f;
  return world;
}

static buttons_t shoot_button(void) {
  buttons_t b;
  memset(&b, 0, sizeof(b));
  b.shoot = true;
  return b;
}

static int live_bullets(const world_t* world) {
  int n = 0;
  for (int i = 0; i < MAX_BULLETS; i++) n += world->bullets[i].alive;
  return n;
}

static int live_effects(const world_t* world, fx_kind_t kind) {
  int n = 0;
  for (int i = 0; i < MAX_EFFECTS; i++)
    n += world->effects[i].alive && world->effects[i].kind == kind;
  return n;
}

static void without_a_gun_shooting_does_nothing(void) {
  world_t* world = fresh();
  buttons_t fire = shoot_button();
  step(world, &fire, 0.5f);
  CHECK(live_bullets(world) == 0);
  CHECK(!world_has_event(world, EVENT_SHOOT));
  free(world);
}

static void with_a_gun_a_bullet_leaves_the_muzzle(void) {
  world_t* world = fresh();
  world->player.has_gun = true;
  buttons_t fire = shoot_button();
  step(world, &fire, 0.02f);
  CHECK(live_bullets(world) == 1);
  CHECK(live_effects(world, FX_MUZZLE) == 1);
  free(world);
}

static void a_bullet_explodes_a_zombie(void) {
  world_t* world = fresh();
  world->player.has_gun = true;
  spawn_zombie(world, world->player.x + 250.0f, world->player.y);
  world->enemies[0].speed = 0.0f;
  buttons_t fire = shoot_button();
  step(world, &fire, 0.02f);
  step(world, NULL, 0.6f);
  CHECK(world_count_enemies(world, ENEMY_ZOMBIE) == 0);
  CHECK(world->zombies_killed == 1);
  CHECK(world->score == 200);
  CHECK(live_bullets(world) == 0);
  int particles = 0;
  for (int i = 0; i < MAX_PARTICLES; i++) particles += world->particles[i].alive;
  CHECK(particles > 0 || live_effects(world, FX_ZOMBIE_BOOM) > 0);
  free(world);
}

static void the_explosion_effect_shows_then_fades(void) {
  world_t* world = fresh();
  spawn_zombie(world, world->player.x + 100.0f, world->player.y);
  world_kill_enemy(world, &world->enemies[0], false);
  CHECK(live_effects(world, FX_ZOMBIE_BOOM) == 1);
  CHECK(world_has_event(world, EVENT_ZOMBIE_DIES));
  step(world, NULL, 1.0f);
  CHECK(live_effects(world, FX_ZOMBIE_BOOM) == 0);
  free(world);
}

static void a_bullet_explodes_a_bug(void) {
  world_t* world = fresh();
  world->player.has_gun = true;
  spawn_bug(world, BUG_BEETLE, world->player.x + 200.0f, world->player.y);
  world->enemies[0].speed = 0.0f;
  buttons_t fire = shoot_button();
  step(world, &fire, 0.02f);
  step(world, NULL, 0.5f);
  CHECK(world->bugs_killed == 1);
  CHECK(world_has_event(world, EVENT_BUG_DIES) || world->score == 100);
  free(world);
}

static void a_wall_stops_a_bullet(void) {
  world_t* world = fresh();
  world->player.has_gun = true;
  world->level.tiles[GROUND_ROW - 1][14] = TILE_STONE;
  world->level.tiles[GROUND_ROW - 2][14] = TILE_STONE;
  spawn_zombie(world, 18 * TILE, world->player.y);
  world->enemies[0].speed = 0.0f;
  buttons_t fire = shoot_button();
  step(world, &fire, 0.02f);
  step(world, NULL, 0.6f);
  CHECK(world_count_enemies(world, ENEMY_ZOMBIE) == 1);
  CHECK(live_bullets(world) == 0);
  free(world);
}

static void the_gun_fires_a_few_times_a_second_not_every_frame(void) {
  world_t* world = fresh();
  world->player.has_gun = true;
  buttons_t fire = shoot_button();
  int shots = 0;
  for (int i = 0; i < 240; i++) {
    world_clear_events(world);
    step(world, &fire, 1.0f / 120.0f);
    shots += world_has_event(world, EVENT_SHOOT);
  }
  CHECK(shots >= 5 && shots <= 9);
  free(world);
}

static void shooting_goes_the_way_he_faces(void) {
  world_t* world = fresh();
  world->player.has_gun = true;
  buttons_t left;
  memset(&left, 0, sizeof(left));
  left.left = true;
  step(world, &left, 0.05f);
  CHECK(world->player.facing == -1);
  spawn_zombie(world, world->player.x - 220.0f, world->player.y);
  world->enemies[0].speed = 0.0f;
  buttons_t fire = shoot_button();
  fire.left = true;
  step(world, &fire, 0.02f);
  fire.left = false;
  step(world, NULL, 0.6f);
  CHECK(world->zombies_killed == 1);
  free(world);
}

static void the_pickaxe_kills_what_is_in_front_without_a_gun(void) {
  world_t* world = fresh();
  spawn_zombie(world, world->player.x + 40.0f, world->player.y);
  world->enemies[0].speed = 0.0f;
  buttons_t swing;
  memset(&swing, 0, sizeof(swing));
  swing.melee = true;
  step(world, &swing, 0.02f);
  step(world, NULL, 0.4f);
  CHECK(world->zombies_killed == 1);
  CHECK(world_has_event(world, EVENT_MELEE) || world->zombies_killed == 1);
  free(world);
}

static void the_pickaxe_does_not_reach_behind_him(void) {
  world_t* world = fresh();
  spawn_zombie(world, world->player.x - 60.0f, world->player.y);
  world->enemies[0].speed = 0.0f;
  buttons_t swing;
  memset(&swing, 0, sizeof(swing));
  swing.melee = true;
  step(world, &swing, 0.02f);
  step(world, NULL, 0.4f);
  CHECK(world->zombies_killed == 0);
  free(world);
}

static void jumping_on_a_zombie_squashes_it_and_bounces(void) {
  world_t* world = fresh();
  world->player.y = GROUND_ROW * TILE - 90.0f;
  world->player.vy = 300.0f;
  world->player.grounded = false;
  spawn_zombie(world, world->player.x, GROUND_ROW * TILE);
  world->enemies[0].speed = 0.0f;
  step(world, NULL, 0.3f);
  CHECK(world->zombies_killed == 1);
  CHECK(world->player.hp == PLAYER_HP);
  CHECK(world->player.vy < 0.0f || !world->player.grounded);
  free(world);
}

static void drinking_the_potion_starts_a_spell_that_fades_away(void) {
  world_t* world = fresh();
  world->player.potions = 1;
  buttons_t drink;
  memset(&drink, 0, sizeof(drink));
  drink.drink = true;
  step(world, &drink, 0.02f);
  CHECK(world->player.potions == 0);
  CHECK(world->player.spell_time > POTION_SECONDS - 0.1f);
  CHECK(world_has_event(world, EVENT_DRINK));
  step(world, NULL, POTION_SECONDS * 0.5f);
  CHECK(world->player.spell_time > 0.0f);
  step(world, NULL, POTION_SECONDS * 0.5f + 0.1f);
  CHECK_NEAR(world->player.spell_time, 0.0, 1e-6);
  free(world);
}

static void you_cannot_drink_what_you_do_not_have(void) {
  world_t* world = fresh();
  buttons_t drink;
  memset(&drink, 0, sizeof(drink));
  drink.drink = true;
  step(world, &drink, 0.1f);
  CHECK(world->player.spell_time == 0.0f);
}

static void a_second_potion_waits_until_the_first_has_faded(void) {
  world_t* world = fresh();
  world->player.potions = 2;
  buttons_t drink;
  memset(&drink, 0, sizeof(drink));
  drink.drink = true;
  step(world, &drink, 0.02f);
  step(world, NULL, 0.1f);
  step(world, &drink, 0.02f);
  CHECK(world->player.potions == 1);
  free(world);
}

static void under_the_spell_shooting_casts_a_ray_even_without_a_gun(void) {
  world_t* world = fresh();
  world->player.spell_time = 10.0f;
  CHECK(!world->player.has_gun);
  spawn_zombie(world, world->player.x + 150.0f, world->player.y);
  spawn_zombie(world, world->player.x + 330.0f, world->player.y);
  spawn_bug(world, BUG_BEETLE, world->player.x + 480.0f, world->player.y);
  for (int i = 0; i < 3; i++) world->enemies[i].speed = 0.0f;
  buttons_t cast = shoot_button();
  step(world, &cast, 0.02f);
  CHECK(world_has_event(world, EVENT_SPELL));
  CHECK(world->zombies_killed == 2);
  CHECK(world->bugs_killed == 1);
  CHECK(live_effects(world, FX_ZOMBIE_GREEN) == 2);
  free(world);
}

static void the_ray_does_not_pass_through_walls(void) {
  world_t* world = fresh();
  world->player.spell_time = 10.0f;
  world->level.tiles[GROUND_ROW - 1][14] = TILE_STONE;
  world->level.tiles[GROUND_ROW - 2][14] = TILE_STONE;
  spawn_zombie(world, 17 * TILE, world->player.y);
  world->enemies[0].speed = 0.0f;
  buttons_t cast = shoot_button();
  step(world, &cast, 0.02f);
  CHECK(world->zombies_killed == 0);
  free(world);
}

static void once_the_spell_fades_he_is_unarmed_again(void) {
  world_t* world = fresh();
  world->player.spell_time = 0.2f;
  step(world, NULL, 0.5f);
  spawn_zombie(world, world->player.x + 150.0f, world->player.y);
  world->enemies[0].speed = 0.0f;
  buttons_t cast = shoot_button();
  step(world, &cast, 0.3f);
  CHECK(world->zombies_killed == 0);
  free(world);
}

static void the_spell_outranks_the_pistol_but_the_pistol_returns(void) {
  world_t* world = fresh();
  world->player.has_gun = true;
  world->player.spell_time = 0.3f;
  buttons_t fire = shoot_button();
  step(world, &fire, 0.1f);
  CHECK(world_has_event(world, EVENT_SPELL));
  world_clear_events(world);
  step(world, &fire, 0.6f);
  CHECK(world_has_event(world, EVENT_SHOOT));
  free(world);
}

void run_combat_tests(void) {
  RUN(without_a_gun_shooting_does_nothing);
  RUN(with_a_gun_a_bullet_leaves_the_muzzle);
  RUN(a_bullet_explodes_a_zombie);
  RUN(the_explosion_effect_shows_then_fades);
  RUN(a_bullet_explodes_a_bug);
  RUN(a_wall_stops_a_bullet);
  RUN(the_gun_fires_a_few_times_a_second_not_every_frame);
  RUN(shooting_goes_the_way_he_faces);
  RUN(the_pickaxe_kills_what_is_in_front_without_a_gun);
  RUN(the_pickaxe_does_not_reach_behind_him);
  RUN(jumping_on_a_zombie_squashes_it_and_bounces);
  RUN(drinking_the_potion_starts_a_spell_that_fades_away);
  RUN(you_cannot_drink_what_you_do_not_have);
  RUN(a_second_potion_waits_until_the_first_has_faded);
  RUN(under_the_spell_shooting_casts_a_ray_even_without_a_gun);
  RUN(the_ray_does_not_pass_through_walls);
  RUN(once_the_spell_fades_he_is_unarmed_again);
  RUN(the_spell_outranks_the_pistol_but_the_pistol_returns);
}
