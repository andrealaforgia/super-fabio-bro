/**
 * @file sdl_keyboard.h
 * @brief Reads the keyboard into a `buttons_t`.
 *
 * Arrows or WASD move, space / Z / up / W jump, shift runs, J / F / control
 * shoots or casts, K / X swings the pickaxe, E / Q drinks the potion.
 */
#ifndef ENGINE_PLATFORM_SDL_KEYBOARD_H_
#define ENGINE_PLATFORM_SDL_KEYBOARD_H_

#include "controls.h"

buttons_t keyboard_read(void);

#endif  // ENGINE_PLATFORM_SDL_KEYBOARD_H_
