/**
 * @file bot.h
 * @brief A simple autoplayer: runs right, jumps walls and pits, shoots,
 * swings the pickaxe and drinks potions. It proves the level can be finished
 * and drives the demo and the snapshot tool.
 */
#ifndef GAME_SIM_BOT_H_
#define GAME_SIM_BOT_H_

#include "controls.h"
#include "world.h"

typedef struct {
  float jump_hold;
  float drink_wait;
} bot_t;

buttons_t bot_decide(const world_t* world, bot_t* bot, float dt);

#endif  // GAME_SIM_BOT_H_
