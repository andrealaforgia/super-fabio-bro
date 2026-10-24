/**
 * @file controls.h
 * @brief What the player is asking for this frame: the buttons held, and the
 * ones that went down since the last frame. Free of SDL, so the game and its
 * tests share it; `keyboard.c` fills it from the keys.
 */
#ifndef ENGINE_PLATFORM_CONTROLS_H_
#define ENGINE_PLATFORM_CONTROLS_H_

#include <stdbool.h>

typedef struct {
  bool left, right, up, down;
  bool jump, run, shoot, melee, drink;
  bool pause, confirm, restart, fullscreen, mute;
} buttons_t;

typedef struct {
  buttons_t held;
  buttons_t pressed;
} controls_t;

/* Stores `now` as the held buttons; `pressed` is what was not held before. */
void controls_update(controls_t* controls, const buttons_t* now);

#endif  // ENGINE_PLATFORM_CONTROLS_H_
