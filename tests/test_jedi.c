#include <math.h>

#include "pose.h"
#include "sfx.h"
#include "test_groups.h"
#include "test_support.h"
#include "test_world.h"

#define GROUND ((float)(GROUND_ROW * TILE))

static world_t* knight(void) {
  world_t* world = new_empty_world();
  settle(world);
  world->player.jedi_time = JEDI_SECONDS;
  return world;
}

static int bursts(const world_t* world) {
  return (int)pool_get_active_count(&world->force_bursts);
}

static bool effect_live(const world_t* world, fx_kind_t kind) {
  size_t cursor = 0;
  for (const effect_t* fx; (fx = pool_next_active(&world->effects, &cursor));)
    if (fx->kind == kind) return true;
  return false;
}

static void the_opening_row_holds_the_lightsaber_and_it_turns_fabio_into_a_knight(void) {
  world_t* world = world_new(5, NULL);
  bool found = false;
  for (int row = 0; row < LEVEL_ROWS; row++)
    for (int col = 0; col < 30; col++)
      if (level_tile(&world->level, col, row) == TILE_QBLOCK && world->level.content[row][col] == ITEM_LIGHTSABER)
        found = true;
  CHECK(found);
  CHECK(world->player.jedi_time == 0.0f && world->greeting_left == 0.0f);
  world_spawn_pickup(world, ITEM_LIGHTSABER, world->player.x, world->player.y - 10.0f);
  step(world, NULL, 0.5f);
  CHECK(world->player.jedi_time > JEDI_SECONDS - 1.0f);
  CHECK(world->greeting_left > 0.0f && world->greeting_left <= GREETING_SECONDS);
  CHECK(world->score > 0);
  world_free(world);
}

static void the_shoot_key_swings_the_saber_and_sends_a_burst_not_a_bullet(void) {
  world_t* world = knight();
  world->player.ammo = 5;
  step(world, &(buttons_t){BTN_SHOOT}, 0.02f);
  CHECK(bursts(world) == 1);
  CHECK(world->player.ammo == 5); /* the pistol keeps its bullets */
  CHECK(pose_player(&world->player).strip == STRIP_FAB_JEDI_SWING);
  CHECK(pool_get_active_count(&world->bullets) == 0);
  step(world, &(buttons_t){BTN_SHOOT}, 1.0f);
  int fired = 1 + (int)(1.0f / JEDI_COOLDOWN);
  CHECK(bursts(world) >= 2 && bursts(world) <= fired);
  world_free(world);
}

static void a_burst_destroys_everything_it_passes_through_and_nothing_behind_or_beyond(void) {
  world_t* world = knight();
  world->zombie_timer = 99.0f;
  float x = world->player.x;
  spawn_zombie(world, x + 200.0f, GROUND);
  spawn_zombie(world, x + 420.0f, GROUND);
  spawn_zombie(world, x - 200.0f, GROUND);   /* behind him */
  spawn_zombie(world, x + 1500.0f, GROUND);  /* beyond its reach */
  for (int i = 0; i < 4; i++) enemy_at(world, (size_t)i)->speed = 0.0f;
  step(world, &(buttons_t){BTN_SHOOT}, 0.02f);
  step(world, NULL, 1.0f);
  CHECK(world->zombies_killed == 2);
  CHECK(world_count_enemies(world, ENEMY_ZOMBIE) == 2);
  CHECK(world->score > 0);
  world_free(world);
}

static void a_burst_leaves_green_sparks_and_goes_on(void) {
  world_t* world = knight();
  world->zombie_timer = 99.0f;
  spawn_zombie(world, world->player.x + 200.0f, GROUND);
  enemy_at(world, 0)->speed = 0.0f;
  step(world, &(buttons_t){BTN_SHOOT}, 0.02f);
  bool sparks = false;
  for (int i = 0; i < 30 && !sparks; i++) {
    step(world, NULL, 0.02f);
    sparks = effect_live(world, FX_FORCE_HIT);
  }
  CHECK(sparks);
  CHECK(bursts(world) == 1); /* it flew on through him */
  world_free(world);
}

static void a_wall_stops_the_burst(void) {
  world_t* world = knight();
  world->zombie_timer = 99.0f;
  for (int row = 0; row < GROUND_ROW; row++) world->level.tiles[row][14] = TILE_STONE;
  world->player.x = 10 * TILE;
  spawn_zombie(world, 18 * TILE, GROUND);
  enemy_at(world, 0)->speed = 0.0f;
  step(world, &(buttons_t){BTN_SHOOT}, 0.02f);
  step(world, NULL, 1.0f);
  CHECK(world_count_enemies(world, ENEMY_ZOMBIE) == 1);
  CHECK(bursts(world) == 0);
  world_free(world);
}

