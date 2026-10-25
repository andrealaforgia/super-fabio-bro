/**
 * @file buttons.h
 * @brief The buttons this game reads, as bits of the engine's controls mask.
 */
#ifndef GAME_SIM_BUTTONS_H_
#define GAME_SIM_BUTTONS_H_

#include <stdint.h>

#include "controls.h"

typedef uint32_t buttons_t;

enum {
  BTN_LEFT = 1u << 0,
  BTN_RIGHT = 1u << 1,
  BTN_UP = 1u << 2,
  BTN_DOWN = 1u << 3,
  BTN_JUMP = 1u << 4,
  BTN_RUN = 1u << 5,
  BTN_SHOOT = 1u << 6,
  BTN_MELEE = 1u << 7,
  BTN_DRINK = 1u << 8,
  BTN_PAUSE = 1u << 9,
  BTN_CONFIRM = 1u << 10,
  BTN_RESTART = 1u << 11,
  BTN_FULLSCREEN = 1u << 12,
  BTN_MUTE = 1u << 13,
  BTN_SOUND = 1u << 14,
};

#endif  // GAME_SIM_BUTTONS_H_
