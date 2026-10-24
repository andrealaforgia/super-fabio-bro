/**
 * @file demo.h
 * @brief A scripted showcase that plays the real game: Fabio walks, stomps a
 * zombie, is pushed back by another and stomps it, does the same to bugs,
 * opens a ? block that holds the TDD potion, drinks it and blasts zombies
 * and bugs with the spell, finds the pistol and shoots more of them.
 *
 * The director presses the buttons and stages the enemies; everything else
 * is the ordinary simulation.
 */
#ifndef GAME_SIM_DEMO_H_
#define GAME_SIM_DEMO_H_

#include <stdbool.h>

#include "controls.h"
#include "world.h"

typedef enum {
  DEMO_WALK,
  DEMO_STOMP_ZOMBIE,
  DEMO_PUSHED_BY_ZOMBIE,
  DEMO_STOMP_BUG,
  DEMO_PUSHED_BY_BUG,
  DEMO_TO_POTION_BLOCK,
  DEMO_CLIMB_FOR_POTION,
  DEMO_DRINK,
  DEMO_SPELL_FIGHT,
  DEMO_TO_PISTOL_BLOCK,
  DEMO_CLIMB_FOR_PISTOL,
  DEMO_GUN_FIGHT,
  DEMO_FINALE,
  DEMO_DONE
} demo_phase_t;

typedef struct {
  demo_phase_t phase;
  float phase_time;
  bool entered;
  int stage;
  float jump_hold, jump_cool;
  int base_zombies, base_bugs, base_hp;
  bool hurt_seen, drink_sent;
  int wave;
  float stage_time;
  int potion_col, pistol_col;
} demo_t;

/* Flattens the level into an arena with the two ? blocks and starts the
 * script. */
void demo_world_init(world_t* world, demo_t* demo, uint32_t seed);
buttons_t demo_decide(world_t* world, demo_t* demo, float dt);
bool demo_finished(const demo_t* demo);
/* What is happening, for a caption on screen. */
const char* demo_caption(const demo_t* demo);

#endif  // GAME_SIM_DEMO_H_
