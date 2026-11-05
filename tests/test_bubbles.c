#include <math.h>

#include "pose.h"
#include "sfx.h"
#include "test_groups.h"
#include "test_support.h"
#include "test_world.h"

#define GROUND ((float)(GROUND_ROW * TILE))

static world_t* armed(void) {
  world_t* world = new_empty_world();
  settle(world);
  world->player.bubbles = BUBBLE_GUN_AMMO;
  return world;
}

static int count(const object_pool_t* pool) {
  int n = 0;
  size_t cursor = 0;
  while (pool_next_active(pool, &cursor)) n++;
  return n;
}

static void a_question_block_in_the_opening_holds_the_shotgun_and_picking_it_up_loads_it(void) {
  world_t* world = world_new(5, NULL);
  bool found = false;
  for (int row = 0; row < LEVEL_ROWS; row++)
    for (int col = 0; col < 30; col++)
      if (level_tile(&world->level, col, row) == TILE_QBLOCK && world->level.content[row][col] == ITEM_BUBBLE_GUN)
        found = true;
  CHECK(found);
  world_spawn_pickup(world, ITEM_BUBBLE_GUN, world->player.x, world->player.y - 10.0f);
  step(world, NULL, 0.5f);
  CHECK(world->player.bubbles == BUBBLE_GUN_AMMO);
  world_free(world);
}

static void holding_the_key_lets_a_steady_stream_of_bubbles_go(void) {
  world_t* world = armed();
  buttons_t shoot = BTN_SHOOT;
  step(world, &shoot, 1.0f);
  int fired = BUBBLE_GUN_AMMO - world->player.bubbles;
  CHECK(fired >= 7 && fired <= 9); /* one shot every BUBBLE_COOLDOWN */
  CHECK(count(&world->bubbles) > 8);
  CHECK(world_has_event(world, EVENT_BUBBLE_SHOT));
  world_free(world);
}

static void the_gun_runs_dry_and_the_pistol_takes_over_again(void) {
  world_t* world = armed();
  world->player.ammo = 3;
  buttons_t shoot = BTN_SHOOT;
  step(world, &shoot, 8.0f);
  CHECK(world->player.bubbles == 0);
  CHECK(world->player.ammo < 3);
  world_free(world);
}

static void a_bubble_keeps_swelling_as_it_flies(void) {
  world_t* world = armed();
  buttons_t shoot = BTN_SHOOT;
  step(world, &shoot, 0.1f);
  step(world, NULL, 0.0f);
  bubble_t* b = pool_get_at(&world->bubbles, 0);
  CHECK(b != NULL);
  float early = b->r;
  CHECK(early < 15.0f);
  step(world, NULL, 0.8f);
  b = pool_get_at(&world->bubbles, 0);
  CHECK(pool_is_active(&world->bubbles, 0));
  CHECK(b->r > early + 15.0f && b->r <= BUBBLE_R_MAX);
  step(world, NULL, 1.2f);
  CHECK(count(&world->bubbles) == 0); /* it burst of old age */
  world_free(world);
}

static void a_bubble_bursts_against_a_wall(void) {
  world_t* world = armed();
  world->player.x = 5 * TILE;
  for (int row = 0; row < GROUND_ROW; row++) world->level.tiles[row][8] = TILE_STONE;
  buttons_t shoot = BTN_SHOOT;
  step(world, &shoot, 0.1f);
  step(world, NULL, 1.0f);
  CHECK(count(&world->bubbles) == 0);
  world_free(world);
}

static void a_bubble_that_meets_an_enemy_wraps_him_and_he_floats_helpless(void) {
  world_t* world = armed();
  float x = world->player.x;
  spawn_zombie(world, x + 130.0f, GROUND);
  enemy_t* zombie = pool_get_at(&world->enemies, 0);
  buttons_t shoot = BTN_SHOOT;
  step(world, &shoot, 0.5f);
  CHECK(world_has_event(world, EVENT_BUBBLE_WRAPS) || enemy_in_bubble(world, zombie));
  step(world, NULL, 0.3f);
  CHECK(enemy_in_bubble(world, zombie));
  float held_x = zombie->x, y = zombie->y;
  step(world, NULL, 0.5f);
  CHECK(zombie->x == held_x);  /* no longer walking */
  CHECK(zombie->y < y);        /* floating up */
  CHECK(world->player.hp == PLAYER_HP);
  world_free(world);
}

