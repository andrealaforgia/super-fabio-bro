/**
 * @file demo.h
 * @brief A scripted showcase that plays the real game on a hand-built course:
 * pipes, pits, stairs, stepping stones and floating bricks. Fabio walks and
 * jumps his way along, stomps zombies and bugs that come in from the right
 * edge of the screen, is knocked back by some first, opens ? blocks for three
 * TDD potions and a pistol, blasts waves of enemies with the spell and the
 * gun, and walks into the castle.
 *
 * The director only presses buttons and stages enemies; everything else is
 * the ordinary simulation.
 */
#ifndef GAME_SIM_DEMO_H_
#define GAME_SIM_DEMO_H_

#include <stdbool.h>

#include "bot.h"
#include "controls.h"
#include "world.h"

typedef enum {
  STEP_TRAVEL,       /* arg: column to reach */
  STEP_STOMP_ZOMBIE,
  STEP_PUSHED_BY_ZOMBIE,
  STEP_STOMP_BUG,
  STEP_PUSHED_BY_BUG,
  STEP_BLOCK,        /* arg: column of the ? block to open from below */
  STEP_CLIMB,        /* arg: column; climbs up and takes what came out */
  STEP_DRINK,
  STEP_SPELL_FIGHT,
  STEP_GUN_FIGHT,
  STEP_CASTLE
} demo_step_kind_t;

typedef struct {
  demo_step_kind_t kind;
  int arg;
} demo_step_t;

typedef struct {
  int step;
  float step_time;
  bool entered;
  int stage;
  float stage_time;
  float stage_x;
  float jump_hold, jump_cool;
  int base_zombies, base_bugs, base_hp;
  bool hurt_seen, drink_sent;
  int wave;
  float finished_time;
  bot_t travel;
} demo_t;

/* Builds the course into the world and starts the script. */
void demo_world_init(world_t* world, demo_t* demo, uint32_t seed);
buttons_t demo_decide(world_t* world, demo_t* demo, float dt);
/* True a few seconds after Fabio walks into the castle. */
bool demo_finished(const demo_t* demo);
const char* demo_caption(const demo_t* demo);
int demo_step_count(void);
const demo_step_t* demo_steps(void);

#endif  // GAME_SIM_DEMO_H_