static void the_force_fades_after_twenty_seconds_and_the_robes_with_it(void) {
  world_t* world = knight();
  CHECK(pose_player(&world->player).strip == STRIP_FAB_JEDI_IDLE);
  step(world, NULL, JEDI_SECONDS - 1.0f);
  CHECK(world->player.jedi_time > 0.0f);
  step(world, NULL, 1.2f);
  CHECK(world->player.jedi_time == 0.0f);
  CHECK(pose_player(&world->player).strip == STRIP_FAB_IDLE);
  world->player.ammo = 3;
  step(world, &(buttons_t){BTN_SHOOT}, 0.02f);
  CHECK(bursts(world) == 0 && world->player.ammo == 2);
  world_free(world);
}

static void the_knight_has_his_own_poses_for_every_move(void) {
  world_t* world = knight();
  world->player.vx = 150.0f;
  CHECK(pose_player(&world->player).strip == STRIP_FAB_JEDI_WALK);
  world->player.vx = 280.0f;
  CHECK(pose_player(&world->player).strip == STRIP_FAB_JEDI_RUN);
  world->player.vx = 0.0f;
  world->player.grounded = false;
  CHECK(pose_player(&world->player).strip == STRIP_FAB_JEDI_JUMP);
  world->player.grounded = true;
  world->player.hurt_timer = 0.2f;
  CHECK(pose_player(&world->player).strip == STRIP_FAB_JEDI_HURT);
  world_free(world);
}

static void the_old_knights_greeting_fades_in_holds_and_fades_out(void) {
  CHECK(greeting_alpha(0.0f) == 0.0f);
  CHECK(greeting_alpha(GREETING_SECONDS) == 0.0f);
  float in = greeting_alpha(GREETING_SECONDS - GREETING_FADE_IN * 0.5f);
  CHECK(in > 0.4f && in < 0.6f);
  CHECK(greeting_alpha(GREETING_SECONDS * 0.5f) == 1.0f);
  float out = greeting_alpha(GREETING_FADE_OUT * 0.5f);
  CHECK(out > 0.4f && out < 0.6f);
  world_t* world = knight();
  world->greeting_left = GREETING_SECONDS;
  step(world, NULL, GREETING_SECONDS - 0.5f);
  CHECK(world->greeting_left > 0.0f);
  step(world, NULL, 0.6f);
  CHECK(world->greeting_left == 0.0f);
  world_free(world);
}

static void his_mouth_moves_while_he_speaks_and_is_shut_as_he_fades(void) {
  int changes = 0;
  int last = pose_old_knight(GREETING_SECONDS * 0.5f).frame;
  for (float left = GREETING_SECONDS - GREETING_FADE_IN - 0.01f; left > GREETING_FADE_OUT + 0.01f; left -= 0.02f) {
    int now = pose_old_knight(left).frame;
    changes += now != last;
    last = now;
  }
  CHECK(changes >= 8);
  CHECK(pose_old_knight(0.3f).frame == 0 && pose_old_knight(0.3f).strip == STRIP_OLD_KNIGHT);
}

static void the_saber_has_its_sounds(void) {
  float volume;
  CHECK(sfx_for_event(EVENT_LIGHTSABER, &volume) == SFX_SABER_ON);
  CHECK(sfx_for_event(EVENT_FORCE_BURST, &volume) == SFX_FORCE_BURST);
  world_t* world = knight();
  step(world, &(buttons_t){BTN_SHOOT}, 0.02f);
  CHECK(world_has_event(world, EVENT_FORCE_BURST));
  world_free(world);
}

void run_jedi_tests(void) {
  RUN(the_opening_row_holds_the_lightsaber_and_it_turns_fabio_into_a_knight);
  RUN(the_shoot_key_swings_the_saber_and_sends_a_burst_not_a_bullet);
  RUN(a_burst_destroys_everything_it_passes_through_and_nothing_behind_or_beyond);
  RUN(a_burst_leaves_green_sparks_and_goes_on);
  RUN(a_wall_stops_the_burst);
  RUN(the_force_fades_after_twenty_seconds_and_the_robes_with_it);
  RUN(the_knight_has_his_own_poses_for_every_move);
  RUN(the_old_knights_greeting_fades_in_holds_and_fades_out);
  RUN(his_mouth_moves_while_he_speaks_and_is_shut_as_he_fades);
  RUN(the_saber_has_its_sounds);
}
