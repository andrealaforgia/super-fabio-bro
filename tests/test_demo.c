#include <stdlib.h>
#include <string.h>

#include "demo.h"
#include "test_groups.h"
#include "test_support.h"

typedef struct {
  int stomps, hurts, potions_drunk, spells, shots, melee;
  int zombies_at_phase[DEMO_DONE + 1], bugs_at_phase[DEMO_DONE + 1];
  float finished_at;
} log_t;

static void play(world_t* world, demo_t* demo, log_t* log) {
  controls_t controls;
  memset(&controls, 0, sizeof(controls));
  memset(log, 0, sizeof(*log));
  const float dt = 1.0f / 60.0f;
  for (float t = 0.0f; t < 150.0f && !demo_finished(demo); t += dt) {
    buttons_t now = demo_decide(world, demo, dt);
    controls_update(&controls, &now);
    world_update(world, &controls, dt);
    for (int i = 0; i < world->event_count; i++) {
      switch (world->events[i].kind) {
        case EVENT_STOMP: log->stomps++; break;
        case EVENT_HURT: log->hurts++; break;
        case EVENT_DRINK: log->potions_drunk++; break;
        case EVENT_SPELL: log->spells++; break;
        case EVENT_SHOOT: log->shots++; break;
        default: break;
      }
    }
    world_clear_events(world);
    log->zombies_at_phase[demo->phase] = world->zombies_killed;
    log->bugs_at_phase[demo->phase] = world->bugs_killed;
    log->finished_at = t;
  }
}

static void the_script_runs_from_the_first_step_to_the_last(void) {
  world_t* world = malloc(sizeof(*world));
  demo_t demo;
  log_t log;
  demo_world_init(world, &demo, 4);
  play(world, &demo, &log);
  if (!demo_finished(&demo))
    fprintf(stderr, "stuck in phase %d at x=%.0f after %.0f s\n", demo.phase,
            world->player.x, log.finished_at);
  CHECK(demo_finished(&demo));
  CHECK(log.finished_at < 120.0f);
  free(world);
}

static void fabio_stomps_a_zombie_and_a_bug_and_is_pushed_back_by_each(void) {
  world_t* world = malloc(sizeof(*world));
  demo_t demo;
  log_t log;
  demo_world_init(world, &demo, 4);
  play(world, &demo, &log);
  CHECK(log.stomps >= 4);
  CHECK(log.hurts >= 2);
  CHECK(log.zombies_at_phase[DEMO_STOMP_ZOMBIE] >= 1);
  CHECK(log.zombies_at_phase[DEMO_PUSHED_BY_ZOMBIE] >= 2);
  CHECK(log.bugs_at_phase[DEMO_STOMP_BUG] >= 1);
  CHECK(log.bugs_at_phase[DEMO_PUSHED_BY_BUG] >= 1);
  free(world);
}

static void the_question_block_gives_the_potion_which_he_drinks(void) {
  world_t* world = malloc(sizeof(*world));
  demo_t demo;
  log_t log;
  demo_world_init(world, &demo, 4);
  play(world, &demo, &log);
  CHECK(world->level.tiles[GROUND_ROW - 4][demo.potion_col] == TILE_USED);
  CHECK(log.potions_drunk == 1);
  CHECK(log.spells >= 4);
  free(world);
}

static void the_spell_kills_zombies_and_bugs(void) {
  world_t* world = malloc(sizeof(*world));
  demo_t demo;
  log_t log;
  demo_world_init(world, &demo, 4);
  play(world, &demo, &log);
  int zombies = log.zombies_at_phase[DEMO_SPELL_FIGHT] - log.zombies_at_phase[DEMO_DRINK];
  int bugs = log.bugs_at_phase[DEMO_SPELL_FIGHT] - log.bugs_at_phase[DEMO_DRINK];
  CHECK(zombies >= 3);
  CHECK(bugs >= 3);
  free(world);
}

static void the_pistol_comes_from_the_second_block_and_kills_more(void) {
  world_t* world = malloc(sizeof(*world));
  demo_t demo;
  log_t log;
  demo_world_init(world, &demo, 4);
  play(world, &demo, &log);
  CHECK(world->player.has_gun);
  CHECK(log.shots >= 6);
  int zombies = log.zombies_at_phase[DEMO_GUN_FIGHT] - log.zombies_at_phase[DEMO_CLIMB_FOR_PISTOL];
  int bugs = log.bugs_at_phase[DEMO_GUN_FIGHT] - log.bugs_at_phase[DEMO_CLIMB_FOR_PISTOL];
  CHECK(zombies >= 3);
  CHECK(bugs >= 3);
  CHECK(world->player.alive);
  free(world);
}

void run_demo_tests(void) {
  RUN(the_script_runs_from_the_first_step_to_the_last);
  RUN(fabio_stomps_a_zombie_and_a_bug_and_is_pushed_back_by_each);
  RUN(the_question_block_gives_the_potion_which_he_drinks);
  RUN(the_spell_kills_zombies_and_bugs);
  RUN(the_pistol_comes_from_the_second_block_and_kills_more);
}