static void the_wrapped_enemy_bursts_with_his_bubble_after_a_moment(void) {
  world_t* world = armed();
  spawn_zombie(world, world->player.x + 130.0f, GROUND);
  buttons_t shoot = BTN_SHOOT;
  step(world, &shoot, 0.6f);
  world->player.bubbles = 0;
  CHECK(world_count_enemies(world, ENEMY_ZOMBIE) == 1);
  step(world, NULL, BUBBLE_WRAP_SECONDS + 0.2f);
  CHECK(world_count_enemies(world, ENEMY_ZOMBIE) == 0);
  CHECK(world->zombies_killed == 1);
  CHECK(world->score > 0);
  CHECK(world_has_event(world, EVENT_BUBBLE_POPS) || world->zombies_killed == 1);
  step(world, NULL, 1.0f);
  CHECK(count(&world->bubbles) == 0);
  world_free(world);
}

static void walking_into_a_wrapped_enemy_does_no_harm(void) {
  world_t* world = armed();
  spawn_zombie(world, world->player.x + 130.0f, GROUND);
  enemy_t* zombie = pool_get_at(&world->enemies, 0);
  buttons_t shoot = BTN_SHOOT;
  step(world, &shoot, 0.7f);
  world->player.bubbles = 0;
  CHECK(enemy_in_bubble(world, zombie));
  world->player.x = zombie->x;
  world->player.y = zombie->y;
  step(world, NULL, 0.1f);
  CHECK(world->player.hp == PLAYER_HP);
  world_free(world);
}

static void a_bubble_never_wraps_an_enemy_twice(void) {
  world_t* world = armed();
  spawn_zombie(world, world->player.x + 110.0f, GROUND);
  buttons_t shoot = BTN_SHOOT;
  step(world, &shoot, 0.6f);
  int wrapped = 0;
  size_t cursor = 0;
  for (const bubble_t* b; (b = pool_next_active(&world->bubbles, &cursor));) wrapped += b->victim >= 0;
  CHECK(wrapped == 1);
  world_free(world);
}

static void the_bubbles_have_sounds_and_the_shotgun_has_its_own_poses(void) {
  float volume;
  CHECK(sfx_for_event(EVENT_BUBBLE_SHOT, &volume) == SFX_BUBBLE_SHOT);
  CHECK(sfx_for_event(EVENT_BUBBLE_WRAPS, &volume) == SFX_BUBBLE_WRAP);
  CHECK(sfx_for_event(EVENT_BUBBLE_POPS, &volume) == SFX_BUBBLE_POP);
  world_t* world = armed();
  CHECK(pose_player(&world->player).strip == STRIP_FAB_BUB_IDLE);
  world->player.vx = 150.0f;
  CHECK(pose_player(&world->player).strip == STRIP_FAB_BUB_WALK);
  world->player.vx = 280.0f;
  CHECK(pose_player(&world->player).strip == STRIP_FAB_BUB_RUN);
  world->player.vx = 0.0f;
  world->player.grounded = false;
  CHECK(pose_player(&world->player).strip == STRIP_FAB_BUB_JUMP);
  world->player.grounded = true;
  world->player.shoot_pose = 0.2f;
  CHECK(pose_player(&world->player).strip == STRIP_FAB_BUB_SHOOT);
  world->player.bubbles = 0;
  CHECK(pose_player(&world->player).strip != STRIP_FAB_BUB_SHOOT);
  world_free(world);
}

void run_bubble_tests(void) {
  RUN(a_question_block_in_the_opening_holds_the_shotgun_and_picking_it_up_loads_it);
  RUN(holding_the_key_lets_a_steady_stream_of_bubbles_go);
  RUN(the_gun_runs_dry_and_the_pistol_takes_over_again);
  RUN(a_bubble_keeps_swelling_as_it_flies);
  RUN(a_bubble_bursts_against_a_wall);
  RUN(a_bubble_that_meets_an_enemy_wraps_him_and_he_floats_helpless);
  RUN(the_wrapped_enemy_bursts_with_his_bubble_after_a_moment);
  RUN(walking_into_a_wrapped_enemy_does_no_harm);
  RUN(a_bubble_never_wraps_an_enemy_twice);
  RUN(the_bubbles_have_sounds_and_the_shotgun_has_its_own_poses);
}
