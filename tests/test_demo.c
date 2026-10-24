#include <stdlib.h>
#include <string.h>

#include "demo.h"
#include "test_groups.h"
#include "test_support.h"

typedef struct {
  int stomps, hurts, drinks, spells, shots, jumps;
  int off_screen_spawns, on_screen_spawns;
  float finished_at;
  int deepest_step;
  bool won;
} log_t;

static void play(world_t* world, demo_t* demo, log_t* log, float limit) {
  controls_t controls;
  memset(&controls, 0, sizeof(controls));
  memset(log, 0, sizeof(*log));
  const float dt = 1.0f / 60.0f;
  bool seen[MAX_ENEMIES] = {false};
  for (float t = 0.0f; t < limit && !demo_finished(demo); t += dt) {
    buttons_t now = demo_decide(world, demo, dt);
    controls_update(&controls, &now);
    world_update(world, &controls, dt);
    for (int i = 0; i < world->event_count; i++) {
      switch (world->events[i].kind) {
        case EVENT_STOMP: log->stomps++; break;
        case EVENT_HURT: log->hurts++; break;
        case EVENT_DRINK: log->drinks++; break;
        case EVENT_SPELL: log->spells++; break;
        case EVENT_SHOOT: log->shots++; break;
        case EVENT_JUMP: log->jumps++; break;
        default: break;
      }
    }
    world_clear_events(world);
    for (int k = 0; k < MAX_ENEMIES; k++) {
      const enemy_t* e = &world->enemies[k];
      if (e->alive && !seen[k]) {
        float edge = world->camera_x + world->view_w;
        if (e->x >= edge) log->off_screen_spawns++;
        else log->on_screen_spawns++;
      }
      seen[k] = e->alive;
    }
    if (demo->step > log->deepest_step) log->deepest_step = demo->step;
    log->finished_at = t;
  }
  log->won = world->state == STATE_WON;
}

static void run(world_t** world, demo_t* demo, log_t* log) {
  *world = malloc(sizeof(**world));
  demo_world_init(*world, demo, 4);
  play(*world, demo, log, 400.0f);
}

static void the_script_runs_from_the_first_step_to_the_castle(void) {
  world_t* world;
  demo_t demo;
  log_t log;
  run(&world, &demo, &log);
  if (!log.won)
    fprintf(stderr, "stuck in step %d (kind %d) at x=%.0f after %.0f s\n", demo.step,
            demo_steps()[demo.step].kind, world->player.x, log.finished_at);
  CHECK(log.won);
  CHECK(demo_finished(&demo));
  CHECK(log.finished_at > 60.0f && log.finished_at < 300.0f);
  free(world);
}

static void fabio_jumps_all_along_the_course(void) {
  world_t* world;
  demo_t demo;
  log_t log;
  run(&world, &demo, &log);
  CHECK(log.jumps >= 30);
  free(world);
}

static void zombies_and_bugs_are_stomped_and_he_is_pushed_back_too(void) {
  world_t* world;
  demo_t demo;
  log_t log;
  run(&world, &demo, &log);
  CHECK(log.stomps >= 4);
  CHECK(log.hurts >= 2);
  free(world);
}

static void enemies_always_walk_in_from_the_right_edge_of_the_screen(void) {
  world_t* world;
  demo_t demo;
  log_t log;
  run(&world, &demo, &log);
  CHECK(log.off_screen_spawns >= 20);
  CHECK(log.on_screen_spawns == 0);
  free(world);
}

static void three_potions_are_drunk_and_each_spell_is_used(void) {
  world_t* world;
  demo_t demo;
  log_t log;
  run(&world, &demo, &log);
  CHECK(log.drinks == 3);
  CHECK(log.spells >= 12);
  CHECK(world->level.tiles[GROUND_ROW - 4][212] == TILE_USED);
  CHECK(world->level.tiles[GROUND_ROW - 4][262] == TILE_USED);
  CHECK(world->level.tiles[GROUND_ROW - 4][320] == TILE_USED);
  free(world);
}

static void the_pistol_is_found_and_fired(void) {
  world_t* world;
  demo_t demo;
  log_t log;
  run(&world, &demo, &log);
  CHECK(world->player.has_gun);
  CHECK(log.shots >= 6);
  CHECK(world->zombies_killed >= 12);
  CHECK(world->bugs_killed >= 10);
  free(world);
}

static void the_course_has_pipes_pits_stairs_and_stepping_stones(void) {
  world_t* world = malloc(sizeof(*world));
  demo_t demo;
  demo_world_init(world, &demo, 4);
  int pipes = 0, pits = 0, stone = 0, gold = 0, bricks = 0;
  bool in_pit = false;
  for (int col = 0; col < 440; col++) {
    for (int row = 0; row < GROUND_ROW; row++) {
      tile_t t = level_tile(&world->level, col, row);
      pipes += t == TILE_PIPE_TOP_L;
      stone += t == TILE_STONE;
      gold += t == TILE_GOLD;
      bricks += t == TILE_BRICK;
    }
    bool hole = level_tile(&world->level, col, GROUND_ROW) == TILE_EMPTY;
    if (hole && !in_pit) pits++;
    in_pit = hole;
  }
  CHECK(pipes >= 8);
  CHECK(pits >= 6);
  CHECK(stone >= 20);
  CHECK(gold >= 4);
  CHECK(bricks >= 10);
  CHECK(world->level.decor_count > 20);
  CHECK(world->level.item_count > 20);
  free(world);
}

void run_demo_tests(void) {
  RUN(the_script_runs_from_the_first_step_to_the_castle);
  RUN(fabio_jumps_all_along_the_course);
  RUN(zombies_and_bugs_are_stomped_and_he_is_pushed_back_too);
  RUN(enemies_always_walk_in_from_the_right_edge_of_the_screen);
  RUN(three_potions_are_drunk_and_each_spell_is_used);
  RUN(the_pistol_is_found_and_fired);
  RUN(the_course_has_pipes_pits_stairs_and_stepping_stones);
}
