#include <math.h>
#include <string.h>

#include "bot.h"
#include "test_groups.h"
#include "test_support.h"
#include "test_world.h"

static void play(world_t* world, bot_t* bot, float seconds) {
  controls_t controls;
  memset(&controls, 0, sizeof(controls));
  const float dt = 1.0f / 60.0f;
  for (float t = 0.0f; t < seconds && world->state == STATE_PLAYING; t += dt) {
    buttons_t now = bot_decide(world, bot, dt);
    controls_update(&controls, now);
    world_update(world, &controls, dt);
  }
}

static void every_level_can_be_walked_from_start_to_castle(void) {
  world_t* world = world_new(1, NULL);
  for (uint32_t seed = 1; seed <= 12; seed++) {
    world_restart(world, seed);
    world->spawning = false;
    bot_t bot;
    memset(&bot, 0, sizeof(bot));
    play(world, &bot, 240.0f);
    if (world->state != STATE_WON)
      fprintf(stderr, "seed %u stuck at x=%.0f (goal %.0f) hp=%d\n", seed,
              world->player.x, world->level.goal_x, world->player.hp);
    CHECK(world->state == STATE_WON);
    CHECK(world->player.hp == PLAYER_HP);
  }
  world_free(world);
}

static void the_bot_picks_up_the_pistol_and_a_potion_on_the_way(void) {
  world_t* world = world_new(1, NULL);
  world->spawning = false;
  bot_t bot;
  memset(&bot, 0, sizeof(bot));
  play(world, &bot, 240.0f);
  CHECK(world->coins > 5);
  CHECK(world->score > 1000);
  world_free(world);
}

static void a_long_fight_keeps_the_world_consistent(void) {
  world_t* world = world_new(1, NULL);
  for (uint32_t seed = 1; seed <= 4; seed++) {
    world_restart(world, seed);
    bot_t bot;
    memset(&bot, 0, sizeof(bot));
    controls_t controls;
    memset(&controls, 0, sizeof(controls));
    for (int i = 0; i < 60 * 150; i++) {
      buttons_t now = bot_decide(world, &bot, 1.0f / 60.0f);
      controls_update(&controls, now);
      world_update(world, &controls, 1.0f / 60.0f);
      /* Nobody ever leaves the map or turns to NaN. */
      CHECK(isfinite(world->player.x) && isfinite(world->player.y));
      CHECK(world->player.hp >= 0 && world->player.hp <= PLAYER_HP);
      CHECK(world->player.potions >= 0 && world->player.potions <= 5);
      CHECK(world->player.x >= 0.0f && world->player.x <= level_width());
      size_t cursor = 0;
      for (const enemy_t* e;
           (e = pool_next_active(&world->enemies, &cursor));) {
        CHECK(isfinite(e->x) && isfinite(e->y));
        CHECK(e->x > -200.0f && e->x < level_width() + 200.0f);
      }
      if (world->state == STATE_GAME_OVER || world->state == STATE_WON) break;
    }
  }
  world_free(world);
}

void run_bot_tests(void) {
  RUN(every_level_can_be_walked_from_start_to_castle);
  RUN(the_bot_picks_up_the_pistol_and_a_potion_on_the_way);
  RUN(a_long_fight_keeps_the_world_consistent);
}
